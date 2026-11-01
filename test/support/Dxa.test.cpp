#include "support/dxa.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>
#include <QFile>
#include <QTemporaryDir>
#include <QtEndian>
#include <array>
#include <functional>

namespace {
template<typename T>
void
put(QByteArray& bytes, qsizetype offset, T value)
{
    qToLittleEndian(value, bytes.data() + offset);
}

auto
nameRecord(const QByteArray& name) -> QByteArray
{
    const auto paddedSize = (name.size() + 4) / 4 * 4;
    QByteArray record(4 + 2 * paddedSize, '\0');
    put<quint16>(record, 0, paddedSize / 4);
    record.replace(4, name.size(), name.toUpper());
    record.replace(4 + paddedSize, name.size(), name);
    return record;
}

struct Archive
{
    int version = 5;
    QByteArray header;
    QByteArray names = nameRecord("image.bin");
    QByteArray files;
    QByteArray directories = QByteArray(16, '\0');
    QByteArray payload;

    explicit Archive(int version = 5, QByteArray contents = "ABC")
      : version(version)
      , header(version <= 3 ? 24 : 28, '\0')
      , files(version == 1 ? 40 : 44, '\0')
      , payload(std::move(contents))
    {
        put<quint16>(header, 0, 0x5844);
        put<quint16>(header, 2, version);
        put<quint32>(header, 8, header.size());
        put<quint32>(files, 36, payload.size());
        if (version >= 2)
            put<quint32>(files, 40, 0xffffffff);
        put<quint32>(directories, 0, 0xffffffff);
        put<quint32>(directories, 4, 0xffffffff);
        put<quint32>(directories, 8, 1);
    }

    void compress(QByteArray stream, quint32 outputSize)
    {
        payload = QByteArray(9, '\0') + stream;
        put<quint32>(payload, 0, outputSize);
        put<quint32>(payload, 4, payload.size());
        payload[8] = char(0xff);
        put<quint32>(files, 36, outputSize);
        put<quint32>(files, 40, payload.size());
    }

    auto encode(std::function<void(QByteArray&)> alterHeader = {}) const
      -> QByteArray
    {
        auto head = header;
        auto body = payload;
        auto tables = names + files + directories;
        put<quint32>(head, 4, tables.size());
        put<quint32>(head, 12, head.size() + body.size());
        put<quint32>(head, 16, names.size());
        put<quint32>(head, 20, names.size() + files.size());
        if (alterHeader)
            alterHeader(head);
        const std::array<quint8, 12> key = {
            0x55, 0xaa, 0x20, 0x55, 0x55, 0x06,
            0x55, 0xaa, 0x55, 0xd5, 0x7c, 0x66
        };
        auto encrypt = [&](QByteArray& bytes, quint32 offset) {
            for (qsizetype i = 0; i < bytes.size(); ++i)
                bytes[i] = char(quint8(bytes[i]) ^
                                (version <= 2 ? 0xff : key[(offset + i) % 12]));
        };
        encrypt(tables, version == 5 ? 0 : head.size() + body.size());
        encrypt(body,
                version == 5 ? qFromLittleEndian<quint32>(
                                 files.constData() + files.size() -
                                 (version == 1 ? 40 : 44) + 36)
                             : head.size());
        encrypt(head, 0);
        return head + body + tables;
    }
};

auto
read(const QByteArray& encoded) -> support::DXArchive
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = directory.filePath("fixture.dxa");
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(encoded) == encoded.size());
    file.close();
    return support::extractDxaToMem(support::qStringToPath(path));
}
}

TEST_CASE("DXA reads all supported versions and empty files", "[dxa]")
{
    for (int version = 1; version <= 5; ++version) {
        CAPTURE(version);
        for (const QByteArray content : { QByteArray("ABC"), QByteArray{} }) {
            const auto result = read(Archive(version, content).encode());
            REQUIRE(result.size() == 1);
            const auto& entry = result.at("image.bin");
            CHECK(QByteArray(reinterpret_cast<const char*>(entry.data.get()),
                             entry.size) == content);
        }
    }
}

TEST_CASE("DXA rejects truncated headers, tables and payloads", "[dxa]")
{
    const auto valid = Archive{}.encode();
    for (qsizetype length = 0; length < valid.size(); ++length) {
        CAPTURE(length);
        CHECK(read(valid.first(length)).empty());
    }
    CHECK(read(Archive{}.encode(
                 [](QByteArray& header) { put<quint32>(header, 4, 0); }))
            .empty());
    CHECK(read(Archive{}.encode([](QByteArray& header) {
              put<quint32>(header, 16, 0xffffffff);
          }))
            .empty());
    CHECK(read(Archive{}.encode([](QByteArray& header) {
              put<quint32>(header, 8, 0xfffffff0);
          }))
            .empty());
}

TEST_CASE("DXA bounds names, directory traversal and file sizes", "[dxa]")
{
    Archive archive;
    SECTION("name offset past the table")
    {
        put<quint32>(archive.files, 0, 0xffffffff);
    }
    SECTION("missing name terminator")
    {
        archive.names.fill('A');
        put<quint16>(archive.names, 0, 0);
    }
    SECTION("parent path")
    {
        archive.names = nameRecord("..");
    }
    SECTION("absolute path")
    {
        archive.names = nameRecord("C:/escape");
    }
    SECTION("directory cycle")
    {
        put<quint32>(archive.files, 4, 0x10);
        put<quint32>(archive.files, 32, 0);
    }
    SECTION("file count past the table")
    {
        put<quint32>(archive.directories, 8, 0xffffffff);
    }
    SECTION("payload past end")
    {
        put<quint32>(archive.files, 36, 0xffffffff);
    }
    CHECK(read(archive.encode()).empty());
}

TEST_CASE("DXA validates compressed streams before decoding", "[dxa]")
{
    for (int version = 2; version <= 5; ++version) {
        CAPTURE(version);
        Archive archive(version);
        archive.compress(QByteArray::fromHex("414243ff1002"), 9);
        const auto result = read(archive.encode());
        REQUIRE(result.size() == 1);
        const auto& entry = result.at("image.bin");
        CHECK(QByteArray(reinterpret_cast<const char*>(entry.data.get()),
                         entry.size) == "ABCABCABC");
    }
    for (const QByteArray stream : {
           QByteArray::fromHex("ff"),       // Missing opcode.
           QByteArray::fromHex("ff00"),     // Missing distance.
           QByteArray::fromHex("ff0000"),   // Reference before output.
           QByteArray::fromHex("41ff1000"), // Copy exceeds output.
           QByteArray::fromHex("41424344")  // Extra literal.
         }) {
        CAPTURE(stream.toHex().toStdString());
        Archive archive;
        archive.compress(stream, 3);
        CHECK(read(archive.encode()).empty());
    }
    Archive archive;
    archive.compress("ABC", 3);
    put<quint32>(archive.payload, 4, 0xffffffff);
    CHECK(read(archive.encode()).empty());
}

TEST_CASE("DXA preserves nested directory paths", "[dxa]")
{
    Archive archive;
    const auto nestedName = nameRecord("fonts");
    archive.names = nestedName + archive.names;
    auto directory = QByteArray(44, '\0');
    put<quint32>(directory, 4, 0x10);
    put<quint32>(directory, 32, 16);
    put<quint32>(archive.files, 0, nestedName.size());
    archive.files.prepend(directory);
    auto child = QByteArray(16, '\0');
    put<quint32>(child, 8, 1);
    put<quint32>(child, 12, 44);
    archive.directories.append(child);
    const auto result = read(archive.encode());
    REQUIRE(result.size() == 1);
    CHECK(result.contains("fonts/image.bin"));
}

TEST_CASE("DXA extraction preserves existing edited assets", "[dxa]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto archivePath = directory.filePath("fixture.dxa");
    QFile archive(archivePath);
    REQUIRE(archive.open(QIODevice::WriteOnly));
    const auto encoded = Archive{}.encode();
    REQUIRE(archive.write(encoded) == encoded.size());
    archive.close();
    const auto path = support::qStringToPath(archivePath);
    REQUIRE(support::extractDxaToFile(path) == 0);
    QFile extracted(directory.filePath("fixture/image.bin"));
    REQUIRE(extracted.open(QIODevice::ReadWrite));
    CHECK(extracted.readAll() == "ABC");
    REQUIRE(extracted.resize(0));
    REQUIRE(extracted.seek(0));
    REQUIRE(extracted.write("edited") == 6);
    extracted.close();
    REQUIRE(support::extractDxaToFile(path) == 0);
    REQUIRE(extracted.open(QIODevice::ReadOnly));
    CHECK(extracted.readAll() == "edited");
}

#include <catch2/catch_test_macros.hpp>

#include "qml_components/FileQuery.h"

#include <QFile>
#include <QTemporaryDir>

namespace {

auto
writeBytes(const QString& path, const QByteArray& data) -> void
{
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(data) == data.size());
}

auto
textWith(QChar ch) -> QString
{
    return QString(ch);
}

} // namespace

TEST_CASE("FileQuery reads no-BOM text files as CP932",
          "[FileQuery][encoding]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const auto path = tempDir.filePath("readme.txt");
    writeBytes(path, QByteArray("\x87\x40", 2));

    const auto text = qml_components::FileQuery().readTextFile(path);
    CHECK(text == textWith(QChar(0x2460)));
}

TEST_CASE("FileQuery honors UTF-8 BOM for text files",
          "[FileQuery][encoding]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const auto path = tempDir.filePath("readme-utf8.txt");
    writeBytes(path, QByteArray("\xEF\xBB\xBF\xE3\x81\x82", 6));

    const auto text = qml_components::FileQuery().readTextFile(path);
    CHECK(text == textWith(QChar(0x3042)));
}

TEST_CASE("FileQuery honors UTF-16 BOM for text files",
          "[FileQuery][encoding]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const auto path = tempDir.filePath("readme-utf16le.txt");
    writeBytes(path, QByteArray("\xFF\xFE\x42\x30", 4));

    const auto text = qml_components::FileQuery().readTextFile(path);
    CHECK(text == textWith(QChar(0x3042)));
}

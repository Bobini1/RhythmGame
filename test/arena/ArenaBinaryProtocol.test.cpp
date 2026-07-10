#include "arena/ArenaBinaryProtocol.h"

#include <catch2/catch_test_macros.hpp>

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>

#include <array>
#include <variant>

namespace {

auto
transferId() -> QByteArray
{
    auto result = QByteArray(16, Qt::Uninitialized);
    for (qsizetype i = 0; i < result.size(); ++i) {
        result[i] = static_cast<char>(i);
    }
    return result;
}

auto
packedHashes(int count) -> QByteArray
{
    auto result = QByteArray(count * arena::ArenaSha256Bytes, '\0');
    for (int i = 0; i < count; ++i) {
        qToBigEndian(static_cast<quint32>(i + 1),
                     result.data() + i * arena::ArenaSha256Bytes + 28);
    }
    return result;
}

auto
validChunk() -> arena::ArenaBinaryChunk
{
    return { .kind = arena::ArenaBinaryKind::AvailabilityReset,
             .transferId = transferId(),
             .chunkIndex = 0x01020304U,
             .packedHashes = packedHashes(2) };
}

auto
encoded(arena::ArenaBinaryChunk chunk) -> QByteArray
{
    const auto result = arena::encodeArenaBinaryChunk(chunk);
    REQUIRE(std::holds_alternative<QByteArray>(result));
    return std::get<QByteArray>(result);
}

auto
failureFor(QByteArray bytes) -> arena::ArenaBinaryFailureCode
{
    const auto result = arena::decodeArenaBinaryChunk(bytes);
    REQUIRE(std::holds_alternative<arena::ArenaBinaryFailure>(result));
    return std::get<arena::ArenaBinaryFailure>(result).code;
}

} // namespace

TEST_CASE("ArenaBinaryProtocol encodes the exact big-endian RGA1 frame",
          "[arena][ArenaBinaryProtocol]")
{
    const auto bytes = encoded(validChunk());
    const auto expectedHeader =
      QByteArray::fromHex("52474131" // RGA1
                          "01"       // version
                          "02"       // availability reset
                          "0000"     // reserved
                          "000102030405060708090a0b0c0d0e0f"
                          "01020304"
                          "00000002");

    REQUIRE(bytes.size() == arena::ArenaBinaryHeaderBytes + 64);
    CHECK(bytes.first(arena::ArenaBinaryHeaderBytes) == expectedHeader);
    CHECK(bytes.sliced(arena::ArenaBinaryHeaderBytes) == packedHashes(2));

    const auto decoded = arena::decodeArenaBinaryChunk(bytes);
    REQUIRE(std::holds_alternative<arena::ArenaBinaryChunk>(decoded));
    CHECK(std::get<arena::ArenaBinaryChunk>(decoded) == validChunk());
}

TEST_CASE("ArenaBinaryProtocol matches the shared server golden corpus",
          "[arena][ArenaBinaryProtocol]")
{
    QFile fixture(QString::fromUtf8(ARENA_BINARY_FIXTURE_PATH));
    REQUIRE(fixture.open(QIODevice::ReadOnly));
    const auto document = QJsonDocument::fromJson(fixture.readAll());
    REQUIRE(document.isObject());
    const auto root = document.object();
    CHECK(root.value(QStringLiteral("fixtureSchema")).toInt() == 1);
    CHECK(root.value(QStringLiteral("formatVersion")).toInt() == 1);

    const auto cases = root.value(QStringLiteral("cases")).toArray();
    REQUIRE(cases.size() == 2);
    for (const auto& value : cases) {
        const auto object = value.toObject();
        const auto chunk = arena::ArenaBinaryChunk{
            .kind = static_cast<arena::ArenaBinaryKind>(
              object.value(QStringLiteral("kind")).toInt()),
            .transferId =
              QByteArray::fromHex(object.value(QStringLiteral("transferIdHex"))
                                    .toString()
                                    .toLatin1()),
            .chunkIndex = static_cast<quint32>(
              object.value(QStringLiteral("chunkIndex")).toDouble()),
            .packedHashes = QByteArray::fromHex(
              object.value(QStringLiteral("hashesHex")).toString().toLatin1()),
        };
        const auto expectedFrame = QByteArray::fromHex(
          object.value(QStringLiteral("frameHex")).toString().toLatin1());
        CHECK(encoded(chunk) == expectedFrame);
        const auto decoded = arena::decodeArenaBinaryChunk(expectedFrame);
        REQUIRE(std::holds_alternative<arena::ArenaBinaryChunk>(decoded));
        CHECK(std::get<arena::ArenaBinaryChunk>(decoded) == chunk);
    }

    const auto invalidFrames =
      root.value(QStringLiteral("invalidFrameHex")).toArray();
    REQUIRE(invalidFrames.size() == 3);
    for (const auto& value : invalidFrames) {
        const auto decoded = arena::decodeArenaBinaryChunk(
          QByteArray::fromHex(value.toString().toLatin1()));
        CHECK(std::holds_alternative<arena::ArenaBinaryFailure>(decoded));
    }
}

TEST_CASE("ArenaBinaryProtocol accepts every kind and structural count edge",
          "[arena][ArenaBinaryProtocol]")
{
    for (const auto kind : { arena::ArenaBinaryKind::InventoryUpload,
                             arena::ArenaBinaryKind::AvailabilityReset,
                             arena::ArenaBinaryKind::AvailabilityAdd,
                             arena::ArenaBinaryKind::AvailabilityRemove }) {
        for (const auto count :
             std::array<qsizetype, 3>{ 0, 1, arena::ArenaMaxHashesPerChunk }) {
            auto chunk = validChunk();
            chunk.kind = kind;
            chunk.packedHashes = packedHashes(count);
            const auto bytes = encoded(chunk);
            CHECK(bytes.size() == arena::ArenaBinaryHeaderBytes +
                                    count * arena::ArenaSha256Bytes);
            const auto decoded = arena::decodeArenaBinaryChunk(bytes);
            REQUIRE(std::holds_alternative<arena::ArenaBinaryChunk>(decoded));
            CHECK(std::get<arena::ArenaBinaryChunk>(decoded) == chunk);
        }
    }
}

TEST_CASE(
  "ArenaBinaryProtocol rejects malformed frames without retaining bytes",
  "[arena][ArenaBinaryProtocol]")
{
    auto bytes = encoded(validChunk());

    CHECK(failureFor(bytes.first(arena::ArenaBinaryHeaderBytes - 1)) ==
          arena::ArenaBinaryFailureCode::FrameTooSmall);
    CHECK(failureFor(QByteArray(arena::ArenaMaxBinaryFrameBytes + 1, '\0')) ==
          arena::ArenaBinaryFailureCode::FrameTooLarge);

    auto malformed = bytes;
    malformed[0] = 'X';
    CHECK(failureFor(malformed) == arena::ArenaBinaryFailureCode::InvalidMagic);

    malformed = bytes;
    malformed[4] = 2;
    CHECK(failureFor(malformed) ==
          arena::ArenaBinaryFailureCode::UnsupportedVersion);

    malformed = bytes;
    malformed[5] = 0;
    CHECK(failureFor(malformed) == arena::ArenaBinaryFailureCode::InvalidKind);
    malformed[5] = 5;
    CHECK(failureFor(malformed) == arena::ArenaBinaryFailureCode::InvalidKind);

    malformed = bytes;
    malformed[6] = 1;
    CHECK(failureFor(malformed) ==
          arena::ArenaBinaryFailureCode::ReservedNotZero);

    malformed = bytes;
    qToBigEndian<quint32>(3, malformed.data() + 28);
    CHECK(failureFor(malformed) ==
          arena::ArenaBinaryFailureCode::PayloadSizeMismatch);

    auto chunk = validChunk();
    chunk.transferId.chop(1);
    CHECK(
      std::get<arena::ArenaBinaryFailure>(arena::encodeArenaBinaryChunk(chunk))
        .code == arena::ArenaBinaryFailureCode::InvalidTransferId);
    chunk = validChunk();
    chunk.packedHashes.chop(1);
    CHECK(
      std::get<arena::ArenaBinaryFailure>(arena::encodeArenaBinaryChunk(chunk))
        .code == arena::ArenaBinaryFailureCode::InvalidPackedHashes);
    chunk.packedHashes = packedHashes(arena::ArenaMaxHashesPerChunk + 1);
    CHECK(
      std::get<arena::ArenaBinaryFailure>(arena::encodeArenaBinaryChunk(chunk))
        .code == arena::ArenaBinaryFailureCode::HashCountOutOfRange);
}

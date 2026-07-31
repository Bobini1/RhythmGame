#pragma once

#include <QByteArray>
#include <QByteArrayView>
#include <QtTypes>

#include <variant>

namespace arena {

inline constexpr qsizetype ArenaSha256Bytes = 32;
inline constexpr qsizetype ArenaTransferIdBytes = 16;
inline constexpr qsizetype ArenaBinaryHeaderBytes = 32;
inline constexpr qsizetype ArenaMaxHashesPerChunk = 2'047;
inline constexpr qsizetype ArenaMaxBinaryFrameBytes = 65'536;
inline constexpr qsizetype ArenaMaxInventoryHashes = 250'000;
inline constexpr qsizetype ArenaMaxInventoryBytes =
  ArenaMaxInventoryHashes * ArenaSha256Bytes;

enum class ArenaBinaryKind : quint8
{
    InventoryUpload = 1,
    AvailabilityReset = 2,
    AvailabilityAdd = 3,
    AvailabilityRemove = 4,
};

enum class ArenaBinaryFailureCode
{
    FrameTooSmall,
    FrameTooLarge,
    InvalidMagic,
    UnsupportedVersion,
    InvalidKind,
    ReservedNotZero,
    InvalidTransferId,
    InvalidPackedHashes,
    HashCountOutOfRange,
    PayloadSizeMismatch,
};

struct ArenaBinaryFailure
{
    ArenaBinaryFailureCode code;
    bool operator==(const ArenaBinaryFailure&) const = default;
};

struct ArenaBinaryChunk
{
    ArenaBinaryKind kind;
    QByteArray transferId;
    quint32 chunkIndex{};
    QByteArray packedHashes;
    bool operator==(const ArenaBinaryChunk&) const = default;
};

using EncodeArenaBinaryResult = std::variant<QByteArray, ArenaBinaryFailure>;
using DecodeArenaBinaryResult =
  std::variant<ArenaBinaryChunk, ArenaBinaryFailure>;

[[nodiscard]] auto
encodeArenaBinaryChunk(const ArenaBinaryChunk& chunk)
  -> EncodeArenaBinaryResult;
[[nodiscard]] auto
decodeArenaBinaryChunk(QByteArrayView frame) -> DecodeArenaBinaryResult;

} // namespace arena

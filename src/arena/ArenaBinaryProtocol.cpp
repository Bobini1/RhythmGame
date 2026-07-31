#include "ArenaBinaryProtocol.h"

#include <QtEndian>

#include <cstring>
#include <optional>

namespace arena {
namespace {

constexpr char BinaryMagic[] = { 'R', 'G', 'A', '1' };
constexpr quint8 BinaryFormatVersion = 1;

auto
failure(ArenaBinaryFailureCode code) -> ArenaBinaryFailure
{
    return { code };
}

auto
decodeKind(quint8 value) -> std::optional<ArenaBinaryKind>
{
    switch (value) {
        case static_cast<quint8>(ArenaBinaryKind::InventoryUpload):
            return ArenaBinaryKind::InventoryUpload;
        case static_cast<quint8>(ArenaBinaryKind::AvailabilityReset):
            return ArenaBinaryKind::AvailabilityReset;
        case static_cast<quint8>(ArenaBinaryKind::AvailabilityAdd):
            return ArenaBinaryKind::AvailabilityAdd;
        case static_cast<quint8>(ArenaBinaryKind::AvailabilityRemove):
            return ArenaBinaryKind::AvailabilityRemove;
        default:
            return std::nullopt;
    }
}

auto
validKind(ArenaBinaryKind kind) -> bool
{
    return decodeKind(static_cast<quint8>(kind)).has_value();
}

} // namespace

auto
encodeArenaBinaryChunk(const ArenaBinaryChunk& chunk) -> EncodeArenaBinaryResult
{
    if (!validKind(chunk.kind)) {
        return failure(ArenaBinaryFailureCode::InvalidKind);
    }
    if (chunk.transferId.size() != ArenaTransferIdBytes) {
        return failure(ArenaBinaryFailureCode::InvalidTransferId);
    }
    if (chunk.packedHashes.size() % ArenaSha256Bytes != 0) {
        return failure(ArenaBinaryFailureCode::InvalidPackedHashes);
    }
    const auto hashCount = chunk.packedHashes.size() / ArenaSha256Bytes;
    if (hashCount > ArenaMaxHashesPerChunk) {
        return failure(ArenaBinaryFailureCode::HashCountOutOfRange);
    }

    auto frame = QByteArray(ArenaBinaryHeaderBytes + chunk.packedHashes.size(),
                            Qt::Uninitialized);
    std::memcpy(frame.data(), BinaryMagic, sizeof(BinaryMagic));
    frame[4] = static_cast<char>(BinaryFormatVersion);
    frame[5] = static_cast<char>(chunk.kind);
    qToBigEndian<quint16>(0, frame.data() + 6);
    std::memcpy(
      frame.data() + 8, chunk.transferId.constData(), ArenaTransferIdBytes);
    qToBigEndian(chunk.chunkIndex, frame.data() + 24);
    qToBigEndian(static_cast<quint32>(hashCount), frame.data() + 28);
    if (!chunk.packedHashes.isEmpty()) {
        std::memcpy(frame.data() + ArenaBinaryHeaderBytes,
                    chunk.packedHashes.constData(),
                    static_cast<size_t>(chunk.packedHashes.size()));
    }
    return frame;
}

auto
decodeArenaBinaryChunk(QByteArrayView frame) -> DecodeArenaBinaryResult
{
    if (frame.size() < ArenaBinaryHeaderBytes) {
        return failure(ArenaBinaryFailureCode::FrameTooSmall);
    }
    if (frame.size() > ArenaMaxBinaryFrameBytes) {
        return failure(ArenaBinaryFailureCode::FrameTooLarge);
    }
    if (std::memcmp(frame.data(), BinaryMagic, sizeof(BinaryMagic)) != 0) {
        return failure(ArenaBinaryFailureCode::InvalidMagic);
    }
    if (static_cast<quint8>(frame[4]) != BinaryFormatVersion) {
        return failure(ArenaBinaryFailureCode::UnsupportedVersion);
    }
    const auto kind = decodeKind(static_cast<quint8>(frame[5]));
    if (!kind) {
        return failure(ArenaBinaryFailureCode::InvalidKind);
    }
    if (qFromBigEndian<quint16>(frame.data() + 6) != 0) {
        return failure(ArenaBinaryFailureCode::ReservedNotZero);
    }
    const auto hashCount = qFromBigEndian<quint32>(frame.data() + 28);
    if (hashCount > ArenaMaxHashesPerChunk) {
        return failure(ArenaBinaryFailureCode::HashCountOutOfRange);
    }
    const auto expectedSize =
      ArenaBinaryHeaderBytes +
      static_cast<qsizetype>(hashCount) * ArenaSha256Bytes;
    if (frame.size() != expectedSize) {
        return failure(ArenaBinaryFailureCode::PayloadSizeMismatch);
    }

    return ArenaBinaryChunk{
        .kind = *kind,
        .transferId = QByteArray(frame.data() + 8, ArenaTransferIdBytes),
        .chunkIndex = qFromBigEndian<quint32>(frame.data() + 24),
        .packedHashes = QByteArray(frame.data() + ArenaBinaryHeaderBytes,
                                   frame.size() - ArenaBinaryHeaderBytes),
    };
}

} // namespace arena

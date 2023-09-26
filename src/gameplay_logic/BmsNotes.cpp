//
// Created by bobini on 24.08.23.
//

#include <zstd.h>
#include "BmsNotes.h"
#include <QIODevice>

namespace gameplay_logic {
auto
BmsNotes::getVisibleNotes() const -> const QVector<QVector<Note>>&
{
    return visibleNotes;
}
auto
BmsNotes::getInvisibleNotes() const -> const QVector<QVector<Note>>&
{
    return invisibleNotes;
}
BmsNotes::BmsNotes(QVector<QVector<Note>> visibleNotes,
                   QVector<QVector<Note>> invisibleNotes,
                   QVector<BpmChange> bpmChanges,
                   QVector<Time> barLines,
                   QObject* parent)
  : QObject(parent)
  , visibleNotes(std::move(visibleNotes))
  , invisibleNotes(std::move(invisibleNotes))
  , bpmChanges(std::move(bpmChanges))
  , barLines(std::move(barLines))
{
}
auto
BmsNotes::getBarLines() const -> const QVector<Time>&
{
    return barLines;
}
auto
BmsNotes::getBpmChanges() const -> const QVector<BpmChange>&
{
    return bpmChanges;
}
auto
BmsNotes::load(const std::string& serializedData) -> std::unique_ptr<BmsNotes>
{
    using namespace std::string_literals;
    auto decompressedBuffer = QByteArray{};
    auto decompressedSize =
      ZSTD_getFrameContentSize(serializedData.data(), serializedData.size());
    if (ZSTD_isError(decompressedSize)) {
        const auto* error = ZSTD_getErrorName(decompressedSize);
        throw std::runtime_error{ "Failed to decompress chart data: "s +
                                  error };
    }
    decompressedBuffer.resize(decompressedSize);
    decompressedSize = ZSTD_decompress(decompressedBuffer.data(),
                                       decompressedBuffer.size(),
                                       serializedData.data(),
                                       serializedData.size());
    if (ZSTD_isError(decompressedSize)) {
        // what error?
        const auto* error = ZSTD_getErrorName(decompressedSize);
        throw std::runtime_error{ "Failed to decompress chart data: "s +
                                  error };
    }
    decompressedBuffer.resize(decompressedSize);
    auto noteData = std::make_unique<BmsNotes>();
    auto stream = QDataStream{ &decompressedBuffer, QIODevice::ReadOnly };
    stream >> *noteData;
    return noteData;
}
auto
BmsNotes::serialize() const -> std::string
{
    auto buffer = QByteArray{};
    auto stream = QDataStream{ &buffer, QIODevice::WriteOnly };
    stream << *this;
    // compress it!
    auto compressedBuffer = QByteArray{};
    compressedBuffer.resize(ZSTD_compressBound(buffer.size()));
    auto compressedSize = ZSTD_compress(compressedBuffer.data(),
                                        compressedBuffer.size(),
                                        buffer.data(),
                                        buffer.size(),
                                        1);
    if (ZSTD_isError(compressedSize)) {
        throw std::runtime_error{ "Failed to compress chart data" };
    }
    return compressedBuffer.left(compressedSize).toStdString();
}
auto
BmsNotes::save(db::SqliteCppDb& db, const support::Sha256& sha256) const -> void
{
    auto serializedData = serialize();
    static thread_local auto insertQuery = db.createStatement(
      "INSERT INTO note_data (sha256, note_data) VALUES (:sha256, :note_data)");
    insertQuery.bind(":sha256", sha256);
    insertQuery.bind(":note_data", serializedData);
    insertQuery.execute();
    insertQuery.reset();
}
} // namespace gameplay_logic

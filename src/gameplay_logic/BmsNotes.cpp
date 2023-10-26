//
// Created by bobini on 24.08.23.
//

#include <zstd.h>
#include "BmsNotes.h"
#include "support/Compress.h"
#include <QIODevice>
#include <QVariant>

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
BmsNotes::load(db::SqliteCppDb& db, const support::Sha256& sha256)
  -> std::unique_ptr<BmsNotes>
{
    using namespace std::string_literals;
    auto statement = db.createStatement(
      "SELECT note_data FROM note_data WHERE sha256 = :sha256");
    statement.bind(":sha256", sha256);
    auto result = statement.executeAndGet<std::string>();
    if (!result.has_value()) {
        throw std::runtime_error{ "Failed to load note data" };
    }
    auto serializedData = QByteArray::fromStdString(*result);
    auto decompressedBuffer = support::decompress(serializedData);
    auto noteData = std::make_unique<BmsNotes>();
    auto stream = QDataStream{ &decompressedBuffer, QIODevice::ReadOnly };
    stream >> *noteData;
    return noteData;
}
auto
BmsNotes::serialize() const -> QByteArray
{
    auto buffer = QByteArray{};
    auto stream = QDataStream{ &buffer, QIODevice::WriteOnly };
    stream << *this;
    return support::compress(buffer);
}
auto
BmsNotes::save(db::SqliteCppDb& db, const support::Sha256& sha256) const -> void
{
    auto serializedData = serialize();
    static thread_local auto insertQuery =
      db.createStatement("INSERT OR REPLACE INTO note_data (sha256, note_data) "
                         "VALUES (:sha256, :note_data)");
    insertQuery.bind(":sha256", sha256);
    insertQuery.bind(
      ":note_data", serializedData.data(), serializedData.size());
    insertQuery.execute();
    insertQuery.reset();
}
auto
BmsNotes::getVisibleNoteAt(int column, int row) const -> QVariant
{
    if (column < 0 || column >= visibleNotes.size()) {
        return {};
    }
    if (row < 0 || row >= visibleNotes[column].size()) {
        return {};
    }
    return QVariant::fromValue(visibleNotes[column][row]);
}
auto
BmsNotes::getInvisibleNoteAt(int column, int row) const -> QVariant
{
    if (column < 0 || column >= invisibleNotes.size()) {
        return {};
    }
    if (row < 0 || row >= invisibleNotes[column].size()) {
        return {};
    }
    return QVariant::fromValue(invisibleNotes[column][row]);
}
} // namespace gameplay_logic

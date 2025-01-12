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
BmsNotes::getVisibleNotes() -> QVector<QVector<Note>>&
{
    return visibleNotes;
}
auto
BmsNotes::getInvisibleNotes() -> QVector<QVector<Note>>&
{
    return invisibleNotes;
}
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
BmsNotes::
BmsNotes(QVector<QVector<Note>> visibleNotes,
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
    auto noteData = std::make_unique<BmsNotes>();
    support::decompress(serializedData, *noteData);
    return noteData;
}
auto
BmsNotes::serialize() const -> QByteArray
{
    return support::compress(*this);
}
auto
BmsNotes::save(db::SqliteCppDb& db, const support::Sha256& sha256) const -> void
{
    auto serializedData = serialize();
    auto insertQuery =
      db.createStatement("INSERT OR REPLACE INTO note_data (sha256, note_data) "
                         "VALUES (:sha256, :note_data)");
    insertQuery.bind(":sha256", sha256);
    insertQuery.bind(
      ":note_data", serializedData.data(), serializedData.size());
    insertQuery.execute();
    insertQuery.reset();
}
} // namespace gameplay_logic

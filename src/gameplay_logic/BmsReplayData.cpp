//
// Created by bobini on 30.09.23.
//

#include "BmsReplayData.h"
#include "support/Compress.h"
#include <QIODevice>
#include <utility>

namespace gameplay_logic {
BmsReplayData::BmsReplayData(QList<HitEvent> hitEvents,
                             QString guid,
                             QObject* parent)
  : QObject(parent)
  , hitEvents(std::move(hitEvents))
  , guid(std::move(guid))
{
}
auto
BmsReplayData::getHitEvents() -> const QList<HitEvent>&
{
    return hitEvents;
}
auto
BmsReplayData::getGuid() const -> QString
{
    return guid;
}
void
BmsReplayData::save(db::SqliteCppDb& db) const
{
    auto statement =
      db.createStatement("INSERT OR IGNORE INTO replay_data (score_guid, "
                         "replay_data) VALUES (?, ?)");
    auto compressed = support::compress(hitEvents);
    statement.bind(1, guid.toStdString());
    statement.bind(2, compressed.data(), compressed.size());
    statement.execute();
}
auto
BmsReplayData::load(db::SqliteCppDb& db, QString guid)
  -> std::unique_ptr<BmsReplayData>
{
    auto statement = db.createStatement(
      "SELECT replay_data FROM replay_data WHERE score_guid = ?");
    statement.bind(1, guid.toStdString());
    const auto result = statement.executeAndGet<std::string>();
    if (!result.has_value()) {
        return nullptr;
    }
    const auto data = QByteArray::fromStdString(*result);
    auto hitEvents = QList<HitEvent>{};
    support::decompress(data, hitEvents);
    return std::make_unique<BmsReplayData>(std::move(hitEvents), guid);
}
} // namespace gameplay_logic
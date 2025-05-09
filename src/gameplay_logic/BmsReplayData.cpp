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
BmsReplayData::load(const DTO& dto) -> std::unique_ptr<BmsReplayData>
{
    const auto data = QByteArray::fromStdString(dto.hitEvents);
    auto hitEvents = QList<HitEvent>{};
    support::decompress(data, hitEvents);
    return std::make_unique<BmsReplayData>(std::move(hitEvents),
                                           QString::fromStdString(dto.guid));
}
} // namespace gameplay_logic
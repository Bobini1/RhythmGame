//
// Created by bobini on 30.09.23.
//

#include "BmsReplayData.h"
#include "support/Compress.h"
#include <QIODevice>
#include <utility>

namespace gameplay_logic {
BmsReplayData::BmsReplayData(QList<HitEvent> hitEvents, QObject* parent)
  : QObject(parent)
  , hitEvents(std::move(hitEvents))
{
}
auto
BmsReplayData::getHitEvents() -> const QList<HitEvent>&
{
    return hitEvents;
}
auto
operator<<(QDataStream& stream, const BmsReplayData& data) -> QDataStream&
{
    stream << data.hitEvents;
    return stream;
}
auto
operator>>(QDataStream& stream, BmsReplayData& data) -> QDataStream&
{
    return stream >> data.hitEvents;
}
void
BmsReplayData::save(db::SqliteCppDb& db, int64_t scoreId)
{
    auto statement = db.createStatement(
      "INSERT INTO replay_data (score_id, replay_data) VALUES (?, ?)");
    auto compressed = support::compress(*this);
    statement.bind(1, scoreId);
    statement.bind(2, compressed.data(), compressed.size());
    statement.execute();
}
auto
BmsReplayData::load(db::SqliteCppDb& db, int64_t scoreId)
  -> std::unique_ptr<BmsReplayData>
{
    auto statement = db.createStatement(
      "SELECT replay_data FROM replay_data WHERE score_id = ?");
    statement.bind(1, scoreId);
    auto result = statement.executeAndGet<std::string>();
    if (!result.has_value()) {
        return nullptr;
    }
    auto data = QByteArray::fromStdString(*result);
    auto replayData = std::make_unique<BmsReplayData>();
    support::decompress(data, *replayData);
    return replayData;
}
} // namespace gameplay_logic
//
// Created by bobini on 30.09.23.
//

#include "BmsReplayData.h"
#include "support/Compress.h"
#include <QIODevice>

namespace gameplay_logic {
BmsReplayData::BmsReplayData(QList<HitEvent> misses,
                             QList<HitEvent> hitsWithPoints,
                             QList<HitEvent> hitsWithoutPoints)
  : misses(std::move(misses))
  , hitsWithPoints(std::move(hitsWithPoints))
  , hitsWithoutPoints(std::move(hitsWithoutPoints))
{
}
auto
BmsReplayData::getMisses() const -> QList<HitEvent>
{
    return misses;
}
auto
BmsReplayData::getHitsWithPoints() const -> QList<HitEvent>
{
    return hitsWithPoints;
}
auto
BmsReplayData::getHitsWithoutPoints() const -> QList<HitEvent>
{
    return hitsWithoutPoints;
}
auto
operator<<(QDataStream& stream, const BmsReplayData& data) -> QDataStream&
{
    stream << data.misses;
    stream << data.hitsWithPoints;
    stream << data.hitsWithoutPoints;
    return stream;
}
auto
operator>>(QDataStream& stream, BmsReplayData& data) -> QDataStream&
{
    return stream >> data.misses >> data.hitsWithPoints >>
           data.hitsWithoutPoints;
}
void
BmsReplayData::save(db::SqliteCppDb& db, int64_t scoreId)
{
    auto statement = db.createStatement(
      "INSERT INTO replay_data (score_id, replay_data) VALUES (?, ?)");
    auto data = QByteArray{};
    auto stream = QDataStream(&data, QIODevice::WriteOnly);
    stream << *this;
    auto compressed = support::compress(data);
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
    auto decompressed = support::decompress(data);
    auto stream = QDataStream(&decompressed, QIODevice::ReadOnly);
    auto replayData = std::make_unique<BmsReplayData>();
    stream >> *replayData;
    return replayData;
}
} // namespace gameplay_logic
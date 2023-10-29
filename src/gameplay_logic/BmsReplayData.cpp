//
// Created by bobini on 30.09.23.
//

#include "BmsReplayData.h"
#include "support/Compress.h"
#include <QIODevice>
#include <utility>

namespace gameplay_logic {
BmsReplayData::BmsReplayData(QList<HitEvent> misses,
                             QList<HitEvent> hitsWithPoints,
                             QList<HitEvent> hitsWithoutPoints,
                             QList<HitEvent> releasesWithoutPoints,
                             QList<MineHit> mineHits,
                             QList<HitEvent> lnEndHits,
                             QList<HitEvent> lnEndMisses,
                             QList<HitEvent> lnEndSkips)
  : misses(std::move(misses))
  , hitsWithPoints(std::move(hitsWithPoints))
  , hitsWithoutPoints(std::move(hitsWithoutPoints))
  , releasesWithoutPoints(std::move(releasesWithoutPoints))
  , mineHits(std::move(mineHits))
  , lnEndHits(std::move(lnEndHits))
  , lnEndMisses(std::move(lnEndMisses))
  , lnEndSkips(std::move(lnEndSkips))
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
    stream << data.releasesWithoutPoints;
    stream << data.mineHits;
    stream << data.lnEndHits;
    stream << data.lnEndMisses;
    stream << data.lnEndSkips;
    return stream;
}
auto
operator>>(QDataStream& stream, BmsReplayData& data) -> QDataStream&
{
    return stream >> data.misses >> data.hitsWithPoints >>
           data.hitsWithoutPoints >> data.releasesWithoutPoints >>
           data.mineHits >> data.lnEndHits >> data.lnEndMisses >>
           data.lnEndSkips;
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
auto
BmsReplayData::getReleasesWithoutPoints() const -> QList<HitEvent>
{
    return releasesWithoutPoints;
}
auto
BmsReplayData::getMineHits() const -> QList<MineHit>
{
    return mineHits;
}
auto
BmsReplayData::getLnEndHits() const -> QList<HitEvent>
{
    return lnEndHits;
}
auto
BmsReplayData::getLnEndMisses() const -> QList<HitEvent>
{
    return lnEndMisses;
}
auto
BmsReplayData::getLnEndSkips() const -> QList<HitEvent>
{
    return lnEndSkips;
}
} // namespace gameplay_logic
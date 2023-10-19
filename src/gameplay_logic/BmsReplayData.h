//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_BMSREPLAYDATA_H
#define RHYTHMGAME_BMSREPLAYDATA_H

#include "HitEvent.h"
#include "gameplay_logic/rules/BmsGauge.h"
#include "db/SqliteCppDb.h"
#include "MineHit.h"
namespace gameplay_logic {

class BmsReplayData : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<HitEvent> misses READ getMisses CONSTANT)
    Q_PROPERTY(QList<HitEvent> hitsWithPoints READ getHitsWithPoints CONSTANT)
    Q_PROPERTY(
      QList<HitEvent> hitsWithoutPoints READ getHitsWithoutPoints CONSTANT)
    Q_PROPERTY(QList<HitEvent> releasesWithoutPoints READ
                 getReleasesWithoutPoints CONSTANT)
    Q_PROPERTY(QList<MineHit> mineHits READ getMineHits CONSTANT)
    Q_PROPERTY(QList<HitEvent> lnEndHits READ getLnEndHits CONSTANT)
    Q_PROPERTY(QList<HitEvent> lnEndMisses READ getLnEndMisses CONSTANT)
    Q_PROPERTY(QList<HitEvent> lnEndSkips READ getLnEndSkips CONSTANT)

    QList<HitEvent> misses;
    QList<HitEvent> hitsWithPoints;
    QList<HitEvent> hitsWithoutPoints;
    QList<HitEvent> releasesWithoutPoints;
    QList<MineHit> mineHits;
    QList<HitEvent> lnEndHits;
    QList<HitEvent> lnEndMisses;
    QList<HitEvent> lnEndSkips;

  public:
    BmsReplayData(QList<HitEvent> misses,
                  QList<HitEvent> hitsWithPoints,
                  QList<HitEvent> hitsWithoutPoints,
                  QList<HitEvent> releasesWithoutPoints,
                  QList<MineHit> mineHits,
                  QList<HitEvent> lnEndHits,
                  QList<HitEvent> lnEndMisses,
                  QList<HitEvent> lnEndSkips);
    BmsReplayData() = default;
    auto getMisses() const -> QList<HitEvent>;
    auto getHitsWithPoints() const -> QList<HitEvent>;
    auto getHitsWithoutPoints() const -> QList<HitEvent>;
    auto getReleasesWithoutPoints() const -> QList<HitEvent>;
    auto getMineHits() const -> QList<MineHit>;
    auto getLnEndHits() const -> QList<HitEvent>;
    auto getLnEndMisses() const -> QList<HitEvent>;
    auto getLnEndSkips() const -> QList<HitEvent>;

    friend auto operator<<(QDataStream& stream, const BmsReplayData& data)
      -> QDataStream&;
    friend auto operator>>(QDataStream& stream, BmsReplayData& data)
      -> QDataStream&;

    void save(db::SqliteCppDb& db, int64_t scoreId);
    static auto load(db::SqliteCppDb& db, int64_t scoreId)
      -> std::unique_ptr<BmsReplayData>;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSREPLAYDATA_H

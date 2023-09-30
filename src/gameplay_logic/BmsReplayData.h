//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_BMSREPLAYDATA_H
#define RHYTHMGAME_BMSREPLAYDATA_H

#include "Miss.h"
#include "Tap.h"
#include "gameplay_logic/rules/BmsGauge.h"
#include "db/SqliteCppDb.h"
namespace gameplay_logic {

class BmsReplayData : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<Miss> misses READ getMisses CONSTANT)
    Q_PROPERTY(QList<Tap> hitsWithPoints READ getHitsWithPoints CONSTANT)
    Q_PROPERTY(QList<Tap> hitsWithoutPoints READ getHitsWithoutPoints CONSTANT)

    QList<Miss> misses;
    QList<Tap> hitsWithPoints;
    QList<Tap> hitsWithoutPoints;

  public:
    BmsReplayData(QList<Miss> misses,
                  QList<Tap> hitsWithPoints,
                  QList<Tap> hitsWithoutPoints);
    BmsReplayData() = default;
    auto getMisses() const -> QList<Miss>;
    auto getHitsWithPoints() const -> QList<Tap>;
    auto getHitsWithoutPoints() const -> QList<Tap>;

    friend auto operator<<(QDataStream& stream, const BmsReplayData& data)
      -> QDataStream&;
    friend auto operator>>(QDataStream& stream, BmsReplayData& data)
      -> QDataStream&;

    void save(db::SqliteCppDb& db, int scoreId);
    static auto load(db::SqliteCppDb& db, int scoreId)
      -> std::unique_ptr<BmsReplayData>;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSREPLAYDATA_H

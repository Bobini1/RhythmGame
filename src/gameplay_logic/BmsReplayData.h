//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_BMSREPLAYDATA_H
#define RHYTHMGAME_BMSREPLAYDATA_H

#include "HitEvent.h"
#include "gameplay_logic/rules/BmsGauge.h"
#include "db/SqliteCppDb.h"
namespace gameplay_logic {

class BmsReplayData : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<HitEvent> hitEvents READ getHitEvents CONSTANT)

    QList<HitEvent> hitEvents;

  public:
    explicit BmsReplayData(QList<HitEvent> hitEvents,
                           QObject* parent = nullptr);
    BmsReplayData() = default;
    auto getHitEvents() -> const QList<HitEvent>&;

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

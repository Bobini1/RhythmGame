//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_BMSREPLAYDATA_H
#define RHYTHMGAME_BMSREPLAYDATA_H

#include "HitEvent.h"
#include "gameplay_logic/rules/BmsGauge.h"
#include "db/SqliteCppDb.h"
namespace gameplay_logic {

class BmsReplayData final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<HitEvent> hitEvents READ getHitEvents CONSTANT)
    Q_PROPERTY(QString guid READ getGuid CONSTANT)

    QList<HitEvent> hitEvents;
    QString guid;

  public:
    explicit BmsReplayData(QList<HitEvent> hitEvents,
                           QString guid,
                           QObject* parent = nullptr);
    auto getHitEvents() -> const QList<HitEvent>&;
    auto getGuid() const -> QString;

    void save(db::SqliteCppDb& db) const;
    static auto load(db::SqliteCppDb& db, QString guid)
      -> std::unique_ptr<BmsReplayData>;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSREPLAYDATA_H

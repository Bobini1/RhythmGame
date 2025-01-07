//
// Created by bobini on 07.10.23.
//

#ifndef RHYTHMGAME_SCOREDB_H
#define RHYTHMGAME_SCOREDB_H

#include <functional>
#include "db/SqliteCppDb.h"
#include "gameplay_logic/BmsResult.h"
#include "gameplay_logic/BmsGaugeHistory.h"
#include "gameplay_logic/BmsReplayData.h"
namespace qml_components {
class ScoreDb : public QObject
{
    Q_OBJECT

    db::SqliteCppDb* scoreDb;

  public:
    explicit ScoreDb(db::SqliteCppDb* scoreDb);
    Q_INVOKABLE QList<gameplay_logic::BmsResult*> getScoresForChart(
      QString sha256);
    Q_INVOKABLE gameplay_logic::BmsGaugeHistory* getGaugeHistory(
      int64_t scoreId);
    Q_INVOKABLE gameplay_logic::BmsReplayData* getReplayData(int64_t scoreId);
};
} // namespace qml_components

#endif // RHYTHMGAME_SCOREDB_H

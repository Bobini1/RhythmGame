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
    Q_INVOKABLE QList<gameplay_logic::BmsResult*> getScoresForMd5(
      const QString& md5) const;
    Q_INVOKABLE gameplay_logic::BmsGaugeHistory* getGaugeHistory(
      const QString& guid) const;
    Q_INVOKABLE gameplay_logic::BmsReplayData* getReplayData(
      const QString& guid) const;
    Q_INVOKABLE int getTotalScoreCount() const;
};
} // namespace qml_components

#endif // RHYTHMGAME_SCOREDB_H

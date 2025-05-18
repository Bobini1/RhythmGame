//
// Created by bobini on 07.10.23.
//

#ifndef RHYTHMGAME_SCOREDB_H
#define RHYTHMGAME_SCOREDB_H

#include <functional>
#include <QIfPendingReply>
#include "db/SqliteCppDb.h"
#include "gameplay_logic/BmsScore.h"
namespace qml_components {
class ScoreDb final : public QObject
{
    Q_OBJECT

    db::SqliteCppDb* scoreDb;

  public:
    explicit ScoreDb(db::SqliteCppDb* scoreDb);
    Q_INVOKABLE QIfPendingReply<QList<QList<gameplay_logic::BmsScore*>>> getScoresForMd5(
      const QList<QString>& md5s) const;
    Q_INVOKABLE int getTotalScoreCount() const;
};
} // namespace qml_components

#endif // RHYTHMGAME_SCOREDB_H

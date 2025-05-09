//
// Created by bobini on 07.10.23.
//

#include "ScoreDb.h"

#include <QFutureWatcher>
#include <qtconcurrentrun.h>

namespace qml_components {

ScoreDb::ScoreDb(db::SqliteCppDb* scoreDb)
  : scoreDb(scoreDb)
{
}
auto
ScoreDb::getScoresForMd5(const QList<QString>& md5s) const
  -> QIfPendingReply<QList<QList<gameplay_logic::BmsScore*>>>
{
    auto reply = QIfPendingReply<QList<QList<gameplay_logic::BmsScore*>>>{};
    QThreadPool::globalInstance()->start([this, md5s, reply]() mutable {
        auto statement = scoreDb->createStatement("SELECT * "
                                                  "FROM score "
                                                  "JOIN replay_data ON score.guid = replay_data.score_guid "
                                                  "JOIN gauge_history ON score.guid = gauge_history.score_guid "
                                                  "WHERE score.md5 IN (" +
                                                  QString("?, ").repeated(md5s.size()).chopped(2).toStdString() + ")");

        for (int i = 0; i < md5s.size(); ++i) {
            statement.bind(i + 1, md5s[i].toStdString());
        }

        const auto result =
          statement.executeAndGetAll<std::tuple<gameplay_logic::BmsResult::BmsResultDto,
        gameplay_logic::BmsReplayData::DTO, gameplay_logic::BmsGaugeHistory::DTO>>();

        QHash<QString, QList<gameplay_logic::BmsScore*>> groupedScores;
        for (const auto& row : result) {
            auto md5 = QString::fromStdString(std::get<0>(row).md5);
            groupedScores[md5].append(new gameplay_logic::BmsScore{
                gameplay_logic::BmsResult::load(std::get<0>(row)),
                gameplay_logic::BmsReplayData::load(std::get<1>(row)),
                gameplay_logic::BmsGaugeHistory::load(std::get<2>(row))});

        }

        QList<QList<gameplay_logic::BmsScore*>> scores;
        for (const auto& md5 : md5s) {
            scores.append(groupedScores.value(md5, {}));
        }
        reply.setSuccess(std::move(scores));
    });
    return reply;
}
auto
ScoreDb::getTotalScoreCount() const -> int
{
    auto statement = scoreDb->createStatement("SELECT COUNT(*) FROM score");
    return statement.executeAndGet<int>().value_or(0);
}
} // namespace qml_components
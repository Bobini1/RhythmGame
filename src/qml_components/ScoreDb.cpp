//
// Created by bobini on 07.10.23.
//

#include "ScoreDb.h"

#include "gameplay_logic/BmsScoreCourse.h"
#include "support/PathToUtfString.h"

#include <QFutureWatcher>
#include <qcoreapplication.h>
#include <qtconcurrentrun.h>
#include <spdlog/spdlog.h>

namespace qml_components {

constexpr int maxVariables = 999;
auto
ScoreDb::getScoresForMd5Impl(QList<QString> md5s) const -> ScoreQueryResult
{
    auto uniqueMd5s = QSet<QString>{};
    for (const auto& md5 : md5s) {
        uniqueMd5s.insert(md5.toUpper());
    }
    const auto md5sToFetch = uniqueMd5s.values();
    std::vector<std::tuple<gameplay_logic::BmsResult::DTO,
                           gameplay_logic::BmsReplayData::DTO,
                           gameplay_logic::BmsGaugeHistory::DTO>>
      allResults;

    for (int i = 0; i < md5sToFetch.size(); i += maxVariables) {
        auto chunk = md5sToFetch.mid(i, maxVariables);
        auto statement = scoreDb->createStatement(
          "SELECT * "
          "FROM score "
          "JOIN replay_data ON score.guid = replay_data.score_guid "
          "JOIN gauge_history ON score.guid = gauge_history.score_guid "
          "WHERE score.md5 IN (" +
          QString("?, ").repeated(chunk.size()).chopped(2).toStdString() +
          ") ORDER BY score.unix_timestamp DESC");

        for (int j = 0; j < chunk.size(); ++j) {
            statement.bind(j + 1, chunk[j].toStdString());
        }

        const auto result = statement.executeAndGetAll<
          std::tuple<gameplay_logic::BmsResult::DTO,
                     gameplay_logic::BmsReplayData::DTO,
                     gameplay_logic::BmsGaugeHistory::DTO>>();
        allResults.append_range(result);
    }

    QMap<QString, QVariantList> groupedScores;
    auto* mainThread = QCoreApplication::instance()->thread();
    for (const auto& row : allResults) {
        auto md5 = QString::fromStdString(std::get<0>(row).md5);
        auto* score = new gameplay_logic::BmsScore{
            gameplay_logic::BmsResult::load(std::get<0>(row)),
            gameplay_logic::BmsReplayData::load(std::get<1>(row)),
            gameplay_logic::BmsGaugeHistory::load(std::get<2>(row))
        };
        score->moveToThread(mainThread);
        groupedScores[md5].append(QVariant::fromValue(score));
    }
    // get the number of md5s that were not found
    auto totalMd5s = md5s.size();
    auto foundMd5s = groupedScores.size();
    auto notFoundMd5s = totalMd5s - foundMd5s;

    auto groupedVariantScores = QVariantMap{};
    for (const auto& [md5, scores] : groupedScores.asKeyValueRange()) {
        groupedVariantScores[md5] = QVariant::fromValue(std::move(scores));
    }

    ScoreQueryResult result;
    result.scores = std::move(groupedVariantScores);
    result.unplayed = notFoundMd5s;

    return result;
}
ScoreDb::ScoreDb(db::SqliteCppDb* scoreDb)
  : scoreDb(scoreDb)
{
}
auto
ScoreDb::getScoresForMd5(const QList<QString>& md5s) const
  -> QIfPendingReply<ScoreQueryResult>
{
    auto reply = QIfPendingReply<ScoreQueryResult>{};
    if (md5s.isEmpty()) {
        reply.setSuccess(ScoreQueryResult{});
        return reply;
    }
    auto token = stopSource.get_token();

    threadPool.start([this, md5s, token, reply]() mutable {
        try {
            auto result = getScoresForMd5Impl(md5s);

            QMetaObject::invokeMethod(
              QCoreApplication::instance(),
              [reply, token, result = std::move(result)]() mutable {
                  if (token.stop_requested()) {
                      reply.setFailed();
                      return;
                  }
                  reply.setSuccess(result);
              },
              Qt::QueuedConnection);
        } catch (const std::exception& e) {
            QMetaObject::invokeMethod(
              QCoreApplication::instance(),
              [reply, e]() mutable {
                  spdlog::error("Error in getScoresForMd5: {}", e.what());
                  reply.setFailed();
              },
              Qt::QueuedConnection);
        }
    });
    return reply;
}
auto
ScoreDb::getScores(const QString& folder) const
  -> QIfPendingReply<ScoreQueryResult>
{
    auto reply = QIfPendingReply<ScoreQueryResult>{};
    auto token = stopSource.get_token();
    threadPool.start([reply, folder, token, this] {
        try {
            // Query to count unplayed charts
            auto countQuery = scoreDb->createStatement(
              "SELECT COUNT(*) "
              "FROM song_db.charts "
              "WHERE path LIKE ? || '%' "
              "AND NOT EXISTS ("
              "  SELECT 1 FROM score WHERE score.md5 = song_db.charts.md5"
              ")");
            countQuery.bind(1, folder.toStdString());
            auto unplayedCount = countQuery.executeAndGet<int>().value_or(0);

            auto* mainThread = QCoreApplication::instance()->thread();
            auto query = scoreDb->createStatement(
              "SELECT score.*, replay_data.*, gauge_history.* "
              "FROM score "
              "JOIN replay_data ON score.guid = replay_data.score_guid "
              "JOIN gauge_history ON score.guid = gauge_history.score_guid "
              "JOIN song_db.charts ON score.md5 = song_db.charts.md5 "
              "WHERE song_db.charts.path LIKE ? || '%' ");
            query.bind(1, folder.toStdString());
            const auto result = query.executeAndGetAll<
              std::tuple<gameplay_logic::BmsResult::DTO,
                         gameplay_logic::BmsReplayData::DTO,
                         gameplay_logic::BmsGaugeHistory::DTO>>();
            QMap<QString, QVariantList> groupedScores;
            for (const auto& row : result) {
                auto md5 = QString::fromStdString(std::get<0>(row).md5);
                auto* score = new gameplay_logic::BmsScore{
                    gameplay_logic::BmsResult::load(std::get<0>(row)),
                    gameplay_logic::BmsReplayData::load(std::get<1>(row)),
                    gameplay_logic::BmsGaugeHistory::load(std::get<2>(row))
                };
                score->moveToThread(mainThread);
                groupedScores[md5].append(QVariant::fromValue(score));
            }
            auto groupedVariantScores = QVariantMap{};
            for (const auto& [md5, scores] : groupedScores.asKeyValueRange()) {
                groupedVariantScores[md5] =
                  QVariant::fromValue(std::move(scores));
            }
            QMetaObject::invokeMethod(
              QCoreApplication::instance(),
              [reply,
               token,
               unplayedCount,
               groupedVariantScores =
                 std::move(groupedVariantScores)]() mutable {
                  if (token.stop_requested()) {
                      reply.setFailed();
                      return;
                  }
                  ScoreQueryResult result;
                  result.unplayed = unplayedCount;
                  result.scores = std::move(groupedVariantScores);
                  reply.setSuccess(result);
              },
              Qt::QueuedConnection);
        } catch (std::exception& e) {
            QMetaObject::invokeMethod(
              QCoreApplication::instance(),
              [reply, e]() mutable {
                  spdlog::error("Error in getScores: {}", e.what());
                  reply.setFailed();
              },
              Qt::QueuedConnection);
        }
    });
    return reply;
}
auto
ScoreDb::getScores(const resource_managers::Table& table) const
  -> QIfPendingReply<TableScoreQueryResult>
{
    auto reply = QIfPendingReply<TableScoreQueryResult>{};
    auto token = stopSource.get_token();

    threadPool.start([this, table, token, reply]() mutable {
        try {
            auto md5s = QStringList{};
            for (const auto& level : table.levels) {
                for (const auto& entry : level.entries) {
                    md5s.append(entry.md5);
                }
            }

            auto scores = getScoresForMd5Impl(md5s);

            auto courseIds = QStringList{};
            for (const auto& courseList : table.courses) {
                for (const auto& course : courseList) {
                    courseIds.append(course.getIdentifier());
                }
            }

            auto allCourseResults =
              std::vector<gameplay_logic::BmsScoreCourse::DTO>{};
            for (int i = 0; i < courseIds.size(); i += maxVariables) {
                auto chunk = courseIds.mid(i, maxVariables);
                auto statement = scoreDb->createStatement(
                  "SELECT * "
                  "FROM score_course "
                  "WHERE score_course.identifier IN (" +
                  QString("?, ")
                    .repeated(chunk.size())
                    .chopped(2)
                    .toStdString() +
                  ") ORDER BY score_course.unix_timestamp DESC");

                for (int j = 0; j < chunk.size(); ++j) {
                    statement.bind(j + 1, chunk[j].toStdString());
                }

                const auto result =
                  statement
                    .executeAndGetAll<gameplay_logic::BmsScoreCourse::DTO>();
                allCourseResults.append_range(result);
            }

            auto scoreGuids = QList<QString>{};
            for (const auto& course : allCourseResults) {
                const auto& guidsForCourse = course.scoreGuids;
                auto guids = QString::fromStdString(guidsForCourse)
                               .split(' ', Qt::SkipEmptyParts);
                scoreGuids.append(guids);
            }

            std::vector<std::tuple<gameplay_logic::BmsResult::DTO,
                                   gameplay_logic::BmsReplayData::DTO,
                                   gameplay_logic::BmsGaugeHistory::DTO>>
              allResults;

            for (int i = 0; i < scoreGuids.size(); i += maxVariables) {
                auto chunk = scoreGuids.mid(i, maxVariables);
                auto statement = scoreDb->createStatement(
                  "SELECT * "
                  "FROM score "
                  "JOIN replay_data ON score.guid = replay_data.score_guid "
                  "JOIN gauge_history ON score.guid = gauge_history.score_guid "
                  "WHERE score.guid IN (" +
                  QString("?, ")
                    .repeated(chunk.size())
                    .chopped(2)
                    .toStdString() +
                  ") ORDER BY score.unix_timestamp DESC");

                for (int j = 0; j < chunk.size(); ++j) {
                    statement.bind(j + 1, chunk[j].toStdString());
                }

                const auto result = statement.executeAndGetAll<
                  std::tuple<gameplay_logic::BmsResult::DTO,
                             gameplay_logic::BmsReplayData::DTO,
                             gameplay_logic::BmsGaugeHistory::DTO>>();
                allResults.append_range(result);
            }
            QHash<QString, gameplay_logic::BmsScore*> groupedScores;
            auto* mainThread = QCoreApplication::instance()->thread();
            for (const auto& row : allResults) {
                auto guid = QString::fromStdString(std::get<0>(row).guid);
                auto* score = new gameplay_logic::BmsScore{
                    gameplay_logic::BmsResult::load(std::get<0>(row)),
                    gameplay_logic::BmsReplayData::load(std::get<1>(row)),
                    gameplay_logic::BmsGaugeHistory::load(std::get<2>(row))
                };
                score->moveToThread(mainThread);
                groupedScores[guid] = score;
            }
            auto courseScores = QMap<QString, QVariantList>{};
            for (const auto& courseScore : allCourseResults) {
                auto scoreGuids = QString::fromStdString(courseScore.scoreGuids)
                                    .split(' ', Qt::SkipEmptyParts);
                auto scoresForCourse = QList<gameplay_logic::BmsScore*>{};
                for (const auto& guid : scoreGuids) {
                    if (groupedScores.contains(guid)) {
                        scoresForCourse.append(groupedScores[guid]);
                    }
                }
                courseScores[QString::fromStdString(courseScore.identifier)].push_back(
                  QVariant::fromValue(gameplay_logic::BmsScoreCourse::load(courseScore, scoresForCourse).release()));
            }

            auto result = TableScoreQueryResult{};
            result.scores = std::move(scores.scores);
            result.unplayed = scores.unplayed;
            for (const auto& [courseId, scores] :
                 courseScores.asKeyValueRange()) {
                result.courseScores[courseId] = scores;
            }
            result.courseUnplayed = courseIds.size() - courseScores.size();

            QMetaObject::invokeMethod(
              QCoreApplication::instance(),
              [reply, token, result = std::move(result)]() mutable {
                  if (token.stop_requested()) {
                      reply.setFailed();
                      return;
                  }
                  reply.setSuccess(result);
              },
              Qt::QueuedConnection);
        } catch (const std::exception& e) {
            QMetaObject::invokeMethod(
              QCoreApplication::instance(),
              [reply, e]() mutable {
                  spdlog::error("Error in getScoresForMd5: {}", e.what());
                  reply.setFailed();
              },
              Qt::QueuedConnection);
        }
    });
    return reply;
}
auto
ScoreDb::getScores(const resource_managers::Level& level) const
  -> QIfPendingReply<ScoreQueryResult>
{
    auto md5s = QStringList{};
    for (const auto& entry : level.entries) {
        md5s.append(entry.md5);
    }
    return getScoresForMd5(md5s);
}
void
ScoreDb::cancelPending()
{
    threadPool.clear();
    stopSource.request_stop();
    stopSource = std::stop_source{};
}
auto
ScoreDb::getTotalScoreCount() const -> int
{
    auto statement = scoreDb->createStatement("SELECT COUNT(*) FROM score");
    return statement.executeAndGet<int>().value_or(0);
}
} // namespace qml_components
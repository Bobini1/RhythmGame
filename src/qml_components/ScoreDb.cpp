//
// Created by bobini on 07.10.23.
//

#include "ScoreDb.h"

#include "gameplay_logic/BmsResultCourse.h"
#include "gameplay_logic/BmsScoreCourse.h"
#include "support/PathToUtfString.h"

#include <QFutureWatcher>
#include <QHash>
#include <QSet>
#include <QVariantList>
#include <qcoreapplication.h>
#include <qtconcurrentrun.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace qml_components {

constexpr int maxVariables = 999;
namespace {
struct ScoreStatsRow
{
    int64_t playCount{};
    int64_t clearCount{};
    int64_t failCount{};
    int64_t perfectCount{};
    int64_t greatCount{};
    int64_t goodCount{};
    int64_t badCount{};
    int64_t poorCount{};
    int64_t maxCombo{};
};

struct ScoreSummaryRow
{
    std::string md5;
    std::string clearType;
    double points{};
    double maxPoints{};
};

struct ScoreSummaryChart
{
    bool hasScore = false;
    QString bestClearType = QStringLiteral("NOPLAY");
    int bestClearPriority = 0;
    double bestRate = -1.0;
    double bestPoints = 0.0;
    double bestMaxPoints = 0.0;
};

int
summaryClearTypePriority(const QString& clearType)
{
    if (clearType == QStringLiteral("FAILED"))
        return 1;
    if (clearType == QStringLiteral("AEASY"))
        return 2;
    if (clearType == QStringLiteral("LIGHTASSIST") ||
        clearType == QStringLiteral("LIGHT_ASSIST"))
        return 3;
    if (clearType == QStringLiteral("EASY"))
        return 4;
    if (clearType == QStringLiteral("NORMAL"))
        return 5;
    if (clearType == QStringLiteral("HARD"))
        return 6;
    if (clearType == QStringLiteral("EXHARD"))
        return 7;
    if (clearType == QStringLiteral("FC"))
        return 8;
    if (clearType == QStringLiteral("PERFECT"))
        return 9;
    if (clearType == QStringLiteral("MAX"))
        return 10;
    return 0;
}

int
summaryClearTypeLamp(const QString& clearType)
{
    if (clearType == QStringLiteral("FAILED"))
        return 1;
    if (clearType == QStringLiteral("AEASY") ||
        clearType == QStringLiteral("LIGHTASSIST") ||
        clearType == QStringLiteral("LIGHT_ASSIST") ||
        clearType == QStringLiteral("EASY"))
        return 2;
    if (clearType == QStringLiteral("NORMAL"))
        return 3;
    if (clearType == QStringLiteral("HARD") ||
        clearType == QStringLiteral("EXHARD"))
        return 4;
    if (clearType == QStringLiteral("FC") ||
        clearType == QStringLiteral("PERFECT") ||
        clearType == QStringLiteral("MAX")) {
        return 5;
    }
    return 0;
}

QString
canonicalSummaryClearType(const QString& clearType)
{
    if (clearType == QStringLiteral("FAILED"))
        return QStringLiteral("FAILED");
    if (clearType == QStringLiteral("AEASY"))
        return QStringLiteral("AEASY");
    if (clearType == QStringLiteral("LIGHTASSIST") ||
        clearType == QStringLiteral("LIGHT_ASSIST"))
        return QStringLiteral("LIGHTASSIST");
    if (clearType == QStringLiteral("EASY"))
        return QStringLiteral("EASY");
    if (clearType == QStringLiteral("NORMAL"))
        return QStringLiteral("NORMAL");
    if (clearType == QStringLiteral("HARD"))
        return QStringLiteral("HARD");
    if (clearType == QStringLiteral("EXHARD"))
        return QStringLiteral("EXHARD");
    if (clearType == QStringLiteral("FC"))
        return QStringLiteral("FC");
    if (clearType == QStringLiteral("PERFECT"))
        return QStringLiteral("PERFECT");
    if (clearType == QStringLiteral("MAX"))
        return QStringLiteral("MAX");
    return QStringLiteral("NOPLAY");
}

int
summaryClearTypeDistributionIndex(const QString& clearType)
{
    if (clearType == QStringLiteral("FAILED"))
        return 1;
    if (clearType == QStringLiteral("AEASY"))
        return 2;
    if (clearType == QStringLiteral("LIGHTASSIST") ||
        clearType == QStringLiteral("LIGHT_ASSIST")) {
        return 3;
    }
    if (clearType == QStringLiteral("EASY"))
        return 4;
    if (clearType == QStringLiteral("NORMAL"))
        return 5;
    if (clearType == QStringLiteral("HARD"))
        return 6;
    if (clearType == QStringLiteral("EXHARD"))
        return 7;
    if (clearType == QStringLiteral("FC"))
        return 8;
    if (clearType == QStringLiteral("PERFECT"))
        return 9;
    if (clearType == QStringLiteral("MAX"))
        return 10;
    return 0;
}

void
incrementSummaryCount(QVariantMap& counts, const QString& key)
{
    counts.insert(key, counts.value(key).toInt() + 1);
}

QVariantMap
emptySummaryCounts()
{
    return {
        { QStringLiteral("NOPLAY"), 0 }, { QStringLiteral("FAILED"), 0 },
        { QStringLiteral("AEASY"), 0 },  { QStringLiteral("LIGHTASSIST"), 0 },
        { QStringLiteral("EASY"), 0 },   { QStringLiteral("NORMAL"), 0 },
        { QStringLiteral("HARD"), 0 },   { QStringLiteral("EXHARD"), 0 },
        { QStringLiteral("FC"), 0 },     { QStringLiteral("PERFECT"), 0 },
        { QStringLiteral("MAX"), 0 },
    };
}

QVariantList
zeroList(int size)
{
    QVariantList result;
    result.reserve(size);
    for (int i = 0; i < size; ++i) {
        result.append(0);
    }
    return result;
}

class ScoreSummaryAccumulator
{
  public:
    void addChart(const QString& md5)
    {
        const QString key = md5.trimmed().toUpper();
        if (key.isEmpty()) {
            return;
        }
        if (!charts.contains(key)) {
            charts.insert(key, ScoreSummaryChart{});
        }
    }

    void addScore(const QString& md5,
                  const QString& clearType,
                  double points,
                  double maxPoints)
    {
        const QString key = md5.trimmed().toUpper();
        if (key.isEmpty() || clearType.isEmpty()) {
            addChart(key);
            return;
        }

        addChart(key);
        auto& chart = charts[key];
        chart.hasScore = true;

        const int priority = summaryClearTypePriority(clearType);
        if (priority > chart.bestClearPriority) {
            chart.bestClearPriority = priority;
            chart.bestClearType = clearType;
        }

        const double rate = maxPoints > 0.0 ? points / maxPoints : -1.0;
        if (rate > chart.bestRate) {
            chart.bestRate = rate;
            chart.bestPoints = points;
            chart.bestMaxPoints = maxPoints;
        }
    }

    QVariantMap toVariantMap() const
    {
        QVariantMap counts = emptySummaryCounts();
        QVariantList lamps = zeroList(11);
        QVariantList ranks = zeroList(28);
        int folderLamp = 5;
        bool seenScore = false;
        int unplayed = 0;

        for (const auto& chart : charts) {
            if (!chart.hasScore) {
                ++unplayed;
                incrementSummaryCount(counts, QStringLiteral("NOPLAY"));
                lamps[0] = lamps.at(0).toInt() + 1;
                ranks[0] = ranks.at(0).toInt() + 1;
                continue;
            }

            seenScore = true;
            const QString clearType =
              canonicalSummaryClearType(chart.bestClearType);
            incrementSummaryCount(counts, clearType);

            folderLamp = std::min(folderLamp, summaryClearTypeLamp(clearType));
            const int lampIndex =
              std::clamp(summaryClearTypeDistributionIndex(clearType), 0, 10);
            lamps[lampIndex] = lamps.at(lampIndex).toInt() + 1;

            if (chart.bestMaxPoints <= 0.0) {
                ranks[0] = ranks.at(0).toInt() + 1;
                continue;
            }

            const int rank =
              std::clamp(static_cast<int>(std::floor(chart.bestPoints * 27.0 /
                                                     chart.bestMaxPoints)),
                         0,
                         27);
            ranks[rank] = ranks.at(rank).toInt() + 1;
        }

        return {
            { QStringLiteral("lamp"),
              unplayed > 0 ? 0 : (seenScore ? folderLamp : 0) },
            { QStringLiteral("counts"), counts },
            { QStringLiteral("distribution"),
              QVariantMap{
                { QStringLiteral("lamps"), lamps },
                { QStringLiteral("ranks"), ranks },
              } },
        };
    }

  private:
    QHash<QString, ScoreSummaryChart> charts;
};
}

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
          "SELECT score.max_points, score.max_hits, score.normal_note_count, "
          "score.scratch_count, score.ln_count, score.bss_count, "
          "score.mine_count, "
          "score.clear_type, score.points, score.max_combo, score.poor, "
          "score.empty_poor, score.bad, score.good, score.great, "
          "score.perfect, score.mine_hits, score.guid, score.sha256, "
          "score.md5, score.unix_timestamp, score.length, "
          "score.random_sequence, score.random_seed, "
          "score.note_order_algorithm, score.note_order_algorithm_p2, "
          "score.dp_options, score.keymode, score.game_version, score.owner, "
          "replay_data.*, gauge_history.* "
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
        allResults.insert(allResults.end(), result.begin(), result.end());
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

auto
ScoreDb::getScoresForCourseIdImpl(const QList<QString>& courseIds) const
  -> ScoreQueryResult
{
    auto allCourseResults = std::vector<gameplay_logic::BmsResultCourse::DTO>{};
    for (int i = 0; i < courseIds.size(); i += maxVariables) {
        auto chunk = courseIds.mid(i, maxVariables);
        auto statement = scoreDb->createStatement(
          "SELECT * "
          "FROM score_course "
          "WHERE score_course.identifier IN (" +
          QString("?, ").repeated(chunk.size()).chopped(2).toStdString() +
          ") ORDER BY score_course.unix_timestamp DESC");

        for (int j = 0; j < chunk.size(); ++j) {
            statement.bind(j + 1, chunk[j].toStdString());
        }

        const auto result =
          statement.executeAndGetAll<gameplay_logic::BmsResultCourse::DTO>();
        allCourseResults.insert(
          allCourseResults.end(), result.begin(), result.end());
    }

    auto scoreGuids = QList<QString>{};
    for (const auto& course : allCourseResults) {
        const auto& guidsForCourse = course.scoreGuids;
        auto guids =
          QString::fromStdString(guidsForCourse).split(' ', Qt::SkipEmptyParts);
        scoreGuids.append(guids);
    }

    std::vector<std::tuple<gameplay_logic::BmsResult::DTO,
                           gameplay_logic::BmsReplayData::DTO,
                           gameplay_logic::BmsGaugeHistory::DTO>>
      allResults;

    for (int i = 0; i < scoreGuids.size(); i += maxVariables) {
        auto chunk = scoreGuids.mid(i, maxVariables);
        auto statement = scoreDb->createStatement(
          "SELECT score.max_points, score.max_hits, score.normal_note_count, "
          "score.scratch_count, score.ln_count, score.bss_count, "
          "score.mine_count, "
          "score.clear_type, score.points, score.max_combo, score.poor, "
          "score.empty_poor, score.bad, score.good, score.great, "
          "score.perfect, score.mine_hits, score.guid, score.sha256, "
          "score.md5, score.unix_timestamp, score.length, "
          "score.random_sequence, score.random_seed, "
          "score.note_order_algorithm, score.note_order_algorithm_p2, "
          "score.dp_options, score.keymode, score.game_version, score.owner, "
          "replay_data.*, gauge_history.* "
          "FROM score "
          "JOIN replay_data ON score.guid = replay_data.score_guid "
          "JOIN gauge_history ON score.guid = gauge_history.score_guid "
          "WHERE score.guid IN (" +
          QString("?, ").repeated(chunk.size()).chopped(2).toStdString() +
          ") ORDER BY score.unix_timestamp DESC");

        for (int j = 0; j < chunk.size(); ++j) {
            statement.bind(j + 1, chunk[j].toStdString());
        }

        const auto result = statement.executeAndGetAll<
          std::tuple<gameplay_logic::BmsResult::DTO,
                     gameplay_logic::BmsReplayData::DTO,
                     gameplay_logic::BmsGaugeHistory::DTO>>();
        allResults.insert(allResults.end(), result.begin(), result.end());
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
        auto score = gameplay_logic::BmsScoreCourse::fromScores(
          gameplay_logic::BmsResultCourse::load(courseScore, scoresForCourse),
          scoresForCourse);
        score->moveToThread(mainThread);
        courseScores[QString::fromStdString(courseScore.identifier)].push_back(
          QVariant::fromValue(score.release()));
    }

    auto result = ScoreQueryResult{};
    result.scores = QVariantMap{};
    for (const auto& [courseId, scores] : courseScores.asKeyValueRange()) {
        result.scores[courseId] = scores;
    }
    result.unplayed = courseIds.size() - courseScores.size();

    return result;
}

auto
ScoreDb::getScoreSummaryForMd5Impl(const QList<QString>& md5s) const
  -> QVariantMap
{
    ScoreSummaryAccumulator summary;
    QSet<QString> uniqueMd5s;
    for (const QString& md5 : md5s) {
        const QString normalized = md5.trimmed().toUpper();
        if (normalized.isEmpty() || uniqueMd5s.contains(normalized)) {
            continue;
        }
        uniqueMd5s.insert(normalized);
        summary.addChart(normalized);
    }

    const auto md5sToFetch = uniqueMd5s.values();
    for (int i = 0; i < md5sToFetch.size(); i += maxVariables) {
        auto chunk = md5sToFetch.mid(i, maxVariables);
        auto statement = scoreDb->createStatement(
          "SELECT score.md5, score.clear_type, score.points, score.max_points "
          "FROM score "
          "WHERE score.md5 IN (" +
          QString("?, ").repeated(chunk.size()).chopped(2).toStdString() + ")");

        for (int j = 0; j < chunk.size(); ++j) {
            statement.bind(j + 1, chunk[j].toStdString());
        }

        const auto rows = statement.executeAndGetAll<ScoreSummaryRow>();
        for (const auto& row : rows) {
            summary.addScore(QString::fromStdString(row.md5),
                             QString::fromStdString(row.clearType),
                             row.points,
                             row.maxPoints);
        }
    }

    return summary.toVariantMap();
}

auto
ScoreDb::getFolderScoreSummaryImpl(const QString& folder) const -> QVariantMap
{
    ScoreSummaryAccumulator summary;
    auto query = scoreDb->createStatement(
      "SELECT song_db.charts.md5, "
      "COALESCE(score.clear_type, ''), "
      "COALESCE(score.points, 0), "
      "COALESCE(score.max_points, 0) "
      "FROM song_db.charts "
      "LEFT JOIN score ON score.md5 = song_db.charts.md5 "
      "WHERE song_db.charts.path LIKE ? || '%'");

    query.bind(1, folder.toStdString());
    const auto rows = query.executeAndGetAll<ScoreSummaryRow>();
    for (const auto& row : rows) {
        const auto md5 = QString::fromStdString(row.md5);
        summary.addChart(md5);
        if (!row.clearType.empty()) {
            summary.addScore(md5,
                             QString::fromStdString(row.clearType),
                             row.points,
                             row.maxPoints);
        }
    }
    return summary.toVariantMap();
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
QIfPendingReply<ScoreQueryResult>
ScoreDb::getScoresForCourseId(const QList<QString>& courseIds) const
{
    auto reply = QIfPendingReply<ScoreQueryResult>{};
    if (courseIds.isEmpty()) {
        reply.setSuccess(ScoreQueryResult{});
        return reply;
    }
    auto token = stopSource.get_token();
    threadPool.start([this, courseIds, token, reply]() mutable {
        try {
            auto result = getScoresForCourseIdImpl(courseIds);

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
                  spdlog::error("Error in getScoresForCourseId: {}", e.what());
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
              "SELECT score.max_points, score.max_hits, "
              "score.normal_note_count, score.scratch_count, score.ln_count, "
              "score.bss_count, "
              "score.mine_count, score.clear_type, score.points, "
              "score.max_combo, score.poor, score.empty_poor, score.bad, "
              "score.good, score.great, score.perfect, score.mine_hits, "
              "score.guid, score.sha256, score.md5, score.unix_timestamp, "
              "score.length, score.random_sequence, score.random_seed, "
              "score.note_order_algorithm, score.note_order_algorithm_p2, "
              "score.dp_options, score.keymode, score.game_version, "
              "score.owner, "
              "replay_data.*, gauge_history.* "
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
  -> QIfPendingReply<TableQueryResult>
{
    auto reply = QIfPendingReply<TableQueryResult>{};
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

            auto result = TableQueryResult{};
            result.scores = std::move(scores);

            auto courseResults = getScoresForCourseIdImpl(courseIds);
            result.courseScores = std::move(courseResults);

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

auto
ScoreDb::getScoreSummary(const QString& folder) const
  -> QIfPendingReply<QVariantMap>
{
    auto reply = QIfPendingReply<QVariantMap>{};
    auto token = stopSource.get_token();
    threadPool.start([this, folder, token, reply]() mutable {
        try {
            auto result = getFolderScoreSummaryImpl(folder);
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
                  spdlog::error("Error in getScoreSummary: {}", e.what());
                  reply.setFailed();
              },
              Qt::QueuedConnection);
        }
    });
    return reply;
}

auto
ScoreDb::getScoreSummary(const resource_managers::Table& table) const
  -> QIfPendingReply<QVariantMap>
{
    auto md5s = QStringList{};
    for (const auto& level : table.levels) {
        for (const auto& entry : level.entries) {
            md5s.append(entry.md5);
        }
    }

    auto reply = QIfPendingReply<QVariantMap>{};
    auto token = stopSource.get_token();
    threadPool.start([this, md5s, token, reply]() mutable {
        try {
            auto result = getScoreSummaryForMd5Impl(md5s);
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
                  spdlog::error("Error in getScoreSummary(table): {}",
                                e.what());
                  reply.setFailed();
              },
              Qt::QueuedConnection);
        }
    });
    return reply;
}

auto
ScoreDb::getScoreSummary(const resource_managers::Level& level) const
  -> QIfPendingReply<QVariantMap>
{
    auto md5s = QStringList{};
    for (const auto& entry : level.entries) {
        md5s.append(entry.md5);
    }

    auto reply = QIfPendingReply<QVariantMap>{};
    auto token = stopSource.get_token();
    threadPool.start([this, md5s, token, reply]() mutable {
        try {
            auto result = getScoreSummaryForMd5Impl(md5s);
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
                  spdlog::error("Error in getScoreSummary(level): {}",
                                e.what());
                  reply.setFailed();
              },
              Qt::QueuedConnection);
        }
    });
    return reply;
}

auto
ScoreDb::getTotalStats() const -> QIfPendingReply<ScoreStatsResult>
{
    auto reply = QIfPendingReply<ScoreStatsResult>{};
    auto token = stopSource.get_token();
    threadPool.start([reply, token, this] {
        try {
            auto statement = scoreDb->createStatement(
              "SELECT "
              "COUNT(*), "
              "COALESCE(SUM(CASE WHEN clear_type NOT IN ('FAILED', 'NOPLAY') "
              "THEN 1 ELSE 0 END), 0), "
              "COALESCE(SUM(CASE WHEN clear_type = 'FAILED' THEN 1 ELSE 0 "
              "END), 0), "
              "COALESCE(SUM(perfect), 0), "
              "COALESCE(SUM(great), 0), "
              "COALESCE(SUM(good), 0), "
              "COALESCE(SUM(bad), 0), "
              "COALESCE(SUM(poor + empty_poor), 0), "
              "COALESCE(MAX(max_combo), 0) "
              "FROM score");
            auto row = statement.executeAndGet<ScoreStatsRow>().value_or(
              ScoreStatsRow{});

            QMetaObject::invokeMethod(
              QCoreApplication::instance(),
              [reply, token, row]() mutable {
                  if (token.stop_requested()) {
                      reply.setFailed();
                      return;
                  }
                  ScoreStatsResult result;
                  result.playCount = row.playCount;
                  result.clearCount = row.clearCount;
                  result.failCount = row.failCount;
                  result.perfectCount = row.perfectCount;
                  result.greatCount = row.greatCount;
                  result.goodCount = row.goodCount;
                  result.badCount = row.badCount;
                  result.poorCount = row.poorCount;
                  result.maxCombo = row.maxCombo;
                  reply.setSuccess(result);
              },
              Qt::QueuedConnection);
        } catch (std::exception& e) {
            QMetaObject::invokeMethod(
              QCoreApplication::instance(),
              [reply, e]() mutable {
                  spdlog::error("Error in getTotalStats: {}", e.what());
                  reply.setFailed();
              },
              Qt::QueuedConnection);
        }
    });
    return reply;
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

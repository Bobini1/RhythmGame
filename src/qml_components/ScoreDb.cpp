//
// Created by bobini on 07.10.23.
//

#include "ScoreDb.h"

#include "gameplay_logic/BmsResultCourse.h"
#include "gameplay_logic/BmsScoreCourse.h"

#include <QCache>
#include <QHash>
#include <QJSEngine>
#include <QPointer>
#include <QSet>
#include <QVariantList>
#include <qcoreapplication.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace qml_components {

constexpr int maxVariables = 999;

class ScoreCache
{
  public:
    struct Load
    {
        QString guid;
        bool course;
        std::stop_source stop;
        QList<support::PendingReplySource<QVariant>> subscribers;
    };
    quint64 revision = 0;
    QHash<QString, gameplay_logic::BmsResult*> results;
    QSet<QString> presentResults;
    QVariantMap chartResults;
    QHash<QString, gameplay_logic::BmsResultCourse*> courseResults;
    QVariantMap courseHistories;
    QCache<QString, QObject> scores{ 64 * 1024 * 1024 };
    QHash<QString, std::shared_ptr<Load>> loads;
};

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

struct StoredScoreRow
{
    gameplay_logic::BmsResult::DTO result;
    int source{};
    int longNoteMode{};
    int replayRowExists{};
    gameplay_logic::BmsReplayData::DTO replayData;
    int gaugeRowExists{};
    gameplay_logic::BmsGaugeHistory::DTO gaugeHistory;
};

struct StoredResultRow
{
    gameplay_logic::BmsResult::DTO result;
    int source;
    int longNoteMode;
    int replayAvailable;
    int gaugeAvailable;
};

constexpr auto storedResultColumns =
  "score.max_points, score.max_hits, score.normal_note_count, "
  "score.scratch_count, score.ln_count, score.bss_count, score.mine_count, "
  "score.clear_type, score.points, score.max_combo, score.poor, "
  "score.empty_poor, score.bad, score.good, score.great, score.perfect, "
  "score.mine_hits, score.guid, score.sha256, score.md5, "
  "score.unix_timestamp, score.length, score.random_sequence, "
  "score.random_seed, score.note_order_algorithm, "
  "score.note_order_algorithm_p2, score.dp_options, score.keymode, "
  "score.game_version, score.owner, score.source, score.ln_mode";

const auto storedScoreColumns =
  std::string(storedResultColumns) + ", " +
  "replay_data.score_guid IS NOT NULL, replay_data.id, "
  "replay_data.score_guid, replay_data.replay_data, "
  "gauge_history.score_guid IS NOT NULL, gauge_history.id, "
  "gauge_history.score_guid, gauge_history.gauge_info ";

auto
loadStoredScore(const StoredScoreRow& row, QObject* parent = nullptr)
  -> std::unique_ptr<gameplay_logic::BmsScore>
{
    auto replayData = row.replayRowExists
                        ? gameplay_logic::BmsReplayData::load(row.replayData)
                        : nullptr;
    auto gaugeHistory =
      row.gaugeRowExists
        ? gameplay_logic::BmsGaugeHistory::load(row.gaugeHistory)
        : nullptr;
    return std::make_unique<gameplay_logic::BmsScore>(
      gameplay_logic::BmsResult::load(row.result),
      std::move(replayData),
      std::move(gaugeHistory),
      parent,
      static_cast<gameplay_logic::BmsScore::Source>(row.source),
      static_cast<gameplay_logic::BmsScore::LongNoteMode>(row.longNoteMode));
}

template<typename Result, typename Query>
auto
runScoreQuery(qml_components::ScoreDb* owner,
              QThreadPool& threadPool,
              std::string operation,
              Query query) -> support::PendingReply*
{
    auto source = support::PendingReplySource<Result>{ owner };
    auto* reply = source.reply();
    const auto stopToken = source.stopToken();

    threadPool.start([source,
                      stopToken,
                      operation = std::move(operation),
                      query = std::move(query)]() mutable {
        if (stopToken.stop_requested())
            return;

        try {
            auto result = query(stopToken);
            if (stopToken.stop_requested())
                return;

            auto* application = QCoreApplication::instance();
            const auto queued =
              application && QMetaObject::invokeMethod(
                               application,
                               [source, result = std::move(result)] {
                                   auto* reply = source.reply();
                                   if (!reply || reply->isResultAvailable())
                                       return;
                                   (void)source.succeed(result);
                               },
                               Qt::QueuedConnection);
            if (!queued) {
                spdlog::error("{}: failed to queue score query result",
                              operation);
            }
        } catch (const std::exception& exception) {
            if (stopToken.stop_requested())
                return;
            auto error = std::string(exception.what());
            auto* application = QCoreApplication::instance();
            const auto queued =
              application &&
              QMetaObject::invokeMethod(
                application,
                [source, operation, error] {
                    if (source.fail())
                        spdlog::error("{}: {}", operation, error);
                },
                Qt::QueuedConnection);
            if (!queued)
                spdlog::error("{}: {}", operation, error);
        }
    });

    return reply;
}

int
summaryClearTypePriority(const QString& clearType)
{
    if (clearType == QStringLiteral("FAILED"))
        return 1;
    if (clearType == QStringLiteral("AEASY"))
        return 2;
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
        { QStringLiteral("NOPLAY"), 0 },  { QStringLiteral("FAILED"), 0 },
        { QStringLiteral("AEASY"), 0 },   { QStringLiteral("EASY"), 0 },
        { QStringLiteral("NORMAL"), 0 },  { QStringLiteral("HARD"), 0 },
        { QStringLiteral("EXHARD"), 0 },  { QStringLiteral("FC"), 0 },
        { QStringLiteral("PERFECT"), 0 }, { QStringLiteral("MAX"), 0 },
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
ScoreDb::getScoreSummaryForMd5Impl(const QList<QString>& md5s,
                                   std::stop_token stopToken) const
  -> QVariantMap
{
    ScoreSummaryAccumulator summary;
    QSet<QString> uniqueMd5s;
    for (const QString& md5 : md5s) {
        if (stopToken.stop_requested())
            return {};
        const QString normalized = md5.trimmed().toUpper();
        if (normalized.isEmpty() || uniqueMd5s.contains(normalized)) {
            continue;
        }
        uniqueMd5s.insert(normalized);
        summary.addChart(normalized);
    }

    const auto md5sToFetch = uniqueMd5s.values();
    for (int i = 0; i < md5sToFetch.size(); i += maxVariables) {
        if (stopToken.stop_requested())
            return {};
        auto chunk = md5sToFetch.mid(i, maxVariables);
        auto statement = scoreDb->createStatement(
          "SELECT score.md5, score.clear_type, score.points, score.max_points "
          "FROM score "
          "WHERE score.md5 IN (" +
          QString("?, ").repeated(chunk.size()).chopped(2).toStdString() + ")");

        for (int j = 0; j < chunk.size(); ++j) {
            statement.bind(j + 1, chunk[j].toStdString());
        }

        const auto rows =
          statement.executeAndGetAll<ScoreSummaryRow>(stopToken);
        for (const auto& row : rows) {
            if (stopToken.stop_requested())
                return {};
            summary.addScore(QString::fromStdString(row.md5),
                             QString::fromStdString(row.clearType),
                             row.points,
                             row.maxPoints);
        }
    }

    return summary.toVariantMap();
}

auto
ScoreDb::getFolderScoreSummaryImpl(const QString& folder,
                                   std::stop_token stopToken) const
  -> QVariantMap
{
    ScoreSummaryAccumulator summary;
    auto query = scoreDb->createStatement(
      "SELECT song_db.charts.md5, "
      "COALESCE(score.clear_type, ''), "
      "COALESCE(score.points, 0), "
      "COALESCE(score.max_points, 0) "
      "FROM song_db.charts "
      "LEFT JOIN score ON score.md5 = song_db.charts.md5 "
      "WHERE instr(song_db.charts.path, ?1) = 1 OR "
      "song_db.charts.id IN (SELECT chart_id FROM song_db.folder_charts "
      "WHERE directory IN (SELECT id FROM song_db.parent_dir "
      "WHERE instr(dir, ?1) = 1))");

    query.bind(1, folder.toStdString());
    const auto rows = query.executeAndGetAll<ScoreSummaryRow>(stopToken);
    for (const auto& row : rows) {
        if (stopToken.stop_requested())
            return {};
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

namespace {
auto
copyScore(const QObject* score) -> std::unique_ptr<QObject>
{
    if (const auto* chart =
          qobject_cast<const gameplay_logic::BmsScore*>(score))
        return chart->clone();
    return qobject_cast<const gameplay_logic::BmsScoreCourse*>(score)->clone();
}

qsizetype
scoreCost(const QObject* score)
{
    qsizetype bytes = 1024;
    const gameplay_logic::BmsReplayData* replay;
    const gameplay_logic::BmsGaugeHistory* gauges;
    if (const auto* chart =
          qobject_cast<const gameplay_logic::BmsScore*>(score)) {
        replay = chart->getReplayData();
        gauges = chart->getGaugeHistory();
    } else {
        const auto* course =
          qobject_cast<const gameplay_logic::BmsScoreCourse*>(score);
        replay = course->getReplayData();
        gauges = course->getGaugeHistory();
        for (const auto* stage : course->getScores())
            bytes += scoreCost(stage);
    }
    if (replay)
        bytes +=
          replay->getHitEvents().size() * sizeof(gameplay_logic::HitEvent);
    if (gauges) {
        for (const auto& info : gauges->getGaugeInfo())
            bytes +=
              sizeof(info) + info.gaugeHistory.size() *
                               sizeof(gameplay_logic::rules::GaugeHistoryEntry);
    }
    return bytes;
}

auto
readChartRow(db::SqliteCppDb& db, const QString& guid, std::stop_token stop)
  -> StoredScoreRow
{
    auto statement =
      db.createStatement(std::string("SELECT ") + storedScoreColumns +
                         " FROM score LEFT JOIN replay_data ON score.guid = "
                         "replay_data.score_guid "
                         "LEFT JOIN gauge_history ON score.guid = "
                         "gauge_history.score_guid WHERE score.guid = ?");
    statement.bind(1, guid.toStdString());
    auto row = statement.executeAndGet<StoredScoreRow>(stop);
    if (!row)
        throw std::runtime_error("Saved score no longer exists");
    return std::move(*row);
}

auto
readFullScore(db::SqliteCppDb& db,
              const QString& guid,
              bool course,
              std::stop_token stop) -> std::unique_ptr<QObject>
{
    auto transaction = db.transaction();
    if (!course) {
        auto row = readChartRow(db, guid, stop);
        transaction.commit();
        if (stop.stop_requested())
            return nullptr;
        return loadStoredScore(row);
    }
    auto query =
      db.createStatement("SELECT * FROM score_course WHERE guid = ?");
    query.bind(1, guid.toStdString());
    auto row = query.executeAndGet<gameplay_logic::BmsResultCourse::DTO>(stop);
    if (!row)
        throw std::runtime_error("Saved course score no longer exists");
    const auto guids =
      QString::fromStdString(row->scoreGuids).split(' ', Qt::SkipEmptyParts);
    QHash<QString, StoredScoreRow> rows;
    for (const auto& stageGuid : guids) {
        if (stop.stop_requested())
            return nullptr;
        if (!rows.contains(stageGuid))
            rows.insert(stageGuid, readChartRow(db, stageGuid, stop));
    }
    // Release the read connection before decompressing and combining timelines.
    transaction.commit();
    QObject owner;
    QHash<QString, gameplay_logic::BmsScore*> loaded;
    QList<gameplay_logic::BmsScore*> stages;
    QList<gameplay_logic::BmsResult*> results;
    for (const auto& stageGuid : guids) {
        if (stop.stop_requested())
            return nullptr;
        auto stage = loaded.contains(stageGuid)
                       ? loaded.value(stageGuid)->clone()
                       : loadStoredScore(rows[stageGuid]);
        if (!stage)
            return nullptr;
        stage->setParent(&owner);
        loaded.insert(stageGuid, stage.get());
        results.append(stage->getResult());
        stages.append(stage.release());
    }
    return gameplay_logic::BmsScoreCourse::fromScores(
      gameplay_logic::BmsResultCourse::load(*row, results), stages);
}
}

void
ScoreDb::refreshResults(const QStringList& guids)
{
    try {
        auto transaction = scoreDb->transaction();
        auto sql =
          std::string("SELECT ") + storedResultColumns +
          ", EXISTS(SELECT 1 FROM replay_data WHERE score_guid = score.guid), "
          "EXISTS(SELECT 1 FROM gauge_history WHERE score_guid = score.guid) "
          "FROM "
          "score";
        if (!guids.isEmpty())
            sql +=
              " WHERE score.guid IN (" +
              QString("?,").repeated(guids.size()).chopped(1).toStdString() +
              ")";
        auto query = scoreDb->createStatement(sql);
        for (int i = 0; i < guids.size(); ++i)
            query.bind(i + 1, guids[i].toStdString());
        auto rows = query.executeAndGetAll<StoredResultRow>();
        auto courses =
          scoreDb->createStatement("SELECT * FROM score_course")
            .executeAndGetAll<gameplay_logic::BmsResultCourse::DTO>();
        transaction.commit();

        // Decode result metadata before replacing the published chart results.
        std::vector<std::unique_ptr<gameplay_logic::BmsResult>> updated;
        for (const auto& row : rows) {
            auto result = gameplay_logic::BmsResult::load(row.result);
            result->setStoredData(
              row.replayAvailable,
              row.gaugeAvailable,
              row.source != 0,
              gameplay_logic::BmsScore::sourceName(
                static_cast<gameplay_logic::BmsScore::Source>(row.source)));
            updated.push_back(std::move(result));
        }
        if (guids.isEmpty())
            cache->presentResults.clear();
        else
            for (const auto& guid : guids)
                cache->presentResults.remove(guid);
        for (auto& result : updated) {
            const auto guid = result->getGuid();
            cache->presentResults.insert(guid);
            if (auto* existing = cache->results.value(guid)) {
                existing->updateFrom(*result);
            } else {
                result->setParent(this);
                cache->results.insert(guid, result.release());
            }
        }
        QMap<QString, QList<gameplay_logic::BmsResult*>> byMd5;
        for (const auto& guid : cache->presentResults) {
            auto* result = cache->results.value(guid);
            byMd5[result->getMd5().toUpper()].append(result);
        }
        const auto histories = [](auto& groups) {
            QVariantMap map;
            for (auto it = groups.begin(); it != groups.end(); ++it) {
                auto& results = it.value();
                std::ranges::sort(results, [](auto* a, auto* b) {
                    if (a->getUnixTimestamp() != b->getUnixTimestamp())
                        return a->getUnixTimestamp() > b->getUnixTimestamp();
                    return a->getGuid() < b->getGuid();
                });
                QVariantList list;
                list.reserve(results.size());
                for (auto* result : results)
                    list.append(QVariant::fromValue(result));
                map.insert(it.key(), list);
            }
            return map;
        };
        cache->chartResults = histories(byMd5);
        QMap<QString, QList<gameplay_logic::BmsResultCourse*>> byCourse;
        for (const auto& row : courses) {
            QList<gameplay_logic::BmsResult*> stages;
            for (const auto& guid : QString::fromStdString(row.scoreGuids)
                                      .split(' ', Qt::SkipEmptyParts))
                stages.append(cache->presentResults.contains(guid)
                                ? cache->results.value(guid)
                                : nullptr);
            auto result = gameplay_logic::BmsResultCourse::load(row, stages);
            auto* existing = cache->courseResults.value(result->getGuid());
            if (existing) {
                existing->updateFrom(*result);
            } else {
                result->setParent(this);
                existing = result.release();
                cache->courseResults.insert(existing->getGuid(), existing);
            }
            byCourse[existing->getIdentifier()].append(existing);
        }
        cache->courseHistories = histories(byCourse);
        cache->scores.clear();
        ++cache->revision;
        emit resultsChanged();
    } catch (const std::exception& e) {
        spdlog::error("Failed to refresh saved results: {}", e.what());
    }
}

const QVariantMap&
ScoreDb::getChartResults() const
{
    return cache->chartResults;
}

const QVariantMap&
ScoreDb::getCourseResults() const
{
    return cache->courseHistories;
}

void
ScoreDb::rememberScore(const gameplay_logic::BmsScore* score)
{
    const auto guid = score->getResult()->getGuid();
    if (guid.isEmpty())
        return;
    refreshResults({ guid });
    cache->scores.insert(
      "chart/" + guid, score->clone().release(), scoreCost(score));
}

void
ScoreDb::rememberCourseScore(const gameplay_logic::BmsScoreCourse* score)
{
    if (score->getResult()->getGuid().isEmpty())
        return;
    QStringList guids;
    for (auto* stage : score->getScores())
        guids.append(stage->getResult()->getGuid());
    refreshResults(guids);
    cache->scores.insert("course/" + score->getResult()->getGuid(),
                         score->clone().release(),
                         scoreCost(score));
}

void
ScoreDb::deliverScore(const support::PendingReplySource<QVariant>& source,
                      const QObject* score)
{
    auto* reply = source.reply();
    if (!reply || reply->isResultAvailable())
        return;
    (void)source.succeed(copyScore(score));
}

support::PendingReply*
ScoreDb::getScore(QObject* result)
{
    auto source = support::PendingReplySource<QVariant>{ this };
    auto* reply = source.reply();
    const auto* chart = qobject_cast<gameplay_logic::BmsResult*>(result);
    const auto* course = qobject_cast<gameplay_logic::BmsResultCourse*>(result);
    if (stopping || (!chart && !course)) {
        (void)source.fail();
        return reply;
    }
    const auto guid = chart ? chart->getGuid() : course->getGuid();
    const auto key =
      (course ? QStringLiteral("course/") : QStringLiteral("chart/")) + guid;
    auto job = cache->loads.value(key);
    if (!job) {
        job = std::make_shared<ScoreCache::Load>();
        job->guid = guid;
        job->course = course != nullptr;
        cache->loads.insert(key, job);
    }
    job->subscribers.append(source);
    source.setCancellationHandler([this, key, weak = std::weak_ptr(job)] {
        auto job = weak.lock();
        if (!job)
            return;
        const auto active =
          std::ranges::any_of(job->subscribers, [](const auto& subscriber) {
              auto* reply = subscriber.reply();
              return reply && !reply->isResultAvailable();
          });
        if (!active) {
            job->stop.request_stop();
            if (cache->loads.value(key) == job)
                cache->loads.remove(key);
        }
    });
    if (job->subscribers.size() == 1) {
        QMetaObject::invokeMethod(
          this,
          [this, key, job] {
              if (cache->loads.value(key) != job)
                  return;
              if (auto* score = cache->scores.object(key)) {
                  auto copy = copyScore(score);
                  cache->loads.remove(key);
                  const auto guard = QPointer<ScoreDb>(this);
                  for (const auto& subscriber : job->subscribers) {
                      if (!guard)
                          break;
                      guard->deliverScore(subscriber, copy.get());
                  }
              } else
                  startScoreLoad(key);
          },
          Qt::QueuedConnection);
    }
    return reply;
}

void
ScoreDb::startScoreLoad(const QString& key)
{
    const auto job = cache->loads.value(key);
    if (!job || stopping)
        return;
    const auto revision = cache->revision;
    const auto owner = QPointer<ScoreDb>(this);
    threadPool.start([this, owner, key, job, revision] {
        if (job->stop.stop_requested())
            return;
        std::unique_ptr<QObject> score;
        QString error;
        try {
            score = readFullScore(
              *scoreDb, job->guid, job->course, job->stop.get_token());
            if (job->stop.stop_requested())
                return;
            score->moveToThread(nullptr);
        } catch (const std::exception& e) {
            error = QString::fromUtf8(e.what());
        }
        // Queue on the application so discarded workers never outlive their
        // database.
        QMetaObject::invokeMethod(
          QCoreApplication::instance(),
          [owner,
           key,
           job,
           revision,
           score = std::move(score),
           error]() mutable {
              if (!owner || owner->stopping ||
                  owner->cache->loads.value(key) != job)
                  return;
              if (revision != owner->cache->revision) {
                  owner->startScoreLoad(key);
                  return;
              }
              owner->cache->loads.remove(key);
              if (!score) {
                  spdlog::error("Failed to load score {}: {}",
                                key.toStdString(),
                                error.toStdString());
                  for (const auto& subscriber : job->subscribers)
                      (void)subscriber.fail();
                  return;
              }
              score->moveToThread(owner->thread());
              auto held = std::move(score);
              const auto cost = scoreCost(held.get());
              if (cost <= owner->cache->scores.maxCost())
                  owner->cache->scores.insert(
                    key, copyScore(held.get()).release(), cost);
              for (const auto& subscriber : job->subscribers) {
                  if (!owner)
                      break;
                  owner->deliverScore(subscriber, held.get());
              }
          },
          Qt::QueuedConnection);
    });
}

ScoreDb::ScoreDb(db::SqliteCppDb* scoreDb)
  : scoreDb(scoreDb)
  , cache(std::make_unique<ScoreCache>())
{
    threadPool.setMaxThreadCount(1);
}

ScoreDb::~ScoreDb()
{
    stopping = true;
    const auto replyChildren =
      findChildren<support::PendingReply*>(Qt::FindDirectChildrenOnly);
    auto replies = QList<QPointer<support::PendingReply>>{};
    replies.reserve(replyChildren.size());
    for (auto* reply : replyChildren)
        replies.append(reply);

    for (const auto& reply : replies) {
        if (!reply)
            continue;
        reply->cancel();
        if (reply)
            reply->setParent(this);
    }
    threadPool.clear();
    threadPool.waitForDone();
}

auto
ScoreDb::makeStoppedReply() -> support::PendingReply*
{
    auto source = support::PendingReplySource<QVariant>{ this };
    auto* reply = source.reply();
    (void)source.fail();
    return reply;
}

auto
ScoreDb::getScoreSummary(const QString& folder) -> support::PendingReply*
{
    if (stopping)
        return makeStoppedReply();
    return runScoreQuery<QVariantMap>(
      this,
      threadPool,
      "Error in getScoreSummary",
      [this, folder](std::stop_token stopToken) {
          return getFolderScoreSummaryImpl(folder, stopToken);
      });
}

auto
ScoreDb::getScoreSummary(const resource_managers::Table& table)
  -> support::PendingReply*
{
    if (stopping)
        return makeStoppedReply();
    auto md5s = QStringList{};
    for (const auto& level : table.levels) {
        for (const auto& entry : level.entries)
            md5s.append(entry.md5);
    }
    return runScoreQuery<QVariantMap>(this,
                                      threadPool,
                                      "Error in getScoreSummary(table)",
                                      [this, md5s](std::stop_token stopToken) {
                                          return getScoreSummaryForMd5Impl(
                                            md5s, stopToken);
                                      });
}

auto
ScoreDb::getScoreSummary(const resource_managers::Level& level)
  -> support::PendingReply*
{
    if (stopping)
        return makeStoppedReply();
    auto md5s = QStringList{};
    for (const auto& entry : level.entries)
        md5s.append(entry.md5);
    return runScoreQuery<QVariantMap>(this,
                                      threadPool,
                                      "Error in getScoreSummary(level)",
                                      [this, md5s](std::stop_token stopToken) {
                                          return getScoreSummaryForMd5Impl(
                                            md5s, stopToken);
                                      });
}

auto
ScoreDb::getTotalStats() -> support::PendingReply*
{
    if (stopping)
        return makeStoppedReply();
    return runScoreQuery<ScoreStatsResult>(
      this,
      threadPool,
      "Error in getTotalStats",
      [this](std::stop_token stopToken) {
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
            "FROM score WHERE source = 0");
          const auto row =
            statement.executeAndGet<ScoreStatsRow>(stopToken).value_or(
              ScoreStatsRow{});
          return ScoreStatsResult{
              .playCount = row.playCount,
              .clearCount = row.clearCount,
              .failCount = row.failCount,
              .perfectCount = row.perfectCount,
              .greatCount = row.greatCount,
              .goodCount = row.goodCount,
              .badCount = row.badCount,
              .poorCount = row.poorCount,
              .maxCombo = row.maxCombo,
          };
      });
}
auto
ScoreDb::getTotalScoreCount() const -> int
{
    auto statement = scoreDb->createStatement("SELECT COUNT(*) FROM score");
    return statement.executeAndGet<int>().value_or(0);
}
} // namespace qml_components

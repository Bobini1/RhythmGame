//
// Created by bobini on 07.10.23.
//

#ifndef RHYTHMGAME_SCOREDB_H
#define RHYTHMGAME_SCOREDB_H

#include <functional>
#include <QThreadPool>
#include <QVariantMap>
#include "db/SqliteCppDb.h"
#include "gameplay_logic/BmsScore.h"
#include "resource_managers/Tables.h"
#include "support/PendingReply.h"
namespace gameplay_logic {
class BmsScoreCourse;
}
namespace qml_components {
class ScoreCache;

class ScoreStatsResult
{
    Q_GADGET
    Q_PROPERTY(qint64 playCount MEMBER playCount CONSTANT)
    Q_PROPERTY(qint64 clearCount MEMBER clearCount CONSTANT)
    Q_PROPERTY(qint64 failCount MEMBER failCount CONSTANT)
    Q_PROPERTY(qint64 perfectCount MEMBER perfectCount CONSTANT)
    Q_PROPERTY(qint64 greatCount MEMBER greatCount CONSTANT)
    Q_PROPERTY(qint64 goodCount MEMBER goodCount CONSTANT)
    Q_PROPERTY(qint64 badCount MEMBER badCount CONSTANT)
    Q_PROPERTY(qint64 poorCount MEMBER poorCount CONSTANT)
    Q_PROPERTY(qint64 maxCombo MEMBER maxCombo CONSTANT)
  public:
    qint64 playCount{};
    qint64 clearCount{};
    qint64 failCount{};
    qint64 perfectCount{};
    qint64 greatCount{};
    qint64 goodCount{};
    qint64 badCount{};
    qint64 poorCount{};
    qint64 maxCombo{};
};

/**
 * @brief Provides cached results and asynchronous full scores for one profile.
 * @details Results are loaded into memory when the profile opens and refreshed
 * after saves and imports. chartResults and courseResults update QML bindings
 * automatically, without SQL or replay decoding when read.
 * Result objects keep their identity and live as long as this profile's
 * ScoreDb. Imports update their properties in place. Deleted runs leave the
 * maps, but retained objects remain readable until the profile closes.
 * Full-score queries execute in the thread pool. Each returned PendingReply
 * owns its cancellation; callers retain and cancel only the work they started.
 * Full scores returned to QML live independently of their reply. For
 * C++-only queries, score objects are children of the reply and share its
 * lifetime.
 */
class ScoreDb final : public QObject
{
    Q_OBJECT
    /** Chart MD5s (upper-case) mapped to BmsResult lists, newest first. */
    Q_PROPERTY(
      QVariantMap chartResults READ getChartResults NOTIFY resultsChanged)
    /** Course identifiers mapped to BmsResultCourse lists, newest first. */
    Q_PROPERTY(
      QVariantMap courseResults READ getCourseResults NOTIFY resultsChanged)

    db::SqliteCppDb* scoreDb;
    QThreadPool threadPool;
    bool stopping = false;
    std::unique_ptr<ScoreCache> cache;
    void startScoreLoad(const QString& key);
    void deliverScore(const support::PendingReplySource<QVariant>& source,
                      const QObject* score);
    auto getScoreSummaryForMd5Impl(const QList<QString>& md5s,
                                   std::stop_token stopToken) const
      -> QVariantMap;
    auto getFolderScoreSummaryImpl(const QString& folder,
                                   std::stop_token stopToken) const
      -> QVariantMap;
    auto makeStoppedReply() -> support::PendingReply*;

  public:
    explicit ScoreDb(db::SqliteCppDb* scoreDb);
    ~ScoreDb() override;
    /**
     * Loads a BmsScore or BmsScoreCourse for a result from this profile.
     * Concurrent requests share decoding but have independent cancellation and
     * object lifetimes. Decoded lists share their backing storage. An idle
     * cache retains up to 64 MiB of estimated payload; live consumers may
     * retain more. Missing records and malformed attachments fail the reply.
     * Imported scores may succeed without a recording or gauge history.
     * ScoreDetails supplies a declarative QML interface to this operation.
     */
    Q_INVOKABLE support::PendingReply* getScore(QObject* result);
    const QVariantMap& getChartResults() const;
    const QVariantMap& getCourseResults() const;
    /** Refresh after a committed import, or just the changed run after saving.
     */
    void refreshResults(const QStringList& guids = {});
    void rememberScore(const gameplay_logic::BmsScore* score);
    void rememberCourseScore(const gameplay_logic::BmsScoreCourse* score);
    Q_INVOKABLE support::PendingReply* getScoreSummary(const QString& folder);
    Q_INVOKABLE support::PendingReply* getScoreSummary(
      const resource_managers::Table& table);
    Q_INVOKABLE support::PendingReply* getScoreSummary(
      const resource_managers::Level& level);
    Q_INVOKABLE support::PendingReply* getTotalStats();

    Q_INVOKABLE int getTotalScoreCount() const;
  signals:
    void resultsChanged();
};
} // namespace qml_components

#endif // RHYTHMGAME_SCOREDB_H

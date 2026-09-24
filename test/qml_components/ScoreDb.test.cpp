#include "qml_components/ScoreDb.h"
#include "gameplay_logic/BmsScoreCourse.h"
#include "resource_managers/DefineDb.h"
#include "resource_managers/ChartDataFactory.h"
#include "resource_managers/Profile.h"
#include "resource_managers/SongAssetStore.h"
#include "support/QStringToPath.h"
#include "support/Version.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJSEngine>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QTemporaryDir>
#include <QThread>
#include <catch2/catch_test_macros.hpp>
#include <magic_enum/magic_enum.hpp>

namespace {
struct Scores
{
    QTemporaryDir directory;
    std::filesystem::path songPath =
      support::qStringToPath(directory.filePath("songs.sqlite"));
    std::filesystem::path scorePath =
      support::qStringToPath(directory.filePath("scores.sqlite"));
    db::SqliteCppDb songs{ songPath };
    std::unique_ptr<QNetworkAccessManager> network;
    resource_managers::SongAssetStore assets;
    std::unique_ptr<resource_managers::Profile> profile;

    Scores()
    {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char name[] = "ScoreDb.test";
            static char* argv[] = { name, nullptr };
            static QCoreApplication application(argc, argv);
        }
        network = std::make_unique<QNetworkAccessManager>();
        resource_managers::defineDb(songs);
        openProfile();
    }

    void openProfile()
    {
        profile = std::make_unique<resource_managers::Profile>(
          songPath,
          scorePath,
          QMap<QString, qml_components::ThemeFamily>{},
          QList<QString>{},
          network.get(),
          &assets);
    }

    void save()
    {
        const auto guid = QStringLiteral("score-1");
        auto result = std::make_unique<gameplay_logic::BmsResult>(
          200.0,
          100,
          100,
          0,
          0,
          0,
          0,
          QStringLiteral("HARD"),
          QList<int>(magic_enum::enum_count<gameplay_logic::Judgement>()),
          0,
          150.0,
          80,
          1234,
          10000,
          QList<qint64>{},
          0,
          resource_managers::NoteOrderAlgorithm::Normal,
          resource_managers::NoteOrderAlgorithm::Normal,
          resource_managers::DpOptions::Off,
          gameplay_logic::ChartData::Keymode::K7,
          guid,
          QStringLiteral("SHA256"),
          QStringLiteral("MD5"));
        gameplay_logic::BmsScore score(
          std::move(result),
          std::make_unique<gameplay_logic::BmsReplayData>(
            QList<gameplay_logic::HitEvent>{}, guid),
          std::make_unique<gameplay_logic::BmsGaugeHistory>(
            QList<gameplay_logic::BmsGaugeInfo>{}, guid));
        score.save(profile->getDb());
        profile->getDb().execute(
          "INSERT INTO score_course(guid, identifier, score_guids, clear_type, "
          "max_combo, constraints, unix_timestamp, game_version) VALUES "
          "('course-1', 'COURSE', 'score-1', 'HARD', 80, '', 1234, 0)");
    }
};

void
waitFor(support::PendingReply* reply)
{
    QElapsedTimer timer;
    timer.start();
    while (!reply->isResultAvailable() && timer.elapsed() < 5000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    REQUIRE(reply->isResultAvailable());
    REQUIRE(reply->isSuccessful());
}

void
collect(QJSEngine& engine)
{
    for (int i = 0; i < 3; ++i) {
        engine.collectGarbage();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
}
}

TEST_CASE("Profile migrations are atomic and score history uses indexes",
          "[Database][ScoreDb]")
{
    Scores fixture;
    auto& db = fixture.profile->getDb();
    CHECK(db.createStatement("PRAGMA synchronous").executeAndGet<int>() == 2);
    CHECK(fixture.songs.createStatement("PRAGMA synchronous")
            .executeAndGet<int>() == 1);
    for (const auto* query :
         { "EXPLAIN QUERY PLAN SELECT * FROM score WHERE md5 = 'MD5' "
           "ORDER BY unix_timestamp DESC",
           "EXPLAIN QUERY PLAN SELECT * FROM score_course WHERE identifier = "
           "'COURSE' "
           "ORDER BY unix_timestamp DESC" }) {
        const auto plan =
          db.createStatement(query)
            .executeAndGetAll<std::tuple<int, int, int, std::string>>();
        REQUIRE(plan.size() == 1);
        CHECK(std::get<3>(plan.front()).find("USING INDEX") !=
              std::string::npos);
    }
    fixture.profile.reset();
    {
        db::SqliteCppDb old(fixture.scorePath);
        old.execute("ALTER TABLE score DROP COLUMN source");
        auto version = old.createStatement(
          "UPDATE properties SET value = ? WHERE key = 'version'");
        version.bind(1, static_cast<int64_t>(support::packVersion(1, 3, 5)));
        version.execute();
        old.execute("CREATE TRIGGER reject_version BEFORE INSERT ON properties "
                    "WHEN new.key = 'version' BEGIN "
                    "SELECT RAISE(ABORT, 'injected migration failure'); END");
        REQUIRE_THROWS(fixture.openProfile());
        CHECK(
          old
            .createStatement("SELECT count(*) FROM pragma_table_info('score') "
                             "WHERE name = 'source'")
            .executeAndGet<int>() == 0);
        CHECK(old
                .createStatement(
                  "SELECT value FROM properties WHERE key = 'version'")
                .executeAndGet<int64_t>() == support::packVersion(1, 3, 5));
        old.execute("DROP TRIGGER reject_version");
    }
    REQUIRE_NOTHROW(fixture.openProfile());
    CHECK(fixture.profile->getDb()
            .createStatement("SELECT count(*) FROM pragma_table_info('score') "
                             "WHERE name = 'source'")
            .executeAndGet<int>() == 1);
}

TEST_CASE("Folder scores include exact additional memberships",
          "[ScoreDb][folders]")
{
    Scores fixture;
    fixture.save();
    const auto folder = QStringLiteral("catalog:/packs/Pack%2F100%25/");
    fixture.songs.execute("INSERT INTO parent_dir(id, dir) VALUES "
                          "(1, 'catalog:/packs/Pack%2F100%25/'), "
                          "(2, 'catalog:/packs/Packx2F100x25/')");
    for (const auto* md5 : { "MD5", "UNPLAYED", "OUTSIDE" }) {
        const resource_managers::ChartDataFactory factory;
        const auto path = QStringLiteral("C:/songs/%1/chart.bms").arg(md5);
        auto components = factory.loadChartData(
          "#PLAYER 1\n#TITLE Test\n#ARTIST Test\n#BPM 120\n#00111:0100\n",
          support::qStringToPath(path),
          [](auto) { return 1; },
          -1);
        components.chartData->save(fixture.songs);
        auto update = fixture.songs.createStatement(
          "UPDATE charts SET md5 = ? WHERE path = ?");
        update.bind(1, md5);
        update.bind(2, path.toStdString());
        update.execute();
    }
    fixture.songs.execute(
      "INSERT INTO folder_charts(directory, chart_id) "
      "SELECT CASE md5 WHEN 'OUTSIDE' THEN 2 ELSE 1 END, id FROM charts");

    const auto summaryReply = std::unique_ptr<support::PendingReply>(
      fixture.profile->getScoreDb()->getScoreSummary(folder));
    waitFor(summaryReply.get());
    const auto summary = summaryReply->value().toMap();
    const auto counts = summary.value("counts").toMap();
    CHECK(counts.value("HARD").toInt() == 1);
    CHECK(counts.value("NOPLAY").toInt() == 1);

    const auto scoresReply = std::unique_ptr<support::PendingReply>(
      fixture.profile->getScoreDb()->getScores(folder));
    waitFor(scoresReply.get());
    const auto scores =
      scoresReply->value().value<qml_components::ScoreQueryResult>();
    CHECK(scores.unplayed == 1);
    REQUIRE(scores.scores.size() == 1);
    REQUIRE(scores.scores.contains("MD5"));
    CHECK(scores.scores.value("MD5").toList().size() == 1);
}

TEST_CASE("QML score lists retain their payload independently of replies",
          "[ScoreDb]")
{
    Scores fixture;
    fixture.save();
    QJSEngine engine;
    support::PendingReply* reply = nullptr;
    QString key;
    SECTION("Chart scores")
    {
        reply = fixture.profile->getScoreDb()->getScoresForMd5({ "MD5" });
        key = "MD5";
    }
    SECTION("Course scores")
    {
        reply =
          fixture.profile->getScoreDb()->getScoresForCourseId({ "COURSE" });
        key = "COURSE";
    }
    engine.globalObject().setProperty("reply", engine.newQObject(reply));
    waitFor(reply);
    QPointer<QObject> score = reply->value()
                                .value<qml_components::ScoreQueryResult>()
                                .scores.value(key)
                                .toList()
                                .front()
                                .value<QJSValue>()
                                .toQObject();
    REQUIRE(score);
    CHECK(score->thread() == QCoreApplication::instance()->thread());
    engine.evaluate("var kept = reply.value.scores; reply = null;");
    delete reply;
    collect(engine);
    REQUIRE(score);
    engine.globalObject().setProperty("key", key);
    REQUIRE(engine.evaluate("kept[key][0].result.guid.length > 0").toBool());
    engine.evaluate("var keptScore = kept[key][0]; kept = null;");
    collect(engine);
    REQUIRE(score);
    engine.evaluate("keptScore = null;");
    collect(engine);
    CHECK(score.isNull());
}

TEST_CASE("Score browsing does not wait for an uncommitted score import",
          "[ScoreDb][scheduling]")
{
    Scores fixture;
    fixture.save();
    auto transaction = fixture.profile->getDb().transaction();
    fixture.profile->getDb().execute("UPDATE score SET md5 = 'CHANGED'");
    auto query = std::unique_ptr<support::PendingReply>(
      fixture.profile->getScoreDb()->getScoresForMd5({ "MD5" }));
    waitFor(query.get());
    CHECK(
      query->value().value<qml_components::ScoreQueryResult>().scores.contains(
        "MD5"));
    transaction.commit();
    query.reset(fixture.profile->getScoreDb()->getScoresForMd5({ "MD5" }));
    waitFor(query.get());
    CHECK(query->value()
            .value<qml_components::ScoreQueryResult>()
            .scores.isEmpty());
}

TEST_CASE("Unconsumed score results are reclaimed with the reply", "[ScoreDb]")
{
    Scores fixture;
    fixture.save();
    QJSEngine engine;
    auto* reply = fixture.profile->getScoreDb()->getScoresForMd5({ "MD5" });
    bool qml = false;
    SECTION("C++ caller") {}
    SECTION("QML caller")
    {
        qml = true;
        engine.globalObject().setProperty("reply", engine.newQObject(reply));
    }
    waitFor(reply);
    auto scoreValue = reply->value()
                        .value<qml_components::ScoreQueryResult>()
                        .scores.value("MD5")
                        .toList()
                        .front();
    QPointer<QObject> score = qml ? scoreValue.value<QJSValue>().toQObject()
                                  : scoreValue.value<QObject*>();
    REQUIRE(score);
    QPointer<QObject> result =
      qobject_cast<gameplay_logic::BmsScore*>(score.data())->getResult();
    scoreValue = {};
    if (qml) {
        QPointer<support::PendingReply> replyGuard(reply);
        engine.evaluate("reply = null;");
        collect(engine);
        CHECK(replyGuard.isNull());
    } else {
        delete reply;
    }
    CHECK(score.isNull());
    CHECK(result.isNull());
}

#include "qml_components/ScoreDb.h"
#include "Lr2SelectItemModel.h"
#include "resource_managers/ChartFolderModel.h"
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
#include <QMetaEnum>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
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

    void save(QString guid = "score-1",
              double points = 150,
              int64_t timestamp = 1234)
    {
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
          points,
          80,
          timestamp,
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
          "INSERT OR IGNORE INTO score_course(guid, identifier, score_guids, "
          "clear_type, "
          "max_combo, constraints, unix_timestamp, game_version) VALUES "
          "('course-1', 'COURSE', 'score-1', 'HARD', 80, '', 1234, 0)");
    }
};

void
waitFor(support::PendingReply* reply, bool success = true)
{
    QElapsedTimer timer;
    timer.start();
    while (!reply->isResultAvailable() && timer.elapsed() < 5000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    REQUIRE(reply->isResultAvailable());
    REQUIRE(reply->isSuccessful() == success);
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
}

TEST_CASE("Cached histories update QML bindings and retain result identities",
          "[ScoreDb][results][qml]")
{
    Scores fixture;
    auto* db = fixture.profile->getScoreDb();
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        QtObject {
            id: root
            required property var profile
            property string hash: "MD5"
            readonly property var charts: root.profile?.scoreDb.chartResults ?? ({})
            readonly property var courses: root.profile?.scoreDb.courseResults ?? ({})
            readonly property var results: root.charts[root.hash] || []
            readonly property var courseResults: root.courses["COURSE"] || []
            readonly property int count: root.results.length
            readonly property real points: root.results.reduce((sum, result) => sum + result.points, 0)
            property var kept: null
            property var keptCourse: null
            property var keptStage: null
            readonly property real keptPoints: root.kept?.points ?? 0
            readonly property real coursePoints: root.keptCourse?.points ?? 0
            readonly property real stagePoints: root.keptStage?.points ?? 0
        }
    )",
                      QUrl("qrc:/CachedResultsTest.qml"));
    INFO(component.errorString().toStdString());
    REQUIRE(component.isReady());
    auto object =
      std::unique_ptr<QObject>(component.createWithInitialProperties(
        { { "profile", QVariant::fromValue(fixture.profile.get()) } }));
    REQUIRE(object);
    engine.globalObject().setProperty("h", engine.newQObject(object.get()));
    CHECK(object->property("count").toInt() == 0);

    fixture.save();
    db->refreshResults();
    CHECK(object->property("count").toInt() == 1);
    CHECK(object->property("points").toDouble() == 150);
    engine.evaluate("h.kept = h.results[0]; h.keptCourse = h.courseResults[0]; "
                    "h.keptStage = h.keptCourse.results[0];");
    QPointer<QObject> kept = engine.evaluate("h.kept").toQObject();
    QPointer<QObject> keptCourse = engine.evaluate("h.keptCourse").toQObject();
    QPointer<QObject> keptStage = engine.evaluate("h.keptStage").toQObject();
    REQUIRE(kept);
    REQUIRE(keptCourse);
    REQUIRE(keptStage);

    fixture.profile->getDb().execute("UPDATE score SET points = 175");
    db->refreshResults({ "score-1" });
    CHECK(engine
            .evaluate(
              "h.kept === h.results[0] && h.keptCourse === h.courseResults[0] "
              "&& h.keptStage === h.keptCourse.results[0]")
            .toBool());
    CHECK(object->property("points").toDouble() == 175);
    CHECK(object->property("keptPoints").toDouble() == 175);
    CHECK(object->property("coursePoints").toDouble() == 175);
    CHECK(object->property("stagePoints").toDouble() == 175);

    const auto children = db->findChildren<gameplay_logic::BmsResult*>().size();
    db->refreshResults();
    db->refreshResults();
    CHECK(db->findChildren<gameplay_logic::BmsResult*>().size() == children);
    fixture.save("score-2", 190, 2000);
    db->refreshResults({ "score-2" });
    CHECK(object->property("count").toInt() == 2);
    CHECK(object->property("points").toDouble() == 365);
    CHECK(
      engine
        .evaluate("h.results[0].guid === 'score-2' && h.results[1] === h.kept")
        .toBool());

    // Reading a cached history does not acquire a database connection.
    auto transaction = fixture.profile->getDb().transaction();
    fixture.profile->getDb().execute("UPDATE score SET md5 = 'NEW'");
    CHECK(db->getChartResults().value("MD5").toList().size() == 2);
    transaction.commit();
    db->refreshResults();
    CHECK(object->property("count").toInt() == 0);
    object->setProperty("hash", "NEW");
    CHECK(object->property("count").toInt() == 2);

    fixture.profile->getDb().execute("DELETE FROM replay_data");
    fixture.profile->getDb().execute("DELETE FROM gauge_history");
    fixture.profile->getDb().execute("DELETE FROM score");
    db->refreshResults();
    collect(engine);
    CHECK(object->property("count").toInt() == 0);
    REQUIRE(kept);
    CHECK(object->property("keptPoints").toDouble() == 175);
    CHECK(
      engine
        .evaluate("!h.keptCourse.complete && h.keptCourse.results[0] === null")
        .toBool());
    REQUIRE(keptStage);
    CHECK(object->property("stagePoints").toDouble() == 175);
    fixture.profile.reset();
    CHECK(kept.isNull());
    CHECK(keptCourse.isNull());
    CHECK(keptStage.isNull());
}

TEST_CASE("Unconsumed full scores are reclaimed with the reply", "[ScoreDb]")
{
    Scores fixture;
    fixture.save();
    auto* db = fixture.profile->getScoreDb();
    db->refreshResults();
    QJSEngine engine;
    auto* reply = db->getScore(
      db->getChartResults().value("MD5").toList().front().value<QObject*>());
    bool qml = false;
    SECTION("C++ caller") {}
    SECTION("QML caller")
    {
        qml = true;
        engine.globalObject().setProperty("reply", engine.newQObject(reply));
    }
    waitFor(reply);
    auto scoreValue = reply->value();
    INFO(scoreValue.typeName());
    QPointer<QObject> score = scoreValue.value<QObject*>();
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

TEST_CASE("Profile startup caches results without decoding recordings",
          "[ScoreDb][results]")
{
    Scores fixture;
    fixture.save();
    fixture.profile->getDb().execute(
      "UPDATE replay_data SET replay_data = X'00'");
    fixture.profile.reset();
    REQUIRE_NOTHROW(fixture.openProfile());
    auto* db = fixture.profile->getScoreDb();
    const auto charts = db->getChartResults();
    REQUIRE(charts.value("MD5").toList().size() == 1);
    CHECK(charts.value("MISSING").toList().isEmpty());
    auto* result =
      charts.value("MD5").toList().front().value<gameplay_logic::BmsResult*>();
    REQUIRE(result);
    CHECK(result->getPoints() == 150);
    CHECK(result->isReplayAvailable());
    CHECK(result->isGaugeHistoryAvailable());
    CHECK_FALSE(result->isImported());
    CHECK(result->children().isEmpty());

    auto* course = db->getCourseResults()
                     .value("COURSE")
                     .toList()
                     .front()
                     .value<gameplay_logic::BmsResultCourse*>();
    REQUIRE(course);
    CHECK(course->getPoints() == 150);
    CHECK(course->isReplayAvailable());
    CHECK(course->findChildren<gameplay_logic::BmsReplayData*>().isEmpty());

    // Cached lookups remain usable even when the database cannot answer them.
    fixture.profile->getDb().execute("DROP TABLE replay_data");
    fixture.profile->getDb().execute("DROP TABLE gauge_history");
    fixture.profile->getDb().execute("DROP TABLE score_course");
    fixture.profile->getDb().execute("DROP TABLE score");
    CHECK(db->getChartResults().value("MD5").toList().size() == 1);
    CHECK(db->getCourseResults().value("COURSE").toList().size() == 1);
}

TEST_CASE("Result refresh updates imported metadata in place and invalidates "
          "full scores",
          "[ScoreDb][results]")
{
    Scores fixture;
    fixture.save();
    auto* db = fixture.profile->getScoreDb();
    db->refreshResults();
    auto* before =
      db->getChartResults().value("MD5").toList().front().value<QObject*>();
    auto first = std::unique_ptr<support::PendingReply>(db->getScore(before));
    waitFor(first.get());
    fixture.profile->getDb().execute(
      "UPDATE score SET md5 = 'NEW', points = 175, source = 1");
    fixture.profile->getDb().execute("DELETE FROM replay_data");
    fixture.profile->getDb().execute("DELETE FROM gauge_history");
    db->refreshResults({ "score-1" });
    CHECK(db->getChartResults().value("MD5").toList().isEmpty());
    auto* result = db->getChartResults()
                     .value("NEW")
                     .toList()
                     .front()
                     .value<gameplay_logic::BmsResult*>();
    REQUIRE(result);
    CHECK(result == before);
    CHECK(result->isImported());
    CHECK(result->getSourceName() == "LR2");
    CHECK_FALSE(result->isReplayAvailable());
    auto reply = std::unique_ptr<support::PendingReply>(db->getScore(result));
    waitFor(reply.get());
    auto* score =
      qobject_cast<gameplay_logic::BmsScore*>(reply->value().value<QObject*>());
    REQUIRE(score);
    CHECK(score->getResult()->getPoints() == 175);
    CHECK_FALSE(score->getReplayData());
    CHECK_FALSE(score->getGaugeHistory());
    CHECK(
      qobject_cast<gameplay_logic::BmsScore*>(first->value().value<QObject*>())
        ->getResult()
        ->getPoints() == 150);
}

TEST_CASE("Full score subscribers cancel independently and share decoded data",
          "[ScoreDb][details]")
{
    Scores fixture;
    fixture.save();
    const auto event = gameplay_logic::HitEvent(
      0,
      0,
      123,
      gameplay_logic::BmsPoints(2, gameplay_logic::Judgement::Perfect, -20),
      gameplay_logic::HitEvent::Action::Press,
      true);
    gameplay_logic::BmsReplayData replay({ event }, "score-1");
    fixture.profile->getDb().execute("DELETE FROM replay_data");
    replay.save(fixture.profile->getDb());
    auto* db = fixture.profile->getScoreDb();
    db->refreshResults();
    auto* result =
      db->getChartResults().value("MD5").toList().front().value<QObject*>();
    auto cancelled =
      std::unique_ptr<support::PendingReply>(db->getScore(result));
    auto first = std::unique_ptr<support::PendingReply>(db->getScore(result));
    auto second = std::unique_ptr<support::PendingReply>(db->getScore(result));
    cancelled->cancel();
    waitFor(first.get());
    waitFor(second.get());
    CHECK_FALSE(cancelled->isSuccessful());
    auto* a =
      qobject_cast<gameplay_logic::BmsScore*>(first->value().value<QObject*>());
    auto* b = qobject_cast<gameplay_logic::BmsScore*>(
      second->value().value<QObject*>());
    REQUIRE(a);
    REQUIRE(b);
    REQUIRE(a != b);
    const auto eventsA = a->getReplayData()->getHitEvents();
    const auto eventsB = b->getReplayData()->getHitEvents();
    REQUIRE(eventsA.size() == 1);
    CHECK(eventsA.constData() == eventsB.constData());
    CHECK(a->getReplayData()->getTotalEarly() ==
          b->getReplayData()->getTotalEarly());

    // A cache hit needs no second decode, even if the stored blob becomes bad.
    fixture.profile->getDb().execute(
      "UPDATE replay_data SET replay_data = X'00'");
    auto cached = std::unique_ptr<support::PendingReply>(db->getScore(result));
    waitFor(cached.get());
    CHECK(
      qobject_cast<gameplay_logic::BmsScore*>(cached->value().value<QObject*>())
        ->getReplayData()
        ->getHitEvents()
        .constData() == eventsA.constData());
}

TEST_CASE(
  "Course results preserve stage slots and full scores combine timelines",
  "[ScoreDb][details][course]")
{
    Scores fixture;
    fixture.save();
    fixture.profile->getDb().execute(
      "UPDATE score_course SET score_guids = 'score-1 score-1'");
    const auto event = gameplay_logic::HitEvent(
      0,
      0,
      123,
      gameplay_logic::BmsPoints(2, gameplay_logic::Judgement::Perfect, -20),
      gameplay_logic::HitEvent::Action::Press,
      true);
    gameplay_logic::BmsReplayData replay({ event }, "score-1");
    fixture.profile->getDb().execute("DELETE FROM replay_data");
    replay.save(fixture.profile->getDb());
    gameplay_logic::BmsGaugeHistory gauges(
      { gameplay_logic::BmsGaugeInfo{
        100, 80, "NORMAL", false, { { 123, 50 } } } },
      "score-1");
    fixture.profile->getDb().execute("DELETE FROM gauge_history");
    gauges.save(fixture.profile->getDb());
    auto* db = fixture.profile->getScoreDb();
    db->refreshResults();
    auto* result = db->getCourseResults()
                     .value("COURSE")
                     .toList()
                     .front()
                     .value<gameplay_logic::BmsResultCourse*>();
    REQUIRE(result);
    REQUIRE(result->getResults().size() == 2);
    CHECK(result->isComplete());
    CHECK(result->getPoints() == 300);
    auto reply = std::unique_ptr<support::PendingReply>(db->getScore(result));
    waitFor(reply.get());
    auto* score = qobject_cast<gameplay_logic::BmsScoreCourse*>(
      reply->value().value<QObject*>());
    REQUIRE(score);
    REQUIRE(score->getScores().size() == 2);
    CHECK(score->getScores()[0] != score->getScores()[1]);
    const auto events = score->getReplayData()->getHitEvents();
    REQUIRE(events.size() == 2);
    CHECK(events[0].getOffsetFromStart() == 123);
    CHECK(events[1].getOffsetFromStart() == 10123);
    const auto history =
      score->getGaugeHistory()->getGaugeInfo()[0].gaugeHistory;
    REQUIRE(history.size() == 2);
    CHECK(history[1].getOffsetFromStart() == 10123);
    auto copy = score->clone();
    CHECK(copy->getReplayData()->getHitEvents().constData() ==
          events.constData());

    fixture.profile->getDb().execute(
      "UPDATE score_course SET score_guids = 'missing score-1 missing'");
    db->refreshResults();
    result = db->getCourseResults()
               .value("COURSE")
               .toList()
               .front()
               .value<gameplay_logic::BmsResultCourse*>();
    REQUIRE(result->getResults().size() == 3);
    CHECK_FALSE(result->getResults()[0]);
    REQUIRE(result->getResults()[1]);
    CHECK_FALSE(result->getResults()[2]);
    CHECK_FALSE(result->isComplete());
    CHECK_FALSE(result->isReplayAvailable());
    CHECK(result->getUnixTimestamp() == 1234);
    auto incomplete =
      std::unique_ptr<support::PendingReply>(db->getScore(result));
    waitFor(incomplete.get(), false);
}

TEST_CASE(
  "QML can retain one loaded score after cache refresh and reply destruction",
  "[ScoreDb][details]")
{
    Scores fixture;
    fixture.save();
    auto* db = fixture.profile->getScoreDb();
    db->refreshResults();
    auto* result =
      db->getChartResults().value("MD5").toList().front().value<QObject*>();
    QJSEngine engine;
    auto* reply = db->getScore(result);
    engine.globalObject().setProperty("reply", engine.newQObject(reply));
    waitFor(reply);
    const auto value = reply->value();
    INFO(value.typeName());
    QPointer<QObject> score = value.value<QObject*>();
    REQUIRE(score);
    engine.evaluate("var kept = reply.value; reply = null;");
    delete reply;
    db->refreshResults();
    collect(engine);
    REQUIRE(score);
    CHECK(
      engine.evaluate("kept.result.points === 150 && kept.replayData !== null")
        .toBool());
    engine.evaluate("kept = null;");
    collect(engine);
    CHECK(score.isNull());
}

TEST_CASE("ScoreDetails follows its inputs and recovers from a failed load",
          "[ScoreDb][ScoreDetails][qml]")
{
    Scores fixture;
    fixture.save();
    auto* db = fixture.profile->getScoreDb();
    db->refreshResults();
    auto* result =
      db->getChartResults().value("MD5").toList().front().value<QObject*>();
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import RhythmGameQml\nScoreDetails { profile: null }",
                      QUrl("qrc:/ScoreDetailsTest.qml"));
    INFO(component.errorString().toStdString());
    REQUIRE(component.isReady());
    auto details =
      std::unique_ptr<QObject>(component.createWithInitialProperties(
        { { "profile", QVariant::fromValue(fixture.profile.get()) },
          { "result", QVariant::fromValue(result) } }));
    INFO(component.errorString().toStdString());
    REQUIRE(details);
    engine.globalObject().setProperty("details",
                                      engine.newQObject(details.get()));
    const auto statusEnum = details->metaObject()->enumerator(
      details->metaObject()->indexOfEnumerator("Status"));
    REQUIRE(statusEnum.isValid());
    const auto awaitStatus = [&](const char* name) {
        QElapsedTimer timer;
        timer.start();
        const auto expected = statusEnum.keyToValue(name);
        REQUIRE(expected >= 0);
        while (details->property("status").toInt() != expected &&
               timer.elapsed() < 5000) {
            QCoreApplication::processEvents();
            QThread::msleep(1);
        }
        INFO("Expected ScoreDetails." << name);
        REQUIRE(details->property("status").toInt() == expected);
    };
    engine.globalObject().setProperty("DetailsIdle",
                                      statusEnum.keyToValue("Idle"));
    awaitStatus("Ready");
    CHECK(engine.evaluate("details.score.result.guid === 'score-1'").toBool());
    details->setProperty("enabled", false);
    CHECK(
      engine
        .evaluate("details.score === null && details.status === DetailsIdle")
        .toBool());
    details->setProperty("enabled", true);
    awaitStatus("Ready");

    // Refreshing the database reloads details even when the result keeps its
    // identity.
    fixture.profile->getDb().execute(
      "UPDATE replay_data SET replay_data = X'00'");
    db->refreshResults();
    result =
      db->getChartResults().value("MD5").toList().front().value<QObject*>();
    details->setProperty("result", QVariant::fromValue(result));
    awaitStatus("Error");
    fixture.profile->getDb().execute("DELETE FROM replay_data");
    gameplay_logic::BmsReplayData({}, "score-1").save(fixture.profile->getDb());
    REQUIRE(QMetaObject::invokeMethod(details.get(), "reload"));
    awaitStatus("Ready");
    details->setProperty("result",
                         QVariant::fromValue(static_cast<QObject*>(nullptr)));
    CHECK(
      engine
        .evaluate("details.score === null && details.status === DetailsIdle")
        .toBool());
}

TEST_CASE("A queued detail request observes refreshed results and tolerates "
          "profile teardown",
          "[ScoreDb][details]")
{
    Scores fixture;
    fixture.save();
    auto* db = fixture.profile->getScoreDb();
    db->refreshResults();
    auto* result =
      db->getChartResults().value("MD5").toList().front().value<QObject*>();
    auto reply = std::unique_ptr<support::PendingReply>(db->getScore(result));
    fixture.profile->getDb().execute("UPDATE score SET points = 180");
    db->refreshResults();
    waitFor(reply.get());
    auto* score =
      qobject_cast<gameplay_logic::BmsScore*>(reply->value().value<QObject*>());
    REQUIRE(score);
    CHECK(score->getResult()->getPoints() == 180);

    result =
      db->getChartResults().value("MD5").toList().front().value<QObject*>();
    QPointer<support::PendingReply> pending = db->getScore(result);
    fixture.profile.reset();
    CHECK(pending.isNull());
    QCoreApplication::processEvents();
    CHECK(score->getResult()->getPoints() == 180);
}

TEST_CASE("Selection sorting and LR2 summaries accept cached result objects",
          "[ScoreDb][results][select]")
{
    Scores fixture;
    fixture.save();
    auto* db = fixture.profile->getScoreDb();
    db->refreshResults();
    const auto records = db->getChartResults();
    const auto scored = QVariant::fromValue(
      resource_managers::Entry{ .title = "Z", .md5 = "MD5" });
    const auto unplayed = QVariant::fromValue(
      resource_managers::Entry{ .title = "A", .md5 = "EMPTY" });
    const QVariantList items{ unplayed, scored };
    resource_managers::ChartFolderModel sorting;
    sorting.setScores(records);
    for (const auto mode : { resource_managers::SelectSortMode::ScoreRate,
                             resource_managers::SelectSortMode::ClearLamp,
                             resource_managers::SelectSortMode::MissCount }) {
        sorting.setSortMode(static_cast<int>(mode));
        const auto sorted = sorting.filterAndSort(items);
        REQUIRE(sorted.size() == 2);
        CHECK(sorted[0].value<resource_managers::Entry>().md5 == "MD5");
    }
    Lr2SelectItemModel presentation;
    presentation.setItems(items);
    presentation.setScoreSummariesFromScores(records);
    CHECK(presentation
            .data(presentation.index(1, 0), Lr2SelectItemModel::ScoreRateRole)
            .toDouble() == 0.75);
    CHECK(
      presentation.data(presentation.index(1, 0), Lr2SelectItemModel::LampRole)
        .toInt() > 0);
}

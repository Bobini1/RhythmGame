#include "ScreenContexts.h"
#include "RegisterGameplayTestTypes.h"
#include "../arena/FakeArenaIdentityProvider.h"
#include "../arena/FakeArenaScheduler.h"
#include "../arena/FakeArenaTransport.h"
#include "resource_managers/DefineDb.h"
#include "resource_managers/SongAssetStore.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>
#include <QCoreApplication>
#include <QEvent>
#include <QNetworkAccessManager>
#include <QPromise>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <memory>
#include <vector>

namespace {
using namespace gameplay_logic;
using namespace rhythm_game_qml;

void
ensureContextApplication()
{
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static char name[] = "ScreenContexts.test";
        static char* argv[] = { name, nullptr };
        static auto* application = new QCoreApplication(argc, argv);
        (void)application;
    }
}

void
registerContextTestTypes()
{
    registerGameplayTestTypes();
    static const auto registration = [] {
        qmlRegisterUncreatableType<Player>(
          "ScreenContractsTest", 1, 0, "Player", "runner supplied");
        qmlRegisterUncreatableType<CoursePlayer>(
          "ScreenContractsTest", 1, 0, "CoursePlayer", "runner supplied");
        qmlRegisterUncreatableType<ChartData>(
          "ScreenContractsTest", 1, 0, "ChartData", "runner supplied");
        qmlRegisterUncreatableType<GameplayContext>(
          "ScreenContractsTest", 1, 0, "GameplayContext", "host supplied");
        qmlRegisterUncreatableType<ResultContext>(
          "ScreenContractsTest", 1, 0, "ResultContext", "host supplied");
        qmlRegisterUncreatableType<ResultPlayer>(
          "ScreenContractsTest", 1, 0, "ResultPlayer", "host supplied");
        qmlRegisterUncreatableType<CourseResultContext>(
          "ScreenContractsTest", 1, 0, "CourseResultContext", "host supplied");
        qmlRegisterUncreatableType<CourseResultPlayer>(
          "ScreenContractsTest", 1, 0, "CourseResultPlayer", "host supplied");
        qmlRegisterSingletonType<ScreenContexts>(
          "ScreenContractsTest",
          1,
          0,
          "ScreenContexts",
          [](QQmlEngine*, QJSEngine*) -> QObject* {
              return new ScreenContexts;
          });
        return true;
    }();
    (void)registration;
}

auto
metadata(QString title, ChartData::Keymode keymode) -> ChartData*
{
    return new ChartData(std::move(title),
                         {},
                         {},
                         {},
                         {},
                         {},
                         {},
                         {},
                         100,
                         100,
                         1,
                         1,
                         false,
                         {},
                         1,
                         0,
                         0,
                         0,
                         0,
                         1000,
                         120,
                         120,
                         120,
                         120,
                         120,
                         0,
                         0,
                         0,
                         {},
                         0,
                         {},
                         {},
                         keymode,
                         {},
                         {},
                         1);
}

struct ContextFixture
{
    QTemporaryDir directory;
    std::filesystem::path songPath =
      support::qStringToPath(directory.filePath("songs.sqlite"));
    db::SqliteCppDb songs{ songPath };
    resource_managers::SongAssetStore assets;
    QNetworkAccessManager network;
    std::unique_ptr<resource_managers::Profile> first;
    std::unique_ptr<resource_managers::Profile> second;
    std::vector<std::unique_ptr<QPromise<BmsGameReferee>>> referees;
    std::vector<
      std::unique_ptr<QPromise<std::unique_ptr<qml_components::BgaContainer>>>>
      bgas;
    ScreenContexts contexts;
    arena::test::FakeArenaTransport transport;
    arena::test::FakeArenaIdentityProvider identity;
    arena::test::FakeArenaScheduler scheduler;
    arena::ArenaSession arenaSession{ &transport,
                                      &identity,
                                      &scheduler,
                                      QUrl("wss://arena.example.test"),
                                      "test" };

    ContextFixture()
    {
        resource_managers::defineDb(songs);
        first = profile("first.sqlite");
        second = profile("second.sqlite");
    }
    ~ContextFixture()
    {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    auto profile(const QString& name)
      -> std::unique_ptr<resource_managers::Profile>
    {
        return std::make_unique<resource_managers::Profile>(
          songPath,
          support::qStringToPath(directory.filePath(name)),
          QMap<QString, qml_components::ThemeFamily>{},
          QList<QString>{},
          &network,
          &assets);
    }
    auto player(ChartData::Keymode keys, resource_managers::Profile* profile)
      -> Player*
    {
        auto promise = std::make_unique<QPromise<BmsGameReferee>>();
        promise->start();
        auto future = promise->future();
        referees.push_back(std::move(promise));
        auto* score =
          new BmsLiveScore(1,
                           0,
                           0,
                           0,
                           0,
                           1,
                           2,
                           {},
                           {},
                           resource_managers::NoteOrderAlgorithm::Normal,
                           resource_managers::NoteOrderAlgorithm::Normal,
                           resource_managers::DpOptions::Off,
                           {},
                           0,
                           1000,
                           {},
                           {},
                           keys,
                           0);
        return new Player(new BmsNotes,
                          score,
                          new GameplayState({}, new BarLinesState({})),
                          profile,
                          future,
                          std::chrono::seconds(1),
                          120);
    }
    auto chart(ChartData::Keymode keys, bool battle, QString title = "Chart")
      -> std::unique_ptr<ChartRunner>
    {
        auto promise = std::make_unique<
          QPromise<std::unique_ptr<qml_components::BgaContainer>>>();
        promise->start();
        auto future = promise->future();
        bgas.push_back(std::move(promise));
        return std::make_unique<ChartRunner>(metadata(std::move(title), keys),
                                             future,
                                             keys,
                                             player(keys, first.get()),
                                             battle ? player(keys, second.get())
                                                    : nullptr);
    }
    auto course(ChartData::Keymode keys, bool battle)
      -> std::unique_ptr<CourseRunner>
    {
        resource_managers::Course definition{};
        definition.name = "Two stages";
        definition.md5s = { "first", "second" };
        return std::make_unique<CourseRunner>(
          new CoursePlayer("p1"),
          battle ? new CoursePlayer("p2") : nullptr,
          definition,
          QList<ChartData*>{ metadata("First", keys),
                             metadata("Second", keys) },
          [this, keys, battle] { return chart(keys, battle); });
    }
    auto score(Player* player) -> std::unique_ptr<BmsScore>
    {
        auto* live = player->getScore();
        return std::make_unique<BmsScore>(
          live->getResult(), live->getReplayData(), live->getGaugeHistory());
    }
};
}

TEST_CASE("Gameplay contexts cover single charts and courses for every "
          "supported layout",
          "[ScreenContexts]")
{
    ensureContextApplication();
    ContextFixture fixture;
    for (const auto keys : { ChartData::Keymode::K5,
                             ChartData::Keymode::K7,
                             ChartData::Keymode::K10,
                             ChartData::Keymode::K14 }) {
        for (const bool battle : { false, true }) {
            if (battle && (keys == ChartData::Keymode::K10 ||
                           keys == ChartData::Keymode::K14))
                continue;
            auto single = fixture.chart(keys, battle);
            auto course = fixture.course(keys, battle);
            for (auto* runner : { static_cast<QObject*>(single.get()),
                                  static_cast<QObject*>(course.get()) }) {
                auto* context = fixture.contexts.createGameplay(runner);
                REQUIRE(context);
                CHECK(context->players().size() == (battle ? 2 : 1));
                CHECK(context->players()[0]->getProfile() ==
                      fixture.first.get());
                CHECK(context->keymode() == keys);
                CHECK(context->chartData());
                CHECK(context->stageIndex() == 0);
                CHECK(context->stageCount() ==
                      (runner == course.get() ? 2 : 1));
                CHECK(context->isCourse() == (runner == course.get()));
                CHECK(context->course().isValid() == context->isCourse());
                CHECK(context->coursePlayers().size() ==
                      (context->isCourse() ? context->players().size() : 0));
                CHECK(context->metaObject()->indexOfProperty("runner") == -1);
                CHECK(context->runner() == runner);
                CHECK(context->parent() == runner);
            }
        }
    }
}

TEST_CASE("Course contexts publish coherent stages and results retain "
          "completed metadata",
          "[ScreenContexts]")
{
    ensureContextApplication();
    ContextFixture fixture;
    auto runner = fixture.course(ChartData::Keymode::K7, true);
    auto* context = fixture.contexts.createGameplay(runner.get());
    const auto firstPlayer = context->players()[0];
    auto firstScore = fixture.score(firstPlayer);
    auto secondScore = fixture.score(context->players()[1]);
    auto* completedChart = context->chartData();
    int stageChanges = 0;
    QObject::connect(context, &GameplayContext::stageChanged, context, [&] {
        ++stageChanges;
        CHECK(context->stageIndex() == 1);
        CHECK(context->chartData()->getTitle() == "Second");
        CHECK(context->players()[0] == runner->getPlayer1());
        CHECK(context->players()[0] != firstPlayer);
    });
    runner->proceed();
    CHECK(stageChanges == 1);
    // The host opens the result after proceed() has installed the next stage.
    std::unique_ptr<ResultContext> result(fixture.contexts.createResult(
      { firstScore.get(), secondScore.get() },
      { fixture.first.get(), fixture.second.get() },
      completedChart,
      context));
    REQUIRE(result);
    CHECK(result->players()[0]->score() == firstScore.get());
    CHECK(result->players()[1]->profile() == fixture.second.get());
    CHECK(result->chartData()->getTitle() == "First");
    CHECK(result->stageIndex() == 0);
    CHECK(result->stageCount() == 2);
    CHECK(result->course().value<resource_managers::Course>().name ==
          "Two stages");
    runner->proceed();
    CHECK(context->stageIndex() == 1);
    CHECK(context->chartData()->getTitle() == "Second");
    CHECK(stageChanges == 1);
}

TEST_CASE(
  "Play context survives decide handoff and follows its runner lifetime",
  "[ScreenContexts]")
{
    ensureContextApplication();
    ContextFixture fixture;
    auto runner = fixture.chart(ChartData::Keymode::K7, false);
    QPointer<GameplayContext> context =
      fixture.contexts.createGameplay(runner.get(), &fixture.arenaSession);
    QObject decide;
    QObject gameplay;
    int screenChanges = 0;
    QObject::connect(context, &GameplayContext::screenChanged, context, [&] {
        ++screenChanges;
    });
    context->setHostScreen(&decide);
    CHECK(context->hostScreen() == &decide);
    context->setHostScreen(&gameplay);
    CHECK(context->hostScreen() == &gameplay);
    CHECK(screenChanges == 2);
    context->setHostScreen(&gameplay);
    CHECK(screenChanges == 2);
    CHECK(context->isArena());
    auto mapping = context->inputMapping();
    std::swap(mapping[0], mapping[1]);
    context->setInputMapping(mapping);
    CHECK(runner->getInputMapping() == mapping);
    runner.reset();
    CHECK(context.isNull());
}

TEST_CASE(
  "Normal and course result screen contracts reject the other result type",
  "[ScreenContexts]")
{
    ensureContextApplication();
    registerContextTestTypes();
    QQmlEngine engine;
    ResultContext chartResult({}, {}, nullptr, nullptr);
    CourseResultContext courseResult({}, {}, {}, {});
    for (const auto type :
         { QByteArray("ResultContext"), QByteArray("CourseResultContext") }) {
        QQmlComponent component(&engine);
        component.setData("import QtQml\nimport ScreenContractsTest\nQtObject "
                          "{ required property " +
                            type + " result }",
                          QUrl());
        INFO(component.errorString().toStdString());
        REQUIRE(component.isReady());
        auto correct = type == "ResultContext"
                         ? QVariant::fromValue(&chartResult)
                         : QVariant::fromValue(&courseResult);
        auto wrong = type == "ResultContext"
                       ? QVariant::fromValue(&courseResult)
                       : QVariant::fromValue(&chartResult);
        std::unique_ptr<QObject> item(
          component.createWithInitialProperties({ { "result", correct } }));
        REQUIRE(item);
        CHECK(item->property("result").value<QObject*>() ==
              correct.value<QObject*>());
        std::unique_ptr<QObject> rejected(
          component.createWithInitialProperties({ { "result", wrong } }));
        // Qt may still create the root with a null property after rejecting
        // an incompatible QObject. It must never expose the other contract.
        CHECK((!rejected ||
               rejected->property("result").value<QObject*>() == nullptr));
    }
}

TEST_CASE("QML can release a failed result without destroying its play",
          "[ScreenContexts]")
{
    ensureContextApplication();
    registerContextTestTypes();
    ContextFixture fixture;
    auto runner = fixture.chart(ChartData::Keymode::K7, false);
    auto score = fixture.score(runner->getPlayer1());
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import ScreenContractsTest
        QtObject {
            id: screen
            required property QtObject runner
            required property QtObject profile
            required property QtObject score
            property GameplayContext gameplay: ScreenContexts.createGameplay(runner)
            property ResultContext result: ScreenContexts.createResult(
                [score], [profile], gameplay.chartData, gameplay)
            Component.onCompleted: gameplay._screen = screen
            function discardResult() { result.destroy(); }
        }
    )",
                      QUrl());
    INFO(component.errorString().toStdString());
    REQUIRE(component.isReady());
    std::unique_ptr<QObject> screen(component.createWithInitialProperties(
      { { "runner", QVariant::fromValue(static_cast<QObject*>(runner.get())) },
        { "profile",
          QVariant::fromValue(static_cast<QObject*>(fixture.first.get())) },
        { "score",
          QVariant::fromValue(static_cast<QObject*>(score.get())) } }));
    REQUIRE(screen);
    QPointer<GameplayContext> gameplay =
      screen->property("gameplay").value<GameplayContext*>();
    QPointer<ResultContext> result =
      screen->property("result").value<ResultContext*>();
    REQUIRE(gameplay);
    REQUIRE(result);
    CHECK(gameplay->parent() == runner.get());
    CHECK(gameplay->property("_runner").value<QObject*>() == runner.get());
    CHECK(gameplay->hostScreen() == screen.get());
    CHECK(result->gameplay() == gameplay);
    CHECK(QQmlEngine::objectOwnership(result) ==
          QQmlEngine::JavaScriptOwnership);
    REQUIRE(QMetaObject::invokeMethod(screen.get(), "discardResult"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    CHECK(result.isNull());
    CHECK(gameplay);
    screen.reset();
    CHECK(gameplay->hostScreen() == nullptr);
    runner.reset();
    CHECK(gameplay.isNull());
}

TEST_CASE(
  "QML bindings follow course participants without changing completed results",
  "[ScreenContexts]")
{
    ensureContextApplication();
    registerContextTestTypes();
    ContextFixture fixture;
    auto runner = fixture.course(ChartData::Keymode::K7, false);
    auto* gameplay = fixture.contexts.createGameplay(runner.get());
    auto score = fixture.score(runner->getPlayer1());
    std::unique_ptr<ResultContext> result(
      fixture.contexts.createResult({ score.get() },
                                    { fixture.first.get() },
                                    gameplay->chartData(),
                                    gameplay));
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import ScreenContractsTest
        QtObject {
            required property GameplayContext gameplay
            required property ResultContext result
            readonly property string liveTitle: gameplay.chartData.title
            readonly property int stage: gameplay.stageIndex
            readonly property int playerCount: gameplay.players.length
            readonly property int chartCount: gameplay.charts.length
            readonly property QtObject player: gameplay.players[0]
            readonly property QtObject profile: gameplay.players[0].profile
            readonly property string courseName: gameplay.course.name
            readonly property int combo: gameplay.coursePlayers[0].combo
            readonly property string completedTitle: result.chartData.title
            readonly property string arenaRoundId: result.arenaRoundId
            readonly property var scores: result.players.map(player => player.score)
            readonly property QtObject completedScore: scores[0]
            readonly property QtObject completedProfile: result.players[0].profile
        }
    )",
                      QUrl());
    INFO(component.errorString().toStdString());
    REQUIRE(component.isReady());
    std::unique_ptr<QObject> item(component.createWithInitialProperties(
      { { "gameplay", QVariant::fromValue(gameplay) },
        { "result", QVariant::fromValue(result.get()) } }));
    REQUIRE(item);
    CHECK(item->property("liveTitle").toString() == "First");
    CHECK(item->property("stage").toInt() == 0);
    CHECK(item->property("playerCount").toInt() == 1);
    CHECK(item->property("chartCount").toInt() == 2);
    CHECK(item->property("player").value<QObject*>() == runner->getPlayer1());
    CHECK(item->property("profile").value<QObject*>() == fixture.first.get());
    CHECK(item->property("courseName").toString() == "Two stages");
    runner->getCoursePlayer1()->setCombo(7);
    CHECK(item->property("combo").toInt() == 7);
    runner->proceed();
    CHECK(item->property("liveTitle").toString() == "Second");
    CHECK(item->property("stage").toInt() == 1);
    CHECK(item->property("player").value<QObject*>() == runner->getPlayer1());
    CHECK(item->property("completedTitle").toString() == "First");
    CHECK(item->property("arenaRoundId").toString().isEmpty());
    CHECK(item->property("completedScore").value<QObject*>() == score.get());
    CHECK(item->property("completedProfile").value<QObject*>() ==
          fixture.first.get());
}

TEST_CASE("Arena round identity is available in the result context",
          "[ScreenContexts]")
{
    ensureContextApplication();
    ContextFixture fixture;
    auto runner = fixture.chart(ChartData::Keymode::K7, false);
    auto* gameplay =
      fixture.contexts.createGameplay(runner.get(), &fixture.arenaSession);
    auto score = fixture.score(runner->getPlayer1());
    std::unique_ptr<ResultContext> result(
      fixture.contexts.createResult({ score.get() },
                                    { fixture.first.get() },
                                    gameplay->chartData(),
                                    gameplay,
                                    QStringLiteral("arena-round")));
    REQUIRE(result);
    CHECK(result->gameplay()->isArena());
    CHECK(result->property("arenaRoundId").toString() == "arena-round");
}

TEST_CASE("Result construction rejects incomplete participants instead of "
          "exposing mismatched arrays",
          "[ScreenContexts]")
{
    ensureContextApplication();
    ContextFixture fixture;
    auto runner = fixture.chart(ChartData::Keymode::K7, false);
    auto score = fixture.score(runner->getPlayer1());
    auto* context = fixture.contexts.createGameplay(runner.get());
    CHECK_FALSE(fixture.contexts.createResult(
      { score.get() }, {}, context->chartData(), context));
    CHECK_FALSE(fixture.contexts.createResult(
      { nullptr }, { fixture.first.get() }, context->chartData(), context));
    std::unique_ptr<ResultContext> result(
      fixture.contexts.createResult({ score.get() },
                                    { fixture.first.get(), nullptr },
                                    context->chartData(),
                                    context));
    REQUIRE(result);
    CHECK(result->players().size() == 1);
    CHECK(result->stageIndex() == 0);
    CHECK(result->stageCount() == 1);
    CHECK(result->arenaRoundId().isEmpty());
    CHECK_FALSE(result->course().isValid());
}

TEST_CASE("Course summary contexts pair aggregate scores with participants",
          "[ScreenContexts]")
{
    ensureContextApplication();
    ContextFixture fixture;
    for (const bool battle : { false, true }) {
        auto runner = fixture.course(ChartData::Keymode::K7, battle);
        QList<BmsScoreCourse*> scores;
        std::vector<std::unique_ptr<BmsScoreCourse>> ownedScores;
        for (auto* player : { runner->getPlayer1(), runner->getPlayer2() }) {
            if (!player)
                continue;
            QList<BmsScore*> stages{ fixture.score(player).release(),
                                     fixture.score(player).release() };
            auto statistics = std::make_unique<BmsResultCourse>(
              "summary",
              runner->getCourse().getIdentifier(),
              stages,
              "FAILED",
              0,
              runner->getCourse().constraints);
            ownedScores.push_back(
              BmsScoreCourse::fromScores(std::move(statistics), stages));
            scores.append(ownedScores.back().get());
        }
        const QList<resource_managers::Profile*> profiles{
            fixture.first.get(), battle ? fixture.second.get() : nullptr
        };
        std::unique_ptr<CourseResultContext> result(
          fixture.contexts.createCourseResult(
            scores, profiles, runner->getChartDatas(), runner->getCourse()));
        REQUIRE(result);
        CHECK(result->course().name == "Two stages");
        CHECK(result->charts().size() == 2);
        CHECK(result->players().size() == scores.size());
        for (qsizetype i = 0; i < scores.size(); ++i) {
            CHECK(result->players()[i]->profile() == profiles[i]);
            CHECK(result->players()[i]->score() == scores[i]);
            CHECK(result->players()[i]->score()->getScores().size() == 2);
        }
        CHECK_FALSE(fixture.contexts.createCourseResult(
          scores, {}, result->charts(), result->course()));
        CHECK_FALSE(fixture.contexts.createCourseResult(
          scores, profiles, {}, result->course()));
    }
}

TEST_CASE(
  "Course flow presents each typed stage result before the course summary",
  "[ScreenContexts][GameplaySession]")
{
    ensureContextApplication();
    registerContextTestTypes();
    for (const bool battle : { false, true }) {
        ContextFixture fixture;
        auto runner = fixture.course(ChartData::Keymode::K7, battle);
        auto* gameplay = fixture.contexts.createGameplay(runner.get());
        QQmlEngine engine;
        QQmlComponent component(
          &engine,
          QUrl::fromLocalFile(QString::fromUtf8(RHYTHMGAME_SOURCE_DIR) +
                              "/test/qml/CourseFlowHarness.qml"));
        INFO(component.errorString().toStdString());
        REQUIRE(component.isReady());
        std::unique_ptr<QObject> object(component.createWithInitialProperties(
          { { "gameplay", QVariant::fromValue(gameplay) } }));
        INFO(component.errorString().toStdString());
        REQUIRE(object);
        engine.globalObject().setProperty("h", engine.newQObject(object.get()));
        auto run = [&](const QString& script) {
            const auto result = engine.evaluate(script);
            INFO(result.toString().toStdString());
            REQUIRE_FALSE(result.isError());
            for (int i = 0; i < 8; ++i) {
                QCoreApplication::processEvents();
                QCoreApplication::sendPostedEvents(nullptr,
                                                   QEvent::DeferredDelete);
            }
        };
        auto check = [&](const QString& expression) {
            const auto result = engine.evaluate(expression);
            INFO(expression.toStdString());
            INFO(result.toString().toStdString());
            REQUIRE_FALSE(result.isError());
            CHECK(result.toBool());
        };
        runner->getPlayer1()->setStatus(ChartRunner::Ready);
        if (runner->getPlayer2())
            runner->getPlayer2()->setStatus(ChartRunner::Ready);
        run("h.session.complete()");
        check("h.visits.join(',') === 'result 0' && h.failures === 0");
        check("h.results[0].gameplay === h.gameplay && h.gameplay.stageIndex "
              "=== 1");
        check("h.results[0].chartData.title === 'First' && "
              "h.results[0].stageCount === 2");
        run("h.active = true");
        check("h.visits.length === 1 && h.departures === 0");
        runner->getPlayer1()->setStatus(ChartRunner::Ready);
        if (runner->getPlayer2())
            runner->getPlayer2()->setStatus(ChartRunner::Ready);
        run("h.session.complete()");
        check("h.visits.join(',') === 'result 0,result 1' && h.failures === 0");
        check("h.results[0].chartData.title === 'First' && "
              "h.results[1].chartData.title === 'Second'");
        check(
          QString("h.results[1].players.length === %1").arg(battle ? 2 : 1));
        run("h.active = true");
        check("h.visits.join(',') === 'result 0,result 1,courseResult' && "
              "h.failures === 0");
        check("h.courseResult.charts.length === 2 && h.departures === 0");
        check(
          QString("h.courseResult.players.length === %1").arg(battle ? 2 : 1));
        run("h.active = true");
        check("h.departures === 1 && h.visits.length === 3");
    }
}

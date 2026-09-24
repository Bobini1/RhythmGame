#include "RegisterGameplayTestTypes.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QEvent>
#include <QJSEngine>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QUrl>

#include <memory>

Q_IMPORT_QML_PLUGIN(RhythmGameQmlPlugin)

namespace {
void
ensureCoreApplication()
{
    static int argc = 1;
    static char appName[] = "RhythmGame_test";
    static char* argv[] = { appName, nullptr };
    if (!QCoreApplication::instance()) {
        [[maybe_unused]] static auto* app = new QCoreApplication(argc, argv);
    }
    registerGameplayTestTypes();
}

class GameplayHarness
{
    QQmlEngine engine;
    std::unique_ptr<QObject> object;

  public:
    GameplayHarness()
    {
        const auto source =
          QString::fromUtf8(RHYTHMGAME_SOURCE_DIR) +
          QStringLiteral("/test/qml/GameplaySessionHarness.qml");
        QQmlComponent component(&engine, QUrl::fromLocalFile(source));
        INFO(component.errorString().toStdString());
        REQUIRE(component.isReady());
        object.reset(component.create());
        INFO(component.errorString().toStdString());
        REQUIRE(object);
        engine.globalObject().setProperty("h", engine.newQObject(object.get()));
        drain();
    }

    void run(const QString& script)
    {
        const auto result = engine.evaluate(script);
        INFO(result.toString().toStdString());
        REQUIRE_FALSE(result.isError());
        drain();
    }

    void check(const QString& expression)
    {
        const auto result = engine.evaluate(expression);
        INFO(expression.toStdString());
        INFO(result.toString().toStdString());
        REQUIRE_FALSE(result.isError());
        CHECK(result.toBool());
    }

    static void drain()
    {
        // Exercise queued notifications, including those emitted synchronously
        // by proceed() while the result is being prepared.
        for (int i = 0; i < 8; ++i) {
            QCoreApplication::processEvents();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
    }
};
}

TEST_CASE("Gameplay starts only the active ready stage",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.check("h.starts === 0 && h.activations === 0");
    h.run("h.startReady = false; h.active = true;");
    h.check("h.starts === 0 && h.activations === 1");
    h.run("h.startReady = true;");
    h.check("h.starts === 1");
    h.run("h.active = false; h.active = true;");
    h.check("h.starts === 1 && h.activations === 1");
}

TEST_CASE("Arena retains control of gameplay startup", "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.arenaManaged = true; h.active = true;");
    h.check("h.starts === 0 && h.activations === 1");
    h.run("h.runner.status = h.finishedStatus;");
    h.check("h.stageSaves === 1 && h.results === 1");
    h.check("h.arena.submissions === 1 && "
            "h.arena.lastScore === h.lastScores[0] && "
            "h.lastArenaRoundId === 'arena-round'");
}

TEST_CASE("Arena results close without a skin initialization callback",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.arenaManaged = true; h.active = true; h.runner.status = "
          "h.finishedStatus;");
    h.check("h.arena.endings === 0 && h.lastArenaRoundId === 'arena-round'");
    h.run("h.lastResult.destroy();");
    h.check("h.arena.endings === 1 && "
            "h.arena.endedRoundId === 'arena-round' && "
            "h.arena.presentedResult.roundId === ''");
    h.run("h.session.gameplay = null;");
    h.check("h.arena.endings === 1");
}

TEST_CASE("Arena result cleanup keeps the session that accepted the score",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.arenaManaged = true; h.active = true; h.runner.status = "
          "h.finishedStatus;");
    h.run("h.session.arenaSession = null; h.lastResult.destroy();");
    h.check("h.arena.submissions === 1 && h.arena.endings === 1 && "
            "h.arena.endedRoundId === 'arena-round'");
}

TEST_CASE("Arena result creation retries retain the submitted round",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.arenaManaged = true; h.failPresentation = true; "
          "h.active = true; h.runner.status = h.finishedStatus;");
    h.check("h.arena.submissions === 1 && h.arena.endings === 0 && "
            "h.lastArenaRoundId === 'arena-round' && h.failures === 1");
    h.run("h.failPresentation = false; h.session.retryTransition();");
    h.check("h.arena.submissions === 1 && h.stageSaves === 1 && "
            "h.results === 2 && h.lastArenaRoundId === 'arena-round'");
    h.run("h.arena.presentedResult = { roundId: 'next-round' }; "
          "h.lastResult.destroy();");
    h.check("h.arena.endedRoundId === 'arena-round' && "
            "h.arena.presentedResult.roundId === 'next-round'");
}

TEST_CASE("Replacing gameplay clears an Arena result that failed to open",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.arenaManaged = true; h.failPresentation = true; "
          "h.active = true; h.runner.status = h.finishedStatus;");
    h.run("h.session.gameplay = null;");
    h.check("h.arena.endings === 1 && "
            "h.arena.endedRoundId === 'arena-round'");
}

TEST_CASE("Local results never submit a score to Arena",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.active = true; h.runner.status = h.finishedStatus;");
    h.check("h.arena.submissions === 0 && h.lastArenaRoundId === ''");
}

TEST_CASE("Rejected Arena submissions still open ordinary chart results",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.arenaManaged = true; h.arena.acceptResult = false; "
          "h.active = true; h.runner.status = h.finishedStatus;");
    h.check("h.results === 1 && h.arena.submissions === 1 && "
            "h.lastArenaRoundId === ''");
    h.run("h.lastResult.destroy();");
    h.check("h.arena.endings === 0");
}

TEST_CASE("Gameplay completion waits for activity and presentation",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.runner.status = h.finishedStatus; h.finishReady = false;");
    h.check("h.stageSaves === 0 && h.results === 0");
    h.run("h.active = true;");
    h.check("h.finishRequests === 1 && h.stageSaves === 0");
    h.run("h.finishReady = true; h.active = false;");
    h.check("h.stageSaves === 0 && h.departures === 0");
    h.run("h.active = true;");
    h.check("h.stageSaves === 1 && h.results === 1 && !h.active");
    h.run("h.active = true;");
    h.check("h.departures === 1 && h.stageSaves === 1 && h.results === 1");
}

TEST_CASE("Explicit gameplay exit skips the natural finish gate",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.active = true; h.finishReady = false;");
    h.run("h.session.complete(); h.session.complete();");
    h.check("h.stageSaves === 1 && h.results === 1 && h.finishRequests === 0");
}

TEST_CASE("A gameplay finish notification can delay its own transition",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.animateFinish = true; h.active = true;");
    h.run("h.runner.status = h.finishedStatus;");
    h.check("h.delaying && h.finishRequests === 1 && h.results === 0");
    h.run("h.delaying = false;");
    h.check("h.stageSaves === 1 && h.results === 1");
}

TEST_CASE(
  "Course results snapshot the finished stage and resume only on return",
  "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.course = true; h.active = true;");
    h.run("h.runner.status = h.finishedStatus;");
    h.check("h.starts === 1 && h.stageSaves === 1 && h.results === 1");
    h.check("h.lastGameplay === h.gameplay && h.courseResults === 0");
    h.check(
      "h.lastData.md5 === 'first' && h.lastProfiles[0] === 'first player'");
    h.run("h.active = true;");
    h.check("h.starts === 2 && h.activations === 2 && h.courseResults === 0");
    h.run("h.runner.status = h.finishedStatus;");
    h.check(
      "h.stageSaves === 2 && h.results === 2 && h.lastData.md5 === 'second'");
    h.run("h.active = true;");
    h.check(
      "h.courseSaves === 1 && h.courseResults === 1 && h.departures === 0");
    h.run("h.active = true;");
    h.check("h.departures === 1 && h.courseSaves === 1 && h.stageSaves === 2");
}

TEST_CASE("Failed result presentation retains scores for a retry",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.course = true; h.failPresentation = true; h.active = true;");
    h.run("h.runner.status = h.finishedStatus;");
    h.check(
      "h.active && h.failures === 1 && h.stageSaves === 1 && h.starts === 1");
    h.run("h.session.synchronize();");
    h.check("h.results === 1");
    h.run("h.failPresentation = false; h.session.retryTransition();");
    h.check(
      "h.stageSaves === 1 && h.results === 2 && h.lastData.md5 === 'first'");
    h.run("h.active = true;");
    h.run("h.runner.status = h.finishedStatus;");
    h.run("h.failPresentation = true; h.active = true;");
    h.check("h.courseSaves === 1 && h.courseResults === 1 && h.failures === 2");
    h.run("h.failPresentation = false; h.session.retryTransition();");
    h.check("h.courseSaves === 1 && h.courseResults === 2 && !h.active");
}

TEST_CASE("Queued gameplay work cannot navigate after its screen is covered",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.active = true;");
    h.run("h.session.complete(); h.active = false;");
    h.check("h.results === 0 && h.stageSaves === 0 && h.departures === 0");
    h.run("h.session.leave(); h.session.complete();");
    h.check("h.results === 0 && h.departures === 0");
}

TEST_CASE("Changing the gameplay context invalidates queued completion",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.active = true;");
    h.run("h.session.complete(); h.session.gameplay = null;");
    h.check("h.results === 0 && h.stageSaves === 0 && h.departures === 0");
    h.run("h.session.gameplay = h.gameplay;");
    h.check("h.starts === 2 && h.activations === 2 && h.results === 0");
}

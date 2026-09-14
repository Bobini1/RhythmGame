#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QJSEngine>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QUrl>

#include <memory>

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
        for (int i = 0; i < 8; ++i)
            QCoreApplication::processEvents();
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
    h.run("h.runner.status = 2;");
    h.check("h.stageSaves === 1 && h.results === 1");
}

TEST_CASE("Gameplay completion waits for activity and presentation",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.runner.status = 2; h.finishReady = false;");
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
    h.run("h.runner.status = 2;");
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
    h.run("h.runner.status = 2;");
    h.check("h.starts === 1 && h.stageSaves === 1 && h.results === 1");
    h.check(
      "h.lastData.md5 === 'first' && h.lastProfiles[0] === 'first player'");
    h.run("h.active = true;");
    h.check("h.starts === 2 && h.activations === 2 && h.courseResults === 0");
    h.run("h.runner.status = 2;");
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
    h.run("h.runner.status = 2;");
    h.check(
      "h.active && h.failures === 1 && h.stageSaves === 1 && h.starts === 1");
    h.run("h.session.synchronize();");
    h.check("h.results === 1");
    h.run("h.failPresentation = false; h.session.retryTransition();");
    h.check(
      "h.stageSaves === 1 && h.results === 2 && h.lastData.md5 === 'first'");
    h.run("h.active = true;");
    h.run("h.runner.status = 2;");
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

TEST_CASE("Changing the runner invalidates queued gameplay completion",
          "[qml][GameplaySession]")
{
    ensureCoreApplication();
    GameplayHarness h;
    h.run("h.active = true;");
    h.run("h.session.complete(); h.session.chart = null;");
    h.check("h.results === 0 && h.stageSaves === 0 && h.departures === 0");
    h.run("h.session.chart = h.runner;");
    h.check("h.starts === 2 && h.activations === 2 && h.results === 0");
}

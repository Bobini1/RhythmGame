#include "Rg.h"
#include "ScreenContexts.h"
#include "qml_components/InputAttached.h"
#include "resource_managers/ChartFactory.h"
#include "resource_managers/GaugeFactory.h"
#include "resource_managers/DefineDb.h"
#include "gameplay_logic/rules/Lr2TimingWindows.h"
#include "sounds/AudioPlayer.h"
#include "support/QStringToPath.h"
#include "../arena/FakeArenaIdentityProvider.h"
#include "../arena/FakeArenaScheduler.h"
#include "../arena/FakeArenaTransport.h"

#include <catch2/catch_session.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <QElapsedTimer>
#include <QDir>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <string_view>

Q_IMPORT_QML_PLUGIN(RhythmGameQmlPlugin)

int
main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "minimal:enable_fonts");
    qputenv("RHYTHMGAME_AUDIO_BACKEND", "Null");
    QGuiApplication application(argc, argv);
    return Catch::Session().run(argc, argv);
}

namespace {
void
checkGameplayFlow(const char* lesson,
                  bool arenaManaged,
                  const char* playMode = "solo")
{
    CAPTURE(lesson, arenaManaged, playMode);
    const auto doublePlay = std::string_view(playMode) == "double";
    const auto battle = std::string_view(playMode) == "battle";
    QTemporaryDir directory;
    const auto root = support::qStringToPath(directory.path());
    db::SqliteCppDb database(root / "songs.sqlite");
    resource_managers::defineDb(database);
    input::InputTranslator input(&database);
    QNetworkAccessManager network;
    sounds::AudioEngine audio;
    resource_managers::SongAssetStore assets;
    qml_components::ProfileList profiles(root / "songs.sqlite",
                                         &database,
                                         {},
                                         root / "profiles",
                                         {},
                                         &network,
                                         &assets);
    auto* profile = profiles.getMainProfile();
    REQUIRE(profile);
    arena::test::FakeArenaTransport transport;
    arena::test::FakeArenaIdentityProvider identity;
    arena::test::FakeArenaScheduler scheduler;
    arena::ArenaSession arena(&transport,
                              &identity,
                              &scheduler,
                              QUrl("wss://arena.example.test"),
                              "test");
    resource_managers::ChartFactory factory(&audio, &input, &assets);
    auto components = resource_managers::ChartDataFactory{}.loadChartData(
      doublePlay ? "#TITLE Startup\n#BPM 120\n#01011:01\n#01021:01\n"
                 : "#TITLE Startup\n#BPM 120\n#01011:01\n",
      root / "chart.bms",
      [](auto) { return 1; });
    resource_managers::ChartFactory::PlayerSpecificData player{
        .profile = profile,
        .gauges = *lesson
          ? resource_managers::GaugeFactory{}.getStandardGauges(profile, 200, 1)
          : QList<gameplay_logic::rules::BmsGauge*>{},
        .hitRules =
          gameplay_logic::rules::HitRules{
            gameplay_logic::rules::lr2_timing_windows::judgeNormal(),
            [](std::chrono::nanoseconds, gameplay_logic::Judgement) {
                return 2.0;
            } },
        .replayedScore = nullptr,
        .noteOrderAlgorithm = resource_managers::NoteOrderAlgorithm::Normal,
        .noteOrderAlgorithmP2 = resource_managers::NoteOrderAlgorithm::Normal,
        .randomSeed = 0
    };
    std::optional<resource_managers::ChartFactory::PlayerSpecificData> player2;
    if (battle) {
        REQUIRE(profiles.createProfile());
        player2 = player;
        player2->profile = profiles.at(1);
        player2->gauges = resource_managers::GaugeFactory{}.getStandardGauges(
          player2->profile, 200, 1);
    }
    auto runner =
      factory.createChart(std::move(components), player, player2, 2.0);
    auto* chartRunner = runner.get();
    if (arenaManaged) {
        chartRunner->holdStart();
    }
    rhythm_game_qml::ScreenContexts contexts;
    std::unique_ptr<rhythm_game_qml::GameplayContext> gameplay(
      contexts.createGameplay(arenaManaged ? runner.get() : runner.release(),
                              arenaManaged ? &arena : nullptr));
    REQUIRE(gameplay);
    REQUIRE(gameplay->status() == gameplay_logic::ChartRunner::Loading);
    Rg rg{ nullptr, &input,  nullptr, nullptr,   nullptr, nullptr,
           nullptr, nullptr, nullptr, &profiles, &arena,  nullptr,
           nullptr, nullptr, &audio,  nullptr,   &assets };
    auto* previousRg = Rg::instance;
    auto* previousInput = qml_components::InputAttached::inputSignalProvider;
    auto* previousAudio = sounds::AudioPlayer::engine;
    auto* previousAssets = sounds::AudioPlayer::assetStore;
    Rg::instance = &rg;
    qml_components::InputAttached::inputSignalProvider = &input;
    sounds::AudioPlayer::engine = &audio;
    sounds::AudioPlayer::assetStore = &assets;
    const auto restore = qScopeGuard([&] {
        Rg::instance = previousRg;
        qml_components::InputAttached::inputSignalProvider = previousInput;
        sounds::AudioPlayer::engine = previousAudio;
        sounds::AudioPlayer::assetStore = previousAssets;
    });
    QQmlEngine engine;
    QStringList warnings;
    QObject::connect(
      &engine, &QQmlEngine::warnings, [&](const QList<QQmlError>& errors) {
          for (const auto& error : errors)
              warnings.append(error.toString());
      });
    if (!*lesson) {
        // Exercise the real profile, chart metadata and native filtered models.
        auto speedChart = resource_managers::ChartDataFactory{}.loadChartData(
          "#TITLE Speed\n#BPM 180\n#BPM01 60\n#BPM02 300\n"
          "#00108:01\n#00208:02\n#01011:01\n",
          root / "speed.bms", [](auto) { return 1; });
        auto* chart = speedChart.chartData.get();
        auto* currentPlayer = gameplay->players().front();
        auto* column = currentPlayer->getState()->getColumnFilters().front();
        auto* bars = currentPlayer->getState()->getBarLineFilter();
        auto* vars = profile->getVars()->getGeneralVars();
        vars->setNoteScreenTimeMillis(1000);
        QQmlComponent viewportComponent(&engine);
        viewportComponent.loadFromModule("RhythmGameQml", "StandardGameplayViewport");
        INFO(viewportComponent.errorString().toStdString());
        REQUIRE(viewportComponent.isReady());
        std::unique_ptr<QObject> viewport(viewportComponent.createWithInitialProperties(
          { { "player", QVariant::fromValue(currentPlayer) },
            { "chartData", QVariant::fromValue(chart) },
            { "noteHeight", 600 } }));
        REQUIRE(viewport);
        using resource_managers::HiSpeedFix;
        for (const auto& [mode, bpm] : {
               std::pair{ HiSpeedFix::Off, 120.0 },
               std::pair{ HiSpeedFix::Main, chart->getMainBpm() },
               std::pair{ HiSpeedFix::Start, chart->getInitialBpm() },
               std::pair{ HiSpeedFix::Min, chart->getMinBpm() },
               std::pair{ HiSpeedFix::Max, chart->getMaxBpm() },
               std::pair{ HiSpeedFix::Avg, chart->getAvgBpm() } }) {
            CAPTURE(mode, bpm);
            vars->setHiSpeedFix(mode);
            CHECK(viewport->property("pixelsPerBeat").toDouble()
                  == Catch::Approx(36000 / bpm));
            CHECK(column->getTopPosition() - column->getBottomPosition()
                  == Catch::Approx(bpm / 60));
            CHECK(bars->getTopPosition() - bars->getBottomPosition()
                  == Catch::Approx(bpm / 60));
        }
        vars->setHiSpeedFix(HiSpeedFix::Off);
        vars->setNoteScreenTimeMillis(500);
        CHECK(viewport->property("pixelsPerBeat").toDouble() == 600);
        CHECK(column->getTopPosition() - column->getBottomPosition() == 1);
        viewport->setProperty("noteHeight", 300);
        CHECK(viewport->property("pixelsPerBeat").toDouble() == 300);
        CHECK(column->getTopPosition() - column->getBottomPosition() == 1);
        viewport->setProperty("visibleHeight", 150);
        viewport->setProperty("barLineHeight", 75);
        CHECK(viewport->property("pixelsPerBeat").toDouble() == 150);
        CHECK(column->getTopPosition() - column->getBottomPosition() == 2);
        CHECK(bars->getTopPosition() - bars->getBottomPosition() == 0.5);
        currentPlayer->getState()->setPosition(20);
        CHECK(column->getBottomPosition() == 20);
        CHECK(column->getTopPosition() == 22);
        currentPlayer->getState()->setPosition(0);
        viewport->setProperty("visibleHeight", 0);
        CHECK(viewport->property("pixelsPerBeat").toDouble() == 0);
        CHECK(column->getTopPosition() == column->getBottomPosition());
        viewport->setProperty("visibleHeight", 150);

        // A new course stage can have the same BPM and still needs its models set.
        auto nextStage = factory.createChart(std::move(speedChart), player, std::nullopt, 2.0);
        auto* nextPlayer = nextStage->getPlayer1();
        viewport->setProperty("player", QVariant::fromValue(nextPlayer));
        auto* nextColumn = nextPlayer->getState()->getColumnFilters().front();
        CHECK(nextColumn->getTopPosition() - nextColumn->getBottomPosition() == 2);
        vars->setHiSpeedFix(HiSpeedFix::Max);
        CHECK(viewport->property("pixelsPerBeat").toDouble() == 60);
        viewport->setProperty("chartData", QVariant::fromValue(gameplay->chartData()));
        CHECK(viewport->property("pixelsPerBeat").toDouble() == 150);
        CHECK(nextColumn->getTopPosition() - nextColumn->getBottomPosition() == 2);
        viewport.reset();
        vars->resetHiSpeedFix();
        vars->resetNoteScreenTimeMillis();
    }
    QQmlComponent navigation(&engine);
    navigation.setData(R"(
import QtQuick
QtObject {
    property Item currentScreen: null
    property int returns: 0
    function returnToPreviousScreen() {
        ++returns;
        currentScreen.enabled = false;
        currentScreen = null;
        return true;
    }
})",
                       QUrl());
    std::unique_ptr<QObject> host(navigation.create());
    REQUIRE(host);
    engine.rootContext()->setContextProperty("globalRoot", host.get());
    QQmlComponent component(&engine);
    if (*lesson) {
        component.loadUrl(QUrl::fromLocalFile(
          QString::fromUtf8(RHYTHMGAME_SOURCE_DIR) +
          "/docs/examples/theme-tutorial/" + lesson + "/Gameplay.qml"));
    } else {
        component.setData(R"(
import QtQuick
import RhythmGameQml
Item {
    id: screen
    required property GameplayContext gameplay
    property int activations: 0
    readonly property string sourceFile: QmlUtils.fileName
    Item {
        StandardGameplayFlow {
            id: flow
            gameplay: screen.gameplay
            readySoundSource: ""
            exitFeedbackSource: ""
            startDelayMillis: 0
            retryEnabled: false
            onStageActivated: ++screen.activations
        }
    }
})",
                          QUrl());
    }
    INFO(component.errorString().toStdString());
    REQUIRE(component.isReady());
    std::unique_ptr<QObject> screen(component.createWithInitialProperties(
      { { "gameplay", QVariant::fromValue(gameplay.get()) },
        { "enabled", false } }));
    INFO(component.errorString().toStdString());
    REQUIRE(screen);
    auto* flow = qmlContext(screen.get())->objectForName("flow");
    REQUIRE(flow);
    flow->setProperty("readySoundSource", QUrl());
    flow->setProperty("exitFeedbackSource", QUrl());
    flow->setProperty("startDelayMillis", arenaManaged ? 50 : 0);
    if (arenaManaged) {
        flow->setProperty("startReady", false);
    }
    flow->setProperty("retryEnabled", false);
    QSignalSpy activated(flow, SIGNAL(stageActivated()));
    REQUIRE(activated.isValid());
    auto* screenItem = qobject_cast<QQuickItem*>(screen.get());
    REQUIRE(screenItem);
    QQuickWindow window;
    window.resize(960, 540);
    screenItem->setSize(window.size());
    screenItem->setParentItem(window.contentItem());
    window.show();
    host->setProperty("currentScreen", QVariant::fromValue(screen.get()));
    auto* flowItem = qobject_cast<QQuickItem*>(flow);
    REQUIRE(flowItem);
    REQUIRE(flowItem->parentItem() != screenItem);
    CHECK_FALSE(flowItem->isEnabled());
    QElapsedTimer timeout;
    timeout.start();
    while (gameplay->status() == gameplay_logic::ChartRunner::Loading &&
           timeout.elapsed() < 5000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    REQUIRE(gameplay->status() == gameplay_logic::ChartRunner::Ready);
    QTest::qWait(100);
    CHECK(gameplay->status() == gameplay_logic::ChartRunner::Ready);
    CHECK_FALSE(chartRunner->startPending());
    CHECK(activated.count() == 0);
    QVariant exited;
    REQUIRE(
      QMetaObject::invokeMethod(flow, "exit", Q_RETURN_ARG(QVariant, exited)));
    CHECK_FALSE(exited.toBool());
    CHECK(host->property("returns").toInt() == 0);
    screenItem->setEnabled(true);
    CHECK(flowItem->isEnabled());
    timeout.restart();
    if (arenaManaged) {
        QTest::qWait(100);
        CHECK_FALSE(chartRunner->startPending());
        QSignalSpy pending(chartRunner,
                           &gameplay_logic::ChartRunner::startPendingChanged);
        flow->setProperty("startReady", true);
        CHECK_FALSE(chartRunner->startPending());
        QTest::qWait(10);
        flowItem->parentItem()->setEnabled(false);
        CHECK_FALSE(flowItem->isEnabled());
        QTest::qWait(100);
        CHECK_FALSE(chartRunner->startPending());
        CHECK(pending.count() == 0);
        flowItem->parentItem()->setEnabled(true);
        timeout.restart();
        while (!chartRunner->startPending() && timeout.elapsed() < 5000) {
            QCoreApplication::processEvents();
            QThread::msleep(1);
        }
        REQUIRE(chartRunner->startPending());
        CHECK(gameplay->status() == gameplay_logic::ChartRunner::Ready);
        CHECK(pending.count() == 1);
        CHECK(activated.count() == 1);
        screenItem->setEnabled(false);
        screenItem->setEnabled(true);
        QTest::qWait(100);
        CHECK(pending.count() == 1);
        CHECK(activated.count() == 1);
        chartRunner->releaseStart();
        CHECK(gameplay->status() == gameplay_logic::ChartRunner::Running);
        CHECK_FALSE(chartRunner->startPending());
        INFO(warnings.join('\n').toStdString());
        CHECK(warnings.isEmpty());
        return;
    }
    while (gameplay->status() != gameplay_logic::ChartRunner::Running &&
           timeout.elapsed() < 5000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    CHECK(gameplay->status() == gameplay_logic::ChartRunner::Running);
    CHECK(activated.count() == 1);
    // The complete flow supplies speed controls without theme-specific wiring.
    {
        using input::BmsKey;
        auto* vars = profile->getVars()->getGeneralVars();
        input.buttonPressed(BmsKey::Start1, 1000);
        input.buttonPressed(BmsKey::Col1sUp, 1001);
        input.col1sUpTicked(1, input::InputTranslator::ButtonTick);
        CHECK(vars->getNoteScreenTimeMillis() == Catch::Approx(999));
        input.buttonReleased(BmsKey::Col1sUp, 1002);
        input.buttonReleased(BmsKey::Start1, 1003);
        flow->setProperty("speedControlsEnabled", false);
        input.buttonPressed(BmsKey::Start1, 2000);
        input.buttonPressed(BmsKey::Col1sUp, 2001);
        input.col1sUpTicked(1, input::InputTranslator::ButtonTick);
        CHECK(vars->getNoteScreenTimeMillis() == Catch::Approx(999));
        input.buttonReleased(BmsKey::Col1sUp, 2002);
        input.buttonReleased(BmsKey::Start1, 2003);
        flow->setProperty("speedControlsEnabled", true);
        vars->resetNoteScreenTimeMillis();
    }
    if (*lesson) {
        auto* vars = profile->getVars()->getGeneralVars();
        auto* overlay =
          screen->findChild<QQuickItem*>("arenaGameplayPlacementFrame");
        REQUIRE(overlay);
        for (bool arenaPanel : { false, true }) {
            overlay->setProperty("active", arenaPanel);
            for (bool covers : { false, true }) {
                vars->setBgaOn(covers);
                vars->setLiftOn(covers);
                vars->setLiftRatio(0.2);
                vars->setLaneCoverOn(covers);
                vars->setLaneCoverRatio(0.25);
                vars->setHiddenOn(covers);
                vars->setHiddenRatio(0.3);
                for (const auto size : { QSize(960, 540),
                                         QSize(640, 480),
                                         QSize(480, 640),
                                         QSize(360, 640) }) {
                    window.resize(size);
                    screenItem->setSize(size);
                    QTest::qWait(50);
                    int lanes = 0;
                    QList<QRectF> laneRects;
                    auto items = screenItem->childItems();
                    for (qsizetype i = 0; i < items.size(); ++i) {
                        auto* item = items[i];
                        items.append(item->childItems());
                        if (!item->property("columnState").isValid())
                            continue;
                        ++lanes;
                        REQUIRE(item->height() > 0);
                        REQUIRE(item->width() > 0);
                        const auto laneRect = item->mapRectToItem(
                          screenItem,
                          QRectF(0, 0, item->width(), item->height()));
                        CHECK(laneRect.right() <= screenItem->width() / 2);
                        for (const auto& previous : laneRects)
                            CHECK_FALSE(previous.intersects(laneRect));
                        laneRects.append(laneRect);
                        if (arenaPanel) {
                            REQUIRE(overlay->isVisible());
                            const auto panelRect = overlay->mapRectToItem(
                              screenItem,
                              QRectF(
                                0, 0, overlay->width(), overlay->height()));
                            CHECK_FALSE(laneRect.intersects(panelRect));
                        }
                        auto* currentPlayer =
                          item->property("player")
                            .value<gameplay_logic::Player*>();
                        REQUIRE(currentPlayer);
                        const auto covered =
                          covers && currentPlayer->getProfile() == profile;
                        const auto travel =
                          currentPlayer->positionAt(std::chrono::seconds(1)) -
                          currentPlayer->positionAt(std::chrono::seconds(0));
                        CHECK(
                          travel * item->property("pixelsPerBeat").toDouble() ==
                          Catch::Approx(item->height() * (covered ? 0.75 : 1)));
                    }
                    qsizetype expectedLanes = 0;
                    for (const auto* participant : gameplay->players())
                        expectedLanes += participant->getColumns().size();
                    CHECK(lanes == expectedLanes);
                    const auto screenshots =
                      qEnvironmentVariable("RHYTHMGAME_TEST_SCREENSHOTS");
                    if (covers && !screenshots.isEmpty()) {
                        QDir().mkpath(screenshots);
                        const auto capture = screenItem->grabToImage();
                        REQUIRE(capture);
                        QSignalSpy captured(capture.get(),
                                            &QQuickItemGrabResult::ready);
                        REQUIRE(captured.wait(1000));
                        CHECK(capture->saveToFile(
                          screenshots + '/' + lesson + '-' +
                          QString::number(size.width()) + 'x' +
                          QString::number(size.height()) +
                          (std::string_view(playMode) == "solo"
                             ? QString()
                             : '-' + QString::fromUtf8(playMode)) +
                          (arenaPanel ? "-arena.png" : ".png")));
                    }
                }
            }
        }
        overlay->setProperty("active", false);
        vars->setBgaOn(true);
        vars->setLiftOn(false);
        vars->setLaneCoverOn(false);
        vars->setHiddenOn(false);
        QTest::qWait(20);
        vars->setNoteScreenTimeMillis(750);
        auto* column =
          gameplay->players().front()->getState()->getColumnFilters().front();
        CHECK(column->getTopPosition() - column->getBottomPosition() ==
              Catch::Approx(1.5));
        vars->resetNoteScreenTimeMillis();
        QQuickItem* exitButton = nullptr;
        for (auto* item : screen->findChildren<QQuickItem*>()) {
            if (item->property("text").toString() == "Exit") {
                exitButton = item;
                break;
            }
        }
        REQUIRE(exitButton);
        screenItem->setEnabled(false);
        REQUIRE(QMetaObject::invokeMethod(exitButton, "clicked"));
        QTest::keyClick(&window, Qt::Key_Escape);
        CHECK(host->property("returns").toInt() == 0);
        screenItem->setEnabled(true);
        QCoreApplication::processEvents();
        QTest::mouseClick(&window,
                          Qt::LeftButton,
                          Qt::NoModifier,
                          exitButton
                            ->mapToScene(QPointF(exitButton->width() / 2,
                                                 exitButton->height() / 2))
                            .toPoint());
        CHECK(host->property("returns").toInt() == 1);
        CHECK_FALSE(flowItem->isEnabled());
        REQUIRE(QMetaObject::invokeMethod(exitButton, "clicked"));
        CHECK(host->property("returns").toInt() == 1);
        host->setProperty("currentScreen", QVariant::fromValue(screen.get()));
        screenItem->setEnabled(true);
        QCoreApplication::processEvents();
        QTest::keyClick(&window, Qt::Key_Escape);
        CHECK(host->property("returns").toInt() == 2);
    }
    auto* currentPlayer = gameplay->players().front();
    currentPlayer->getState()->setVisiblePositionSpans(64, 64);
    auto* column = currentPlayer->getState()->getColumnFilters().front();
    REQUIRE(column->rowCount({}) > 0);
    for (const auto* laneLesson : { "07-gameplay", "10-complete" }) {
        const auto path = QString::fromUtf8(RHYTHMGAME_SOURCE_DIR) +
                          "/docs/examples/theme-tutorial/" + laneLesson +
                          "/NoteLane.qml";
        QQmlComponent laneComponent(&engine, QUrl::fromLocalFile(path));
        INFO(laneComponent.errorString().toStdString());
        REQUIRE(laneComponent.isReady());
        std::unique_ptr<QObject> lane(laneComponent.createWithInitialProperties(
          { { "player", QVariant::fromValue(currentPlayer) },
            { "columnState", QVariant::fromValue(column) },
            { "pixelsPerBeat", 8 },
            { "scratch", false },
            { "width", 64 },
            { "height", 512 } }));
        REQUIRE(lane);
    }
    INFO(warnings.join('\n').toStdString());
    CHECK(warnings.isEmpty());
}
} // namespace

TEST_CASE("StandardGameplayFlow inherits screen activation through containers",
          "[qml][StandardGameplayFlow]")
{
    checkGameplayFlow(GENERATE("", "07-gameplay", "10-complete"), false);
}

TEST_CASE("tutorial gameplay supports double play and battle",
          "[qml][StandardGameplayFlow]")
{
    checkGameplayFlow(GENERATE("07-gameplay", "10-complete"),
                      false,
                      GENERATE("double", "battle"));
}

TEST_CASE("Arena gameplay waits for presentation before requesting its start",
          "[qml][StandardGameplayFlow][ArenaGameplayStartup]")
{
    checkGameplayFlow("", true);
}

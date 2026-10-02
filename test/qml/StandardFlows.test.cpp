#include "Rg.h"
#include "ScreenContexts.h"
#include "qml_components/InputAttached.h"
#include "resource_managers/ChartFactory.h"
#include "resource_managers/DefineDb.h"
#include "gameplay_logic/rules/Lr2TimingWindows.h"
#include "support/QStringToPath.h"

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QThread>

Q_IMPORT_QML_PLUGIN(RhythmGameQmlPlugin)

int
main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "minimal:enable_fonts");
    qputenv("RHYTHMGAME_AUDIO_BACKEND", "Null");
    QGuiApplication application(argc, argv);
    return Catch::Session().run(argc, argv);
}

TEST_CASE("StandardGameplayFlow starts when an active chart finishes loading",
          "[qml][StandardGameplayFlow]")
{
    QTemporaryDir directory;
    const auto root = support::qStringToPath(directory.path());
    db::SqliteCppDb database(root / "songs.sqlite");
    resource_managers::defineDb(database);
    input::InputTranslator input(&database);
    QNetworkAccessManager network;
    sounds::AudioEngine audio;
    resource_managers::SongAssetStore assets;
    resource_managers::Profile profile(root / "songs.sqlite",
                                       root / "profile.sqlite",
                                       {},
                                       {},
                                       &network,
                                       &assets);
    resource_managers::ChartFactory factory(&audio, &input, &assets);
    auto components = resource_managers::ChartDataFactory{}.loadChartData(
      "#TITLE Startup\n#BPM 120\n#01011:01\n", root / "chart.bms", [](auto) {
          return 1;
      });
    resource_managers::ChartFactory::PlayerSpecificData player{
        .profile = &profile,
        .gauges = {},
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
    auto runner = factory.createChart(
      std::move(components), std::move(player), std::nullopt, 2.0);
    rhythm_game_qml::ScreenContexts contexts;
    std::unique_ptr<rhythm_game_qml::GameplayContext> gameplay(
      contexts.createGameplay(runner.release()));
    REQUIRE(gameplay);
    REQUIRE(gameplay->status() == gameplay_logic::ChartRunner::Loading);
    Rg rg{ nullptr, &input,  nullptr, nullptr, nullptr, nullptr,
           nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
           nullptr, nullptr, &audio,  nullptr, &assets };
    auto* previousRg = Rg::instance;
    auto* previousInput = qml_components::InputAttached::inputSignalProvider;
    Rg::instance = &rg;
    qml_components::InputAttached::inputSignalProvider = &input;
    const auto restore = qScopeGuard([&] {
        Rg::instance = previousRg;
        qml_components::InputAttached::inputSignalProvider = previousInput;
    });
    QQmlEngine engine;
    QStringList warnings;
    QObject::connect(
      &engine, &QQmlEngine::warnings, [&](const QList<QQmlError>& errors) {
          for (const auto& error : errors)
              warnings.append(error.toString());
      });
    QQmlComponent navigation(&engine);
    navigation.setData(
      "import QtQml; QtObject { property QtObject currentScreen: null }",
      QUrl());
    std::unique_ptr<QObject> host(navigation.create());
    REQUIRE(host);
    engine.rootContext()->setContextProperty("globalRoot", host.get());
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import RhythmGameQml
Item {
    id: screen
    required property GameplayContext gameplay
    property int activations: 0
    readonly property string sourceFile: QmlUtils.fileName
    StandardGameplayFlow {
        gameplay: screen.gameplay
        readySoundSource: ""
        exitFeedbackSource: ""
        startDelayMillis: 0
        retryEnabled: false
        onStageActivated: ++screen.activations
    }
})",
                      QUrl());
    INFO(component.errorString().toStdString());
    REQUIRE(component.isReady());
    std::unique_ptr<QObject> screen(component.createWithInitialProperties(
      { { "gameplay", QVariant::fromValue(gameplay.get()) } }));
    INFO(component.errorString().toStdString());
    REQUIRE(screen);
    host->setProperty("currentScreen", QVariant::fromValue(screen.get()));
    REQUIRE(gameplay->status() == gameplay_logic::ChartRunner::Loading);
    QElapsedTimer timeout;
    timeout.start();
    while (gameplay->status() != gameplay_logic::ChartRunner::Running &&
           timeout.elapsed() < 5000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    CHECK(gameplay->status() == gameplay_logic::ChartRunner::Running);
    CHECK(screen->property("activations").toInt() == 1);
    auto* currentPlayer = gameplay->players().front();
    currentPlayer->getState()->setVisiblePositionSpans(64, 64);
    auto* column = currentPlayer->getState()->getColumnFilters().front();
    REQUIRE(column->rowCount({}) > 0);
    for (const auto* lesson : { "07-gameplay", "10-complete" }) {
        const auto path = QString::fromUtf8(RHYTHMGAME_SOURCE_DIR) +
                          "/docs/examples/theme-tutorial/" + lesson +
                          "/NoteLane.qml";
        QQmlComponent laneComponent(&engine, QUrl::fromLocalFile(path));
        INFO(laneComponent.errorString().toStdString());
        REQUIRE(laneComponent.isReady());
        std::unique_ptr<QObject> lane(laneComponent.createWithInitialProperties(
          { { "player", QVariant::fromValue(currentPlayer) },
            { "columnState", QVariant::fromValue(column) },
            { "visibleBeats", 64 },
            { "scratch", false },
            { "width", 64 },
            { "height", 512 } }));
        REQUIRE(lane);
    }
    INFO(warnings.join('\n').toStdString());
    CHECK(warnings.isEmpty());
}

#include "Rg.h"
#include "ScreenContexts.h"
#include "qml_components/Bga.h"
#include "qml_components/InputAttached.h"
#include "resource_managers/ChartFactory.h"
#include "resource_managers/DefineDb.h"
#include "resource_managers/GaugeFactory.h"
#include "gameplay_logic/rules/Lr2TimingWindows.h"
#include "support/QStringToPath.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <QElapsedTimer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>

TEST_CASE("Gameplay controls and BGA follow players and replacement stages",
          "[qml][StandardGameplayFlow][GameplayPresentation]")
{
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
    REQUIRE(profiles.createProfile());
    auto* first = profiles.getMainProfile();
    auto* second = profiles.at(1);
    REQUIRE(first != second);
    Rg rg{ nullptr, &input,  nullptr, nullptr,   nullptr, nullptr,
           nullptr, nullptr, nullptr, &profiles, nullptr, nullptr,
           nullptr, nullptr, &audio,  nullptr,   &assets };
    auto* previousRg = Rg::instance;
    auto* previousInput = qml_components::InputAttached::inputSignalProvider;
    Rg::instance = &rg;
    qml_components::InputAttached::inputSignalProvider = &input;
    const auto restore = qScopeGuard([&] {
        Rg::instance = previousRg;
        qml_components::InputAttached::inputSignalProvider = previousInput;
    });
    resource_managers::ChartFactory factory(&audio, &input, &assets);
    rhythm_game_qml::ScreenContexts contexts;
    const auto makePlayer = [](resource_managers::Profile* profile,
                               bool course) {
        return resource_managers::ChartFactory::PlayerSpecificData{
            .profile = profile,
            .gauges =
              course
                ? resource_managers::GaugeFactory{}.getCourseGauges(profile, {})
                : resource_managers::GaugeFactory{}.getStandardGauges(
                    profile, 200, 2),
            .hitRules =
              gameplay_logic::rules::HitRules{
                gameplay_logic::rules::lr2_timing_windows::judgeNormal(),
                [](std::chrono::nanoseconds, gameplay_logic::Judgement) {
                    return 2.0;
                } },
            .replayedScore = nullptr,
            .noteOrderAlgorithm = resource_managers::NoteOrderAlgorithm::Normal,
            .noteOrderAlgorithmP2 =
              resource_managers::NoteOrderAlgorithm::Normal,
            .randomSeed = 0
        };
    };
    const auto makeGameplay = [&](bool dp, bool battle, bool course) {
        auto data = resource_managers::ChartDataFactory{}.loadChartData(
          dp ? "#TITLE DP\n#BPM 120\n#01011:01\n#01021:01\n"
             : "#TITLE SP\n#BPM 120\n#01011:01\n#01018:01\n",
          root / "chart.bms",
          [](auto) { return 1; });
        auto runner = factory.createChart(
          std::move(data),
          makePlayer(first, course),
          battle ? std::optional{ makePlayer(second, course) } : std::nullopt,
          2.0);
        return std::unique_ptr<rhythm_game_qml::GameplayContext>(
          contexts.createGameplay(runner.release(), nullptr));
    };
    auto solo = makeGameplay(false, false, false);
    auto dp = makeGameplay(true, false, false);
    auto battle = makeGameplay(false, true, false);
    auto courseStage = makeGameplay(false, false, true);

    QQmlEngine engine;
    QStringList warnings;
    QObject::connect(
      &engine, &QQmlEngine::warnings, [&](const QList<QQmlError>& errors) {
          for (const auto& error : errors)
              warnings.append(error.toString());
      });
    QQmlComponent navigation(&engine);
    navigation.setData(
      "import QtQml\nQtObject { property QtObject currentScreen: null }",
      QUrl());
    std::unique_ptr<QObject> host(navigation.create());
    REQUIRE(host);
    engine.rootContext()->setContextProperty("globalRoot", host.get());
    QQmlComponent component(&engine);
    component.loadFromModule("RhythmGameQml", "StandardGameplayInput");
    INFO(component.errorString().toStdString());
    REQUIRE(component.isReady());
    std::unique_ptr<QObject> controls(component.createWithInitialProperties(
      { { "gameplay", QVariant::fromValue(solo.get()) },
        { "retryEnabled", false } }));
    REQUIRE(controls);
    QQuickWindow window;
    qobject_cast<QQuickItem*>(controls.get())
      ->setParentItem(window.contentItem());
    window.resize(640, 480);
    window.show();
    auto* vars = first->getVars()->getGeneralVars();
    auto* vars2 = second->getVars()->getGeneralVars();
    using input::BmsKey;
    const auto tapStart = [&](int64_t time) {
        input.buttonPressed(BmsKey::Start1, time);
        input.buttonReleased(BmsKey::Start1, time + 1);
    };
    const auto scratchDown = [&](int side) {
        const auto key = side == 0 ? BmsKey::Col1sDown : BmsKey::Col2sDown;
        input.buttonPressed(key, 0);
        if (side == 0)
            input.col1sDownTicked(1, input::InputTranslator::ButtonTick);
        else
            input.col2sDownTicked(1, input::InputTranslator::ButtonTick);
        input.buttonReleased(key, 1);
    };

    vars->setLaneCoverRatio(0.25);
    tapStart(1000);
    tapStart(1400);
    CHECK(vars->getLaneCoverOn());
    CHECK(vars->getNoteScreenTimeMillis() == Catch::Approx(750));
    tapStart(2000);
    tapStart(2400);
    CHECK_FALSE(vars->getLaneCoverOn());
    CHECK(vars->getNoteScreenTimeMillis() == Catch::Approx(1000));

    vars->setLaneCoverOn(true);
    input.buttonPressed(BmsKey::Select1, 3000);
    scratchDown(0);
    CHECK(vars->getLaneCoverRatio() == Catch::Approx(0.2505));
    input.buttonReleased(BmsKey::Select1, 3001);
    vars->setLaneCoverOn(false);
    vars->setLiftOn(true);
    vars->setHiddenOn(true);
    vars->setLiftRatio(0.2);
    vars->setHiddenRatio(0.3);
    input.buttonPressed(BmsKey::Select1, 4000);
    scratchDown(0);
    CHECK(vars->getLiftRatio() == Catch::Approx(0.2005));
    input.buttonPressed(BmsKey::Start1, 4001);
    scratchDown(0);
    CHECK(vars->getLiftRatio() == Catch::Approx(0.2005));
    CHECK(vars->getNoteScreenTimeMillis() == Catch::Approx(1000));
    input.buttonReleased(BmsKey::Start1, 4002);
    scratchDown(0);
    CHECK(vars->getHiddenRatio() == Catch::Approx(0.3005));
    input.buttonReleased(BmsKey::Select1, 4003);

    // A disabled screen ignores input, and a partial double press cannot carry
    // over.
    tapStart(5000);
    controls->setProperty("enabled", false);
    input.buttonPressed(BmsKey::Start1, 5100);
    scratchDown(0);
    CHECK(vars->getNoteScreenTimeMillis() == Catch::Approx(1000));
    input.buttonReleased(BmsKey::Start1, 5101);
    controls->setProperty("enabled", true);
    tapStart(5200);
    CHECK_FALSE(vars->getLaneCoverOn());

    // The second side is absent in solo, shares the profile in DP, and is
    // independent in battle.
    input.buttonPressed(BmsKey::Start2, 6000);
    scratchDown(1);
    input.buttonReleased(BmsKey::Start2, 6001);
    CHECK(vars->getNoteScreenTimeMillis() == Catch::Approx(1000));
    controls->setProperty("gameplay", QVariant::fromValue(dp.get()));
    input.buttonPressed(BmsKey::Start1, 7000);
    scratchDown(1);
    input.buttonReleased(BmsKey::Start1, 7001);
    CHECK(vars->getNoteScreenTimeMillis() == Catch::Approx(1001));
    controls->setProperty("gameplay", QVariant::fromValue(battle.get()));
    input.buttonPressed(BmsKey::Start2, 8000);
    scratchDown(1);
    input.buttonReleased(BmsKey::Start2, 8001);
    CHECK(vars->getNoteScreenTimeMillis() == Catch::Approx(1001));
    CHECK(vars2->getNoteScreenTimeMillis() == Catch::Approx(1001));

    // A new stage starts a new double-press sequence, even with the same
    // profile.
    tapStart(9000);
    controls->setProperty("gameplay", QVariant::fromValue(courseStage.get()));
    tapStart(9100);
    CHECK_FALSE(vars->getLaneCoverOn());
    input.buttonPressed(BmsKey::Start1, 10000);
    scratchDown(0);
    input.buttonReleased(BmsKey::Start1, 10001);
    CHECK(vars->getNoteScreenTimeMillis() == Catch::Approx(1002.001));

    controls->setProperty(
      "retryAction", QVariant::fromValue(engine.evaluate("(function() {})")));
    controls->setProperty("retryEnabled", true);
    controls->setProperty("retryHoldDurationMillis", 1);
    REQUIRE(controls->property("retryAvailable").toBool());
    input.buttonPressed(BmsKey::Start1, 11000);
    input.buttonPressed(BmsKey::Select1, 11001);
    QElapsedTimer retryTimeout;
    retryTimeout.start();
    while (!controls->property("retryChoosing").toBool() &&
           retryTimeout.elapsed() < 1000)
        QTest::qWait(10);
    REQUIRE(controls->property("retryChoosing").toBool());
    scratchDown(0);
    CHECK(vars->getNoteScreenTimeMillis() == Catch::Approx(1002.001));
    input.buttonReleased(BmsKey::Start1, 11002);
    input.buttonReleased(BmsKey::Select1, 11003);
    QCoreApplication::processEvents();
    CHECK_FALSE(controls->property("retryChoosing").toBool());

    // Cancelling retry must not require a held scratch to be pressed again.
    for (const auto scratch : { BmsKey::Col1sUp, BmsKey::Col1sDown }) {
        const auto up = scratch == BmsKey::Col1sUp;
        const auto tick = [&] {
            if (up)
                input.col1sUpTicked(1, input::InputTranslator::ButtonTick);
            else
                input.col1sDownTicked(1, input::InputTranslator::ButtonTick);
        };
        input.buttonPressed(scratch, 12000);
        input.buttonPressed(BmsKey::Start1, 12001);
        input.buttonPressed(BmsKey::Select1, 12002);
        retryTimeout.restart();
        while (!controls->property("retryChoosing").toBool() &&
               retryTimeout.elapsed() < 1000)
            QTest::qWait(10);
        REQUIRE(controls->property("retryChoosing").toBool());
        const auto duration = vars->getNoteScreenTimeMillis();
        tick();
        CHECK(vars->getNoteScreenTimeMillis() == duration);
        input.buttonReleased(BmsKey::Start1, 12003);
        input.buttonReleased(BmsKey::Select1, 12004);
        QCoreApplication::processEvents();
        REQUIRE_FALSE(controls->property("retryChoosing").toBool());
        input.buttonPressed(BmsKey::Start1, 13000);
        tick();
        CHECK(vars->getNoteScreenTimeMillis() ==
              Catch::Approx(duration * (up ? 0.999 : 1.001)));
        input.buttonReleased(BmsKey::Start1, 13001);
        vars->setLaneCoverOn(true);
        vars->setLaneCoverRatio(0.25);
        input.buttonPressed(BmsKey::Select1, 14000);
        tick();
        CHECK(vars->getLaneCoverRatio() == Catch::Approx(up ? 0.2495 : 0.2505));
        input.buttonReleased(BmsKey::Select1, 14001);
        input.buttonReleased(scratch, 14002);
    }

    QQmlComponent bgaComponent(&engine);
    bgaComponent.loadFromModule("RhythmGameQml", "StandardGameplayBga");
    INFO(bgaComponent.errorString().toStdString());
    REQUIRE(bgaComponent.isReady());
    std::unique_ptr<QObject> bga(bgaComponent.createWithInitialProperties(
      { { "gameplay", QVariant::fromValue(solo.get()) },
        { "width", 256 },
        { "height", 256 } }));
    REQUIRE(bga);
    for (auto* stage : { solo.get(), courseStage.get() }) {
        bga->setProperty("gameplay", QVariant::fromValue(stage));
        QElapsedTimer timeout;
        timeout.start();
        while (!stage->bga() && timeout.elapsed() < 5000)
            QTest::qWait(1);
        REQUIRE(stage->bga());
        for (auto* layer : stage->bga()->getLayers())
            CHECK(layer->getVideoSink());
        auto* poor = qmlContext(bga.get())->objectForName("videoPoor");
        REQUIRE(poor);
        CHECK_FALSE(poor->property("visible").toBool());
        stage->players().front()->getScore()->hit(gameplay_logic::HitEvent(
          0,
          0,
          0,
          gameplay_logic::BmsPoints(0, gameplay_logic::Judgement::Poor, 0),
          gameplay_logic::HitEvent::Action::None,
          true));
        CHECK(poor->property("visible").toBool());
        if (stage == courseStage.get()) {
            bga->setProperty("enabled", false);
            CHECK_FALSE(poor->property("visible").toBool());
            bga->setProperty("enabled", true);
        }
    }
    for (auto* layer : solo->bga()->getLayers())
        CHECK_FALSE(layer->getVideoSink());

    for (const auto* stage : { solo.get(), courseStage.get() }) {
        for (const auto* lesson : { "07-gameplay", "10-complete" }) {
            QQmlComponent gaugeComponent(
              &engine,
              QUrl::fromLocalFile(QString::fromUtf8(RHYTHMGAME_SOURCE_DIR) +
                                  "/docs/examples/theme-tutorial/" + lesson +
                                  "/GaugeBar.qml"));
            INFO(gaugeComponent.errorString().toStdString());
            REQUIRE(gaugeComponent.isReady());
            const auto& gauges =
              stage->players().front()->getScore()->getGauges();
            std::unique_ptr<QObject> gauge(
              gaugeComponent.createWithInitialProperties(
                { { "gauges", QVariant::fromValue(gauges) } }));
            REQUIRE(gauge);
            REQUIRE(gauge->property("activeGauge").value<QObject*>());
            for (auto* value : gauges)
                value->addMineHit(std::chrono::nanoseconds(0), -2);
            CHECK(gauge->property("activeGauge").value<QObject*>() ==
                  gauges.back());
        }
    }
    INFO(warnings.join('\n').toStdString());
    CHECK(warnings.isEmpty());
}

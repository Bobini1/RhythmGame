#include "Rg.h"
#include "ScreenContexts.h"
#include "qml_components/InputAttached.h"
#include "qml_components/Lr2NativeCursor.h"
#include "gameplay_logic/lr2_skin/Lr2SkinModel.h"
#include "resource_managers/ChartFactory.h"
#include "resource_managers/DefineDb.h"
#include "resource_managers/Languages.h"
#include "gameplay_logic/rules/Lr2TimingWindows.h"
#include "gameplay_logic/rules/Lr2HitValues.h"
#include "resource_managers/GaugeFactory.h"
#include "support/TimingWindowsFromHash.h"
#include "sounds/AudioPlayer.h"
#include "support/QStringToPath.h"
#include "../arena/FakeArenaIdentityProvider.h"
#include "../arena/FakeArenaScheduler.h"
#include "../arena/FakeArenaTransport.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <QFile>
#include <QImage>
#include <QEventLoop>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>

Q_IMPORT_QML_PLUGIN(RhythmGameQml_Lr2Plugin)

namespace {
void
registerLegacySkinTypes()
{
    static const auto registered = [] {
        qmlRegisterType<gameplay_logic::lr2_skin::Lr2SkinModel>(
          "RhythmGameQml", 1, 0, "Lr2SkinModel");
        qmlRegisterType<qml_components::Lr2NativeCursor>(
          "RhythmGameQml", 1, 0, "Lr2NativeCursor");
        return true;
    }();
    (void)registered;
}
}

TEST_CASE("Legacy sprites keep image crops stable during hits",
          "[qml][LegacyGameplay]")
{
    registerLegacySkinTypes();
    QTemporaryDir directory;
    QImage atlas(64, 16, QImage::Format_RGBA8888);
    atlas.fill(Qt::white);
    const auto imagePath = directory.filePath("atlas.png");
    REQUIRE(atlas.save(imagePath));
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import RhythmGameQml.Lr2
Lr2SpriteRenderer {
    width: 64
    height: 64
    dsts: [{time: 0, x: 0, y: 0, w: 16, h: 16,
            a: 255, r: 255, g: 255, b: 255, blend: 1, loop: -1, timer: 50},
           {time: 250, x: 0, y: 0, w: 16, h: 16,
            a: 255, r: 255, g: 255, b: 255, blend: 1}]
    skinTime: 1000
    timerFire: 1000
    sourceTimerFire: 1000
    // Runtime descriptors arrive after the sprite is created.
    sourceHasFrameAnimation: false
})",
                      QUrl());
    INFO(component.errorString().toStdString());
    REQUIRE(component.isReady());
    QVariantMap source{ { "source", imagePath },
                        { "x", 0 },
                        { "y", 0 },
                        { "w", 64 },
                        { "h", 16 },
                        { "div_x", 4 },
                        { "div_y", 1 },
                        { "cycle", 250 } };
    std::unique_ptr<QObject> sprite(
      component.createWithInitialProperties({ { "srcData", source } }));
    REQUIRE(sprite);
    REQUIRE(sprite->property("useFastImagePath").toBool());
    auto children = qobject_cast<QQuickItem*>(sprite.get())->childItems();
    QQuickItem* croppedImage = nullptr;
    for (qsizetype i = 0; i < children.size(); ++i) {
        auto* child = children.at(i);
        children.append(child->childItems());
        if (child->property("sourceClipRect").toRectF() == QRectF(0, 0, 16, 16))
            croppedImage = child;
    }
    REQUIRE(croppedImage);
    QSignalSpy cropChanges(croppedImage, SIGNAL(sourceClipRectChanged()));
    REQUIRE(cropChanges.isValid());
    auto* frames = sprite->property("animationFrameState").value<QObject*>();
    REQUIRE(frames);

    SECTION("An ordinary sprite keeps its crop when its hit timer stops")
    {
        REQUIRE(sprite->setProperty("timerFire", -1));
        CHECK_FALSE(sprite->property("hasRenderableState").toBool());
        CHECK(croppedImage->property("sourceClipRect").toRectF() ==
              QRectF(0, 0, 16, 16));
        REQUIRE(sprite->setProperty("timerFire", 1000));
        CHECK(sprite->property("hasRenderableState").toBool());
        CHECK(cropChanges.count() == 0);
    }
    SECTION("Animation discovered after creation uses the loaded atlas")
    {
        REQUIRE(sprite->setProperty("sourceHasFrameAnimation", true));
        REQUIRE_FALSE(sprite->property("useFastImagePath").toBool());
        CHECK(croppedImage->property("source").toUrl().isEmpty());
        cropChanges.clear();
        REQUIRE(sprite->setProperty("skinTime", 1100));
        CHECK(frames->property("frameIndex").toInt() == 1);
        REQUIRE(sprite->setProperty("timerFire", -1));
        REQUIRE(sprite->setProperty("timerFire", 1000));
        CHECK(cropChanges.count() == 0);
    }
    SECTION("Manually selected frames use the loaded atlas")
    {
        REQUIRE(sprite->setProperty("frameOverride", 1));
        CHECK(frames->property("frameIndex").toInt() == 1);
        REQUIRE_FALSE(sprite->property("useFastImagePath").toBool());
        cropChanges.clear();
        REQUIRE(sprite->setProperty("frameOverride", 2));
        CHECK(frames->property("frameIndex").toInt() == 2);
        CHECK(cropChanges.count() == 0);
    }
    SECTION("HIDDEN crops the sampled region without changing the image crop")
    {
        source.insert("hiddenCover", true);
        source.insert("hiddenDisappearLine", 8);
        REQUIRE(sprite->setProperty("srcData", source));
        REQUIRE_FALSE(sprite->property("useFastImagePath").toBool());
        CHECK(frames->property("effectiveSourceClipRect").toRectF().height() ==
              8);
        cropChanges.clear();
        source.insert("hiddenDisappearLine", 4);
        REQUIRE(sprite->setProperty("srcData", source));
        CHECK(frames->property("effectiveSourceClipRect").toRectF().height() ==
              4);
        CHECK(cropChanges.count() == 0);
    }
}

TEST_CASE(
  "Legacy gameplay controls follow hits and misses after loading the skin",
  "[qml][LegacyGameplay]")
{
    registerLegacySkinTypes();
    const auto autoplay = GENERATE(false, true);
    CAPTURE(autoplay);
    QTemporaryDir directory;
    const auto root = support::qStringToPath(directory.path());
    db::SqliteCppDb database(root / "songs.sqlite");
    resource_managers::defineDb(database);
    input::InputTranslator input(&database);
    QNetworkAccessManager network;
    sounds::AudioEngine audio;
    resource_managers::SongAssetStore assets;
    qml_components::SongFolderFactory folders(&database);
    resource_managers::Tables tables(
      &network, QDir(directory.filePath("tables")), &database);
    qml_components::ProgramSettings settings(directory.path(),
                                             directory.path());
    const auto skinPath = directory.filePath("play.lr2skin");
    const qml_components::Screen legacyScreen(
      QUrl("qrc:/qt/qml/RhythmGameQml/Lr2/Lr2GameplayScreen.qml"),
      {},
      R"({"items":[]})",
      {},
      false,
      skinPath);
    const QMap<QString, qml_components::ThemeFamily> families{
        { "Default",
          qml_components::ThemeFamily(
            directory.path(),
            { { "k5", legacyScreen }, { "k7", legacyScreen } },
            {}) }
    };
    qml_components::Themes themes(families);
    qml_components::FileQuery fileQuery;
    qml_components::ProfileList profiles(root / "songs.sqlite",
                                         &database,
                                         families,
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
      "#TITLE Legacy scoring\n#BPM 120\n#00011:0001\n#00118:01\n",
      root / "chart.bms",
      [](auto) { return 1; });
    resource_managers::ChartFactory::PlayerSpecificData player{
        .profile = profile,
        .gauges =
          resource_managers::GaugeFactory{}.getStandardGauges(profile, 200, 2),
        .hitRules =
          gameplay_logic::rules::HitRules{
            support::timingWindowsFromHash(
              components.chartData->getTimingWindowsHash()),
            gameplay_logic::rules::lr2_hit_values::getLr2HitValue },
        .replayedScore = nullptr,
        .noteOrderAlgorithm = resource_managers::NoteOrderAlgorithm::Normal,
        .noteOrderAlgorithmP2 = resource_managers::NoteOrderAlgorithm::Normal,
        .randomSeed = 0,
        .autoPlay = autoplay
    };
    auto runner =
      factory.createChart(std::move(components), player, std::nullopt, 2.0);
    auto* chart = runner.get();
    auto* score = chart->getPlayer1()->getScore();
    rhythm_game_qml::ScreenContexts contexts;
    std::unique_ptr<rhythm_game_qml::GameplayContext> gameplay(
      contexts.createGameplay(runner.release()));
    REQUIRE(QTest::qWaitFor(
      [&] { return chart->getStatus() == gameplay_logic::ChartRunner::Ready; },
      5000));
    QQmlEngine engine;
    resource_managers::Languages languages({}, &engine);
    Rg rg{ &settings,  &input,     nullptr, nullptr,   &folders, nullptr,
           &fileQuery, &themes,    nullptr, &profiles, &arena,   nullptr,
           &tables,    &languages, &audio,  nullptr,   &assets };
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
    QFile skin(directory.filePath("play.lr2skin"));
    REQUIRE(skin.open(QIODevice::WriteOnly));
    skin.write("#INFORMATION,0,Test,Test\n#LOADEND,1\n#PLAYSTART,1\n"
               "#IMAGE,atlas.png\n"
               "#SRC_NUMBER,0,0,0,0,100,10,10,1,0,0,101,0,4\n"
               "#DST_NUMBER,0,0,0,0,10,10,0,255,255,255,255,1,0,0,0\n"
               "#SRC_NUMBER,1,0,0,0,100,10,10,1,0,0,105,0,4\n"
               "#DST_NUMBER,1,0,0,20,10,10,0,255,255,255,255,1,0,0,0\n"
               "#SRC_NUMBER,2,0,0,0,100,10,10,1,0,0,107,0,4\n"
               "#DST_NUMBER,2,0,0,30,10,10,0,255,255,255,255,1,0,0,0\n"
               "#SRC_GROOVEGAUGE,0,0,0,0,40,10,4,1,0,0,1,0\n"
               "#DST_GROOVEGAUGE,0,0,0,40,1,10,0,255,255,255,255,1,0,0,0\n");
    skin.close();
    QImage atlas(100, 10, QImage::Format_RGBA8888);
    atlas.fill(Qt::white);
    REQUIRE(atlas.save(directory.filePath("atlas.png")));
    QStringList warnings;
    QObject::connect(
      &engine, &QQmlEngine::warnings, [&](const QList<QQmlError>& errors) {
          for (const auto& error : errors)
              warnings.append(error.toString());
      });
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml
import RhythmGameQml.Lr2
Item {
    id: host
    required property GameplayContext gameplay
    property alias currentScreen: stack.currentItem
    function returnToPreviousScreen() { return false; }
    StackView {
        id: stack
        anchors.fill: parent
    }
    Component {
        id: screen
        // Load the configured CSV during completion, as the game does.
        Lr2GameplayScreen { gameplay: host.gameplay }
    }
    Component.onCompleted: {
        const item = screen.createObject(stack, {enabled: false, visible: false});
        stack.pushItem(item, {}, StackView.Immediate);
        item.enabled = true;
        item.visible = true;
    }
})",
                      QUrl());
    INFO(component.errorString().toStdString());
    REQUIRE(component.isReady());
    std::unique_ptr<QObject> host(component.beginCreate(engine.rootContext()));
    REQUIRE(host);
    engine.rootContext()->setContextProperty("globalRoot", host.get());
    component.setInitialProperties(
      host.get(), { { "gameplay", QVariant::fromValue(gameplay.get()) } });
    component.completeCreate();
    auto* item = qobject_cast<QQuickItem*>(host.get());
    REQUIRE(item);
    QQuickWindow window;
    window.resize(640, 480);
    item->setSize(window.size());
    item->setParentItem(window.contentItem());
    window.show();
    settings.attachFrameRateLimiter(&window);
    settings.setContinuousRendering(true);
    const auto started = QTest::qWaitFor(
      [&] {
          return chart->getStatus() == gameplay_logic::ChartRunner::Running;
      },
      15000);
    INFO(warnings.join('\n').toStdString());
    REQUIRE(started);
    engine.collectGarbage();
    if (!autoplay) {
        REQUIRE(QTest::qWaitFor(
          [&] {
              return chart->currentOffsetFromStart() >=
                     std::chrono::milliseconds(950);
          },
          3000));
        const auto now =
          std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch() -
            chart->currentOffsetFromStart() + std::chrono::seconds(1))
            .count();
        input.buttonPressed(input::BmsKey::Col11, now);
        input.buttonReleased(input::BmsKey::Col11, now + 1);
    }
    QEventLoop playLoop;
    QTimer::singleShot(2300, &playLoop, &QEventLoop::quit);
    playLoop.exec();
    INFO(warnings.join('\n').toStdString());
    CHECK(chart->getPlayer1()->getElapsed() > 0);
    CHECK(score->getPoints() > 0);
    CHECK(score->getMaxCombo() > 0);
    CHECK(score->judgementCount(static_cast<int>(
            gameplay_logic::Judgement::Poor)) == (autoplay ? 0 : 1));
    auto* currentScreen = host->property("currentScreen").value<QObject*>();
    REQUIRE(currentScreen);
    auto children = qobject_cast<QQuickItem*>(currentScreen)->childItems();
    for (qsizetype i = 0; i < children.size(); ++i) {
        children.append(children.at(i)->childItems());
    }
    auto checkedScore = false;
    auto checkedCombo = false;
    auto checkedGauge = false;
    auto checkedGaugeNumber = false;
    auto checkedRuntimeState = false;
    const auto gauge = currentScreen->property("gameplayFrameStateRef")
                         .value<QObject*>()
                         ->property("gaugeValue1")
                         .toDouble();
    for (auto* child : children) {
        if (child->property("elementIndex").isValid()) {
            checkedRuntimeState = true;
            CHECK(
              child->property("elementActiveOptionsState").value<QObject*>());
            CHECK(child->property("elementTimerState").value<QObject*>());
            CHECK(child->property("elementNumberState").value<QObject*>());
        }
        const auto numberId = child->property("numberId");
        if (numberId.isValid() && numberId.toInt() == 101) {
            checkedScore = true;
            CHECK(child->property("value").toDouble() == score->getPoints());
        }
        if (numberId.isValid() && numberId.toInt() == 105) {
            checkedCombo = true;
            CHECK(child->property("value").toInt() == score->getMaxCombo());
        }
        if (numberId.isValid() && numberId.toInt() == 107) {
            checkedGaugeNumber = true;
            CHECK(child->property("value").toDouble() == std::floor(gauge));
        }
        const auto gaugeValue = child->property("gaugeValue");
        if (gaugeValue.isValid()) {
            checkedGauge = true;
            CHECK(gaugeValue.toDouble() == gauge);
        }
    }
    CHECK(checkedScore);
    CHECK(checkedCombo);
    CHECK(checkedGauge);
    CHECK(checkedGaugeNumber);
    CHECK(checkedRuntimeState);
}

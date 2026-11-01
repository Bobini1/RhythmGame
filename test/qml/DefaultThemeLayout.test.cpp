#include "Rg.h"
#include "resource_managers/ChartFactory.h"
#include "resource_managers/GaugeFactory.h"
#include "resource_managers/Languages.h"
#include "gameplay_logic/rules/Lr2TimingWindows.h"
#include "qml_components/InputAttached.h"
#include "resource_managers/DefineDb.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScopeGuard>
#include <QTemporaryDir>

namespace {
class EmptyImageProvider : public QQuickImageProvider
{
  public:
    EmptyImageProvider()
      : QQuickImageProvider(Image)
    {
    }

    QImage requestImage(const QString&, QSize* size, const QSize&) override
    {
        QImage image(1, 1, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        *size = image.size();
        return image;
    }
};
} // namespace

TEST_CASE("Default gameplay resizing preserves saved settings",
          "[qml][DefaultTheme]")
{
    QTemporaryDir directory;
    const auto root = support::qStringToPath(directory.path());
    const auto themePath = QString::fromUtf8(RHYTHMGAME_SOURCE_DIR) +
                           "/share/RhythmGame/themes/Default";
    const qml_components::ThemeFamily theme(
      themePath,
      { { "k7",
          qml_components::Screen{
            QUrl::fromLocalFile(themePath + "/scripts/gameplay/Gameplay.qml"),
            QUrl::fromLocalFile(themePath + "/settings/k7.json"),
            {},
            {} } } },
      {});
    db::SqliteCppDb database(root / "songs.sqlite");
    resource_managers::defineDb(database);
    input::InputTranslator input(&database);
    QNetworkAccessManager network;
    sounds::AudioEngine audio;
    resource_managers::SongAssetStore assets;
    resource_managers::Profile profile(root / "songs.sqlite",
                                       root / "profile.sqlite",
                                       { { "Default", theme } },
                                       {},
                                       &network,
                                       &assets);
    auto* screenSettings =
      profile.getVars()->getThemeVars()->value("k7").value<QQmlPropertyMap*>();
    REQUIRE(screenSettings);
    auto* settings = screenSettings->value("Default").value<QQmlPropertyMap*>();
    REQUIRE(settings);
    const auto snapshot = [&] {
        QVariantMap values;
        for (const auto& key : settings->keys())
            values.insert(key, settings->value(key));
        return values;
    };
    const auto initial = snapshot();

    resource_managers::ChartFactory factory(&audio, &input, &assets);
    auto components = resource_managers::ChartDataFactory{}.loadChartData(
      "#TITLE Layout\n#BPM 120\n#01011:01\n", root / "chart.bms", [](auto) {
          return 1;
      });
    resource_managers::ChartFactory::PlayerSpecificData player{
        .profile = &profile,
        .gauges =
          resource_managers::GaugeFactory{}.getStandardGauges(&profile, 200, 1),
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

    QQmlEngine engine;
    engine.addImageProvider("test", new EmptyImageProvider);
    resource_managers::Languages languages({}, &engine);
    Rg rg{ nullptr, &input,     nullptr, nullptr, nullptr, nullptr,
           nullptr, nullptr,    nullptr, nullptr, nullptr, nullptr,
           nullptr, &languages, &audio,  nullptr, &assets };
    auto* previousRg = Rg::instance;
    auto* previousInput = qml_components::InputAttached::inputSignalProvider;
    Rg::instance = &rg;
    qml_components::InputAttached::inputSignalProvider = &input;
    const auto restore = qScopeGuard([&] {
        Rg::instance = previousRg;
        qml_components::InputAttached::inputSignalProvider = previousInput;
    });
    QStringList warnings;
    QObject::connect(
      &engine, &QQmlEngine::warnings, [&](const QList<QQmlError>& errors) {
          for (const auto& error : errors)
              warnings.append(error.toString());
      });
    QQmlComponent component(&engine);
    component.setData(
      R"(
import QtQuick
import RhythmGameQml
import "." as Gameplay
Item {
    id: root
    required property Player player
    required property ChartData chartData
    property string screen: "k7"
    property string themeName: "Default"
    property string imagesUrl: "image://test/"
    property string iniImagesUrl: "image://test/"
    property bool customizeMode: true
    function getColumnSizes(vars) {
        return Array.from({length: 16}, (_, i) => i % 8 === 7
            ? vars.scratchWidth : i % 2 === 0 ? vars.whiteWidth : vars.blackWidth);
    }
    Gameplay.Side {
        anchors.fill: parent
        player: root.player
        columns: [7, 0, 1, 2, 3, 4, 5, 6]
        dpSuffix: ""
        index: 0
        pointTarget: 0
    }
})",
      QUrl::fromLocalFile(themePath + "/scripts/gameplay/LayoutTest.qml"));
    INFO(component.errorString().toStdString());
    REQUIRE(component.isReady());
    std::unique_ptr<QObject> scene(component.createWithInitialProperties(
      { { "player", QVariant::fromValue(runner->getPlayer1()) },
        { "chartData", QVariant::fromValue(runner->getChartData()) } }));
    INFO(component.errorString().toStdString());
    REQUIRE(scene);
    QQuickWindow window;
    auto* sceneItem = qobject_cast<QQuickItem*>(scene.get());
    REQUIRE(sceneItem);
    sceneItem->setParentItem(window.contentItem());
    window.resize(1280, 720);
    sceneItem->setSize(QSizeF(1280, 720));
    window.show();
    QCoreApplication::processEvents();
    CHECK(snapshot() == initial);

    for (const auto size :
         { QSizeF(800, 600), QSizeF(1920, 1080), QSizeF(900, 1200) }) {
        sceneItem->setSize(size);
        QCoreApplication::processEvents();
        CHECK(snapshot() == initial);
    }
    INFO(warnings.join('\n').toStdString());
    CHECK(warnings.isEmpty());
}

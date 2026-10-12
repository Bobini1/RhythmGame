#include "Rg.h"
#include "resource_managers/ChartFactory.h"
#include "resource_managers/GaugeFactory.h"
#include "resource_managers/Languages.h"
#include "gameplay_logic/rules/Lr2TimingWindows.h"
#include "qml_components/InputAttached.h"
#include "resource_managers/DefineDb.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <QQmlComponent>
#include <QQmlContext>
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

TEST_CASE(
  "Default profile chooser follows external selection after a wheel edit",
  "[qml][DefaultTheme]")
{
    QTemporaryDir directory;
    const auto root = support::qStringToPath(directory.path());
    db::SqliteCppDb database(root / "songs.sqlite");
    resource_managers::defineDb(database);
    input::InputTranslator input(&database);
    QNetworkAccessManager network;
    resource_managers::SongAssetStore assets;
    qml_components::ProfileList profiles(root / "songs.sqlite",
                                         &database,
                                         {},
                                         root / "profiles",
                                         {},
                                         &network,
                                         &assets);
    REQUIRE(profiles.createProfile());
    REQUIRE(profiles.createProfile());
    auto* initial = profiles.at(2);
    profiles.setMainProfile(initial);

    Rg rg{ nullptr, &input,  nullptr, nullptr,   nullptr, nullptr,
           nullptr, nullptr, nullptr, &profiles, nullptr, nullptr,
           nullptr, nullptr, nullptr, nullptr,   &assets };
    auto* previousRg = Rg::instance;
    auto* previousInput = qml_components::InputAttached::inputSignalProvider;
    Rg::instance = &rg;
    qml_components::InputAttached::inputSignalProvider = &input;
    const auto restore = qScopeGuard([&] {
        Rg::instance = previousRg;
        qml_components::InputAttached::inputSignalProvider = previousInput;
    });

    QQmlEngine engine;
    engine.addImageProvider("test", new EmptyImageProvider);
    engine.addImageProvider("avatar", new EmptyImageProvider);
    QStringList warnings;
    QObject::connect(
      &engine, &QQmlEngine::warnings, [&](const QList<QQmlError>& errors) {
          for (const auto& error : errors)
              warnings.append(error.toString());
      });
    const auto source =
      QString::fromUtf8(RHYTHMGAME_SOURCE_DIR) +
      "/share/RhythmGame/themes/Default/scripts/select/options/";
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import "." as Options
Item {
    id: root
    property string iniImagesUrl: "image://test/"
    Options.Login { scale: 1 }
}
)",
                      QUrl::fromLocalFile(source + "ChooserTest.qml"));
    INFO(component.errorString().toStdString());
    REQUIRE(component.isReady());
    std::unique_ptr<QObject> chooser(component.create());
    INFO(component.errorString().toStdString());
    REQUIRE(chooser);
    QCoreApplication::processEvents();

    QObject* card = nullptr;
    QObject* wheel = nullptr;
    for (auto* child : chooser->findChildren<QObject*>()) {
        if (child->property("playerNumber").isValid())
            card = child;
        if (child->property("pathItemCount").toInt() == 3)
            wheel = child;
    }
    REQUIRE(card);
    REQUIRE(wheel);
    const auto selected = [&] {
        return card->property("profile").value<resource_managers::Profile*>();
    };
    CHECK(profiles.getMainProfile() == initial);
    CHECK(selected() == initial);
    CHECK(wheel->property("currentIndex").toInt() == 2);

    REQUIRE(QMetaObject::invokeMethod(wheel, "decrementCurrentIndex"));
    QCoreApplication::processEvents();
    CHECK(profiles.getMainProfile() == profiles.at(1));
    CHECK(selected() == profiles.at(1));

    profiles.setMainProfile(profiles.at(0));
    QCoreApplication::processEvents();
    CHECK(selected() == profiles.at(0));
    CHECK(wheel->property("currentIndex").toInt() == 0);

    REQUIRE(QMetaObject::invokeMethod(wheel, "incrementCurrentIndex"));
    QCoreApplication::processEvents();
    CHECK(profiles.getMainProfile() == profiles.at(1));
    CHECK(selected() == profiles.at(1));
    INFO(warnings.join('\n').toStdString());
    CHECK(warnings.isEmpty());
}

TEST_CASE("Default gameplay layout only saves explicit edits",
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

    QObject* playArea = nullptr;
    QObject* playAreaEditor = nullptr;
    QObject* fastslow = nullptr;
    QObject* ghostScore = nullptr;
    QObject* judgementEditor = nullptr;
    QObject* fastslowEditor = nullptr;
    QObject* ghostEditor = nullptr;
    for (auto* child : scene->findChildren<QObject*>()) {
        if (!child->property("dragAxis").isValid())
            continue;
        auto* control = child->property("control").value<QQuickItem*>();
        REQUIRE(control);
        if (control->property("heightMultiplier").isValid()) {
            playArea = control;
            playAreaEditor = child;
        } else if (control->property("fast").isValid()) {
            fastslow = control;
            fastslowEditor = child;
        } else if (control->property("points").isValid()) {
            ghostScore = control;
            ghostEditor = child;
        } else if (child->property("dragAxis").toInt() == 2)
            judgementEditor = child;
    }
    REQUIRE(playArea);
    REQUIRE(fastslow);
    REQUIRE(ghostScore);
    REQUIRE(judgementEditor);
    REQUIRE(fastslowEditor);
    REQUIRE(ghostEditor);
    REQUIRE(ghostScore->property("width").toDouble() > 0);

    // Default consumes display. Hits must update it before notes are culled.
    auto* state = runner->getPlayer1()->getState();
    state->setVisiblePositionSpans(4, 4);
    state->setPosition(39);
    QCoreApplication::processEvents();
    QQuickItem* drawnNote = nullptr;
    auto noteItems = sceneItem->childItems();
    for (qsizetype i = 0; i < noteItems.size(); ++i) {
        auto* item = noteItems[i];
        noteItems.append(item->childItems());
        if (item->property("hitData").isValid())
            drawnNote = item;
    }
    REQUIRE(drawnNote);
    REQUIRE(drawnNote->isVisible());
    state->getColumnStates().front()->onHitEvent(
      { 0,
        0,
        0,
        gameplay_logic::BmsPoints(2, gameplay_logic::Judgement::Perfect, 0),
        gameplay_logic::HitEvent::Action::Press,
        true });
    QCoreApplication::processEvents();
    CHECK_FALSE(drawnNote->isVisible());

    // Exercise Default's actual LN image geometry, including an end at zero.
    QQmlComponent columnComponent(&engine);
    columnComponent.setData(
      R"(
import QtQuick
import RhythmGameQml
import "." as Gameplay
Item {
    id: root
    required property Player player
    required property var noteModel
    required property var notes
    property string iniImagesUrl: "image://test/"
    Gameplay.NoteColumn {
        width: 100
        height: 600
        player: root.player
        columnState: root.noteModel
        notes: root.notes
        color: "white"
        noteImage: "test"
        mineImage: "test"
        heightMultiplier: 100
        noteHeight: 10
        hideLnEnds: false
        liftRatio: 1
        flashing: false
    }
})",
      QUrl::fromLocalFile(themePath + "/scripts/gameplay/NoteTest.qml"));
    INFO(columnComponent.errorString().toStdString());
    REQUIRE(columnComponent.isReady());
    using namespace gameplay_logic;
    for (const auto positions :
         { std::pair{ 1.0, 3.0 }, { 3.0, 1.0 }, { 2.0, 0.0 } }) {
        CAPTURE(positions.first, positions.second);
        const QList<Note> notes{ { Time{ 0, positions.first, positions.first },
                                   Snap{ 0, 1 },
                                   Note::Type::LongNoteBegin },
                                 { Time{
                                     1, positions.second, positions.second },
                                   Snap{ 0, 1 },
                                   Note::Type::LongNoteEnd } };
        ColumnState source(
          { NoteState{ notes[0], 0 }, NoteState{ notes[1], 1 } });
        Filter filter(&source);
        filter.setVisibleRange(0, 4);
        std::unique_ptr<QObject> columnScene(
          columnComponent.createWithInitialProperties(
            { { "player", QVariant::fromValue(runner->getPlayer1()) },
              { "noteModel", QVariant::fromValue(&filter) },
              { "notes", QVariant::fromValue(notes) } }));
        INFO(columnComponent.errorString().toStdString());
        REQUIRE(columnScene);
        QCoreApplication::processEvents();
        QQuickItem* body = nullptr;
        auto bodyItems =
          qobject_cast<QQuickItem*>(columnScene.get())->childItems();
        for (qsizetype i = 0; i < bodyItems.size(); ++i) {
            auto* item = bodyItems[i];
            bodyItems.append(item->childItems());
            if (item->property("bodyLength").isValid())
                body = item;
        }
        INFO(warnings.join('\n').toStdString());
        REQUIRE(body);
        CHECK(body->height() == Catch::Approx(180));
        CHECK(body->y() ==
              Catch::Approx(positions.first < positions.second ? -180 : 20));
    }

    for (const auto size :
         { QSizeF(800, 600), QSizeF(1920, 1080), QSizeF(900, 1200) }) {
        sceneItem->setSize(size);
        QCoreApplication::processEvents();
        CHECK(snapshot() == initial);
    }
    auto* area = qobject_cast<QQuickItem*>(playArea);
    const auto blackWidth = settings->value("blackWidth").toDouble();
    const auto whiteWidth = settings->value("whiteWidth").toDouble();
    const auto scratchWidth = settings->value("scratchWidth").toDouble();
    const auto gaps = settings->value("spacing").toDouble() * 7;
    const QRectF areaResized(
      area->x() + 10, area->y() + 20, (area->width() - gaps) * 1.5 + gaps, 720);
    REQUIRE(QMetaObject::invokeMethod(
      playAreaEditor, "geometryEdited", Q_ARG(QRectF, areaResized)));
    CHECK(area->width() == Catch::Approx(areaResized.width()));
    CHECK(area->height() == 720);
    CHECK(settings->value("blackWidth").toDouble() ==
          Catch::Approx(blackWidth * 1.5));
    CHECK(settings->value("whiteWidth").toDouble() ==
          Catch::Approx(whiteWidth * 1.5));
    CHECK(settings->value("scratchWidth").toDouble() ==
          Catch::Approx(scratchWidth * 1.5));
    settings->insert("playAreaHeight", 640.0);
    settings->insert("judgementsHeight", 0.2);
    const auto resized = snapshot();
    QCoreApplication::processEvents();
    CHECK(playArea->property("height").toDouble() == 640.0);
    CHECK(snapshot() == resized);

    for (auto* editor : { judgementEditor, fastslowEditor, ghostEditor }) {
        auto* control = editor->property("control").value<QQuickItem*>();
        const QRectF moved(
          control->x(), control->y() + 20, control->width(), control->height());
        REQUIRE(QMetaObject::invokeMethod(
          editor, "geometryEdited", Q_ARG(QRectF, moved)));
        QCoreApplication::processEvents();
        CHECK(control->y() == Catch::Approx(moved.y()));
    }
    settings->insert("fastslowX", 2.0);
    settings->insert("fastslowY", 1.5);
    settings->insert("ghostScoreX", -1.0);
    settings->insert("ghostScoreY", 2.0);
    const auto edited = snapshot();
    settings->insert("judgementsHeight", 0.1);
    settings->insert("ghostScoreFont", "system:Monospace");
    ghostScore->setProperty("points", -99999);
    QCoreApplication::processEvents();
    for (const auto* key : { "fastslowX",
                             "fastslowY",
                             "fastslowHeight",
                             "ghostScoreX",
                             "ghostScoreY",
                             "ghostScoreHeight",
                             "judgementsOffset" })
        CHECK(settings->value(key) == edited.value(key));
    CHECK(fastslow->property("height").toDouble() ==
          Catch::Approx(settings->value("fastslowHeight").toDouble() * 64));
    CHECK(ghostScore->property("height").toDouble() ==
          Catch::Approx(settings->value("ghostScoreHeight").toDouble() * 64));
    INFO(warnings.join('\n').toStdString());
    CHECK(warnings.isEmpty());
}

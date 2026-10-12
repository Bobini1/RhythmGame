#include "Rg.h"
#include "gameplay_logic/BmsScoreCourse.h"
#include "qml_components/InputAttached.h"
#include "resource_managers/DefineDb.h"
#include "support/QStringToPath.h"
#include "../arena/FakeArenaIdentityProvider.h"
#include "../arena/FakeArenaScheduler.h"
#include "../arena/FakeArenaTransport.h"

#include <catch2/catch_test_macros.hpp>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QWheelEvent>

#include <memory>

TEST_CASE("Tutorial result graphs use loaded chart and course score data",
          "[qml][ThemeAuthoring][ResultGraphs]")
{
    using namespace gameplay_logic;
    auto result = BmsResult::fromJson(
      { { "guid", "graph-test" },
        { "length", 12e9 },
        { "clearType", "NORMAL" },
        { "judgementCounts", QJsonArray{ 1, 1, 2, 1, 1, 4, 1, 0, 0, 0 } } });
    QList<HitEvent> events;
    for (const auto [judgement, millis] : { std::pair{ Judgement::Perfect, 0 },
                                            { Judgement::Perfect, 0 },
                                            { Judgement::Perfect, 0 },
                                            { Judgement::Perfect, 0 },
                                            { Judgement::Great, -15 },
                                            { Judgement::Good, 25 },
                                            { Judgement::Bad, -180 },
                                            { Judgement::Bad, 200 },
                                            { Judgement::Poor, 300 },
                                            { Judgement::EmptyPoor, -300 },
                                            { Judgement::MineHit, 0 },
                                            { Judgement::LnEndSkip, 0 } }) {
        events.append({ 0,
                        0,
                        0,
                        BmsPoints(0, judgement, millis * 1000000LL),
                        HitEvent::Action::Press,
                        true });
    }
    events.append(
      { 0, std::nullopt, 0, std::nullopt, HitEvent::Action::Release, false });
    const QList<BmsGaugeInfo> gauges{
        { 100,
          0,
          "HARD",
          false,
          { { 0, 100 }, { 4'000'000'000, 40 }, { 8'000'000'000, 0 } } },
        { 200,
          160,
          "NORMAL",
          false,
          { { 0, 100 },
            { 4'000'000'000, 180 },
            { 8'000'000'000, 120 },
            { 10'000'000'000, 160 } } }
    };
    auto score = std::make_unique<BmsScore>(
      std::move(result),
      std::make_unique<BmsReplayData>(events, "graph-test"),
      std::make_unique<BmsGaugeHistory>(gauges, "graph-test"));
    auto imported = BmsScore::fromImportedResult(score->getResult()->clone(),
                                                 BmsScore::Source::Lr2);
    QList<BmsScore*> stages{ score->clone().release(),
                             score->clone().release() };
    auto course = BmsScoreCourse::fromScores(
      std::make_unique<BmsResultCourse>(
        "course", "course-id", stages, "NORMAL", 0, QStringList{}),
      stages);
    auto empty = std::make_unique<BmsScore>(
      BmsResult::fromJson({ { "length", 0 } }),
      std::make_unique<BmsReplayData>(QList<HitEvent>{}, "empty"),
      std::make_unique<BmsGaugeHistory>(QList<BmsGaugeInfo>{}, "empty"));

    for (const auto* lesson : { "08-results", "10-complete" }) {
        CAPTURE(lesson);
        QQmlEngine engine;
        QStringList warnings;
        QObject::connect(
          &engine, &QQmlEngine::warnings, [&](const QList<QQmlError>& errors) {
              for (const auto& error : errors)
                  warnings.append(error.toString());
          });
        QQmlComponent component(
          &engine,
          QUrl::fromLocalFile(QStringLiteral(RHYTHMGAME_SOURCE_DIR
                                             "/docs/examples/theme-tutorial/") +
                              lesson + "/ResultStats.qml"));
        INFO(component.errorString().toStdString());
        REQUIRE(component.isReady());
        std::unique_ptr<QObject> view(component.createWithInitialProperties(
          { { "score", QVariant::fromValue(score.get()) } }));
        REQUIRE(view);
        QObject* histogram = nullptr;
        QObject* life = nullptr;
        for (auto* child : view->findChildren<QObject*>()) {
            if (child->property("histogram").isValid())
                histogram = child;
            if (child->property("gauges").isValid())
                life = child;
        }
        REQUIRE(histogram);
        REQUIRE(life);
        engine.globalObject().setProperty("histogram",
                                          engine.newQObject(histogram));
        engine.globalObject().setProperty("life", engine.newQObject(life));
        const auto check = [&](const QString& expression) {
            auto value = engine.evaluate(expression);
            INFO(expression.toStdString());
            INFO(value.toString().toStdString());
            REQUIRE_FALSE(value.isError());
            CHECK(value.toBool());
        };
        check("histogram.histogram.total === 8 && histogram.histogram.limit "
              "=== 200");
        check("histogram.histogram.counts[20] === 4 && "
              "histogram.histogram.counts[2] === 1 && "
              "histogram.histogram.counts[40] === 1");
        check("life.gauge.name === 'NORMAL' && life.duration === 12000000000");
        check("life.points[0].x === 0 && life.points[0].y === 0.5");
        check("Math.abs(life.points[life.points.length - 1].y - 0.2) < 0.00001 "
              "&& life.points[life.points.length - 1].x === 1");
        check("life.points[3].y === 0.5 && life.points[4].y < 0.11");

        QObject* choice = nullptr;
        for (auto* child : life->findChildren<QObject*>()) {
            if (child->property("currentIndex").isValid()) {
                choice = child;
                break;
            }
        }
        REQUIRE(choice);
        choice->setProperty("currentIndex", 0);
        check("life.gauge.name === 'HARD' && life.points[life.points.length - "
              "1].y === 1");
        view->setProperty("score", QVariant::fromValue(course.get()));
        check(
          "histogram.histogram.total === 16 && life.duration === 24000000000");
        view->setProperty("score", QVariant::fromValue(imported.get()));
        check("histogram.histogram.total === 0 && life.points.length === 0 && "
              "life.gauge === null");
        view->setProperty("score", QVariant::fromValue(empty.get()));
        check("histogram.histogram.total === 0 && life.points.length === 0 && "
              "life.duration === 1");
        view->setProperty("score", QVariant::fromValue(score.get()));

        QQuickWindow window;
        auto* item = qobject_cast<QQuickItem*>(view.get());
        REQUIRE(item);
        item->setParentItem(window.contentItem());
        window.show();
        for (const auto size :
             { QSize(1200, 360), QSize(640, 1000), QSize(360, 1000) }) {
            window.resize(size);
            item->setSize(size);
            QTest::qWait(50);
            CHECK(view->property("columns").toInt() ==
                  (size.width() < 720 ? 1 : 3));
            auto* lifeItem = qobject_cast<QQuickItem*>(life);
            auto* timingItem = qobject_cast<QQuickItem*>(histogram);
            CHECK(lifeItem->width() > 0);
            CHECK(timingItem->width() > 0);
            CHECK(lifeItem->x() + lifeItem->width() <= item->width());
            CHECK(lifeItem->y() + lifeItem->height() <= item->height());
            const auto screenshots =
              qEnvironmentVariable("RHYTHMGAME_TEST_SCREENSHOTS");
            if (!screenshots.isEmpty()) {
                QDir().mkpath(screenshots);
                auto capture = item->grabToImage();
                REQUIRE(capture);
                QSignalSpy ready(capture.get(), &QQuickItemGrabResult::ready);
                REQUIRE(ready.wait(1000));
                CHECK(capture->saveToFile(screenshots + '/' + lesson + '-' +
                                          QString::number(size.width()) +
                                          ".png"));
            }
        }
        INFO(warnings.join('\n').toStdString());
        CHECK(warnings.isEmpty());
    }
}

TEST_CASE("Theme list keeps navigation and saved choices together",
          "[qml][ThemeAuthoring]")
{
    QTemporaryDir directory;
    const auto path = support::qStringToPath(directory.path());
    db::SqliteCppDb database(path / "songs.sqlite");
    resource_managers::defineDb(database);
    database.execute("INSERT INTO parent_dir(id,parent_dir,dir) VALUES "
                     "(1,NULL,'A/'),(2,NULL,'B/'),(3,'B/','B/C/')");
    input::InputTranslator input(&database);
    QNetworkAccessManager network;
    sounds::AudioEngine audio;
    resource_managers::SongAssetStore assets;
    qml_components::SongFolderFactory folders(&database);
    resource_managers::Tables tables(
      &network, QDir(directory.filePath("tables")), &database);
    qml_components::ProfileList profiles(path / "songs.sqlite",
                                         &database,
                                         {},
                                         path / "profiles",
                                         {},
                                         &network,
                                         &assets);
    auto* first = profiles.getMainProfile();
    auto* second = profiles.createProfile();
    REQUIRE(first);
    REQUIRE(second);
    profiles.setMainProfile(first);
    arena::test::FakeArenaTransport transport;
    arena::test::FakeArenaIdentityProvider identity;
    arena::test::FakeArenaScheduler scheduler;
    arena::ArenaSession arena(&transport,
                              &identity,
                              &scheduler,
                              QUrl("wss://arena.example.test"),
                              "test");
    Rg rg{ nullptr, &input,  nullptr, nullptr,   &folders, nullptr,
           nullptr, nullptr, nullptr, &profiles, &arena,   nullptr,
           &tables, nullptr, &audio,  nullptr,   &assets };
    auto* previousRg = Rg::instance;
    auto* previousInput = qml_components::InputAttached::inputSignalProvider;
    Rg::instance = &rg;
    qml_components::InputAttached::inputSignalProvider = &input;
    const auto restore = qScopeGuard([&] {
        Rg::instance = previousRg;
        qml_components::InputAttached::inputSignalProvider = previousInput;
    });
    QStringList warnings;
    QQmlEngine engine;
    QObject::connect(
      &engine, &QQmlEngine::warnings, [&](const QList<QQmlError>& errors) {
          for (const auto& error : errors)
              warnings.append(error.toString());
      });
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import QtQuick.Controls.FluentWinUI3
import RhythmGameQml
Item {
    id: screen
    width: 640
    height: 480
    property alias selection: selection
    property alias list: list
    property bool wheelOverlayVisible: false
    property int overlayWheelEvents: 0
    readonly property int k7: SelectKeymodeFilter.K7
    readonly property int k5: SelectKeymodeFilter.K5
    readonly property int allKeys: SelectKeymodeFilter.All
    readonly property int titleSort: SelectSortMode.Title
    readonly property int levelSort: SelectSortMode.Level
    StandardSelectController {
        id: selection
        preview: null
        ranking: null
        backgroundMusic: null
        feedbackEnabled: false
        previewFilesEnabled: false
        folderClearStatsEnabled: false
    }
    StandardSelectListView {
        id: list
        anchors.fill: parent
        controller: selection
        focus: true
        delegate: ItemDelegate {
            required property int index
            required property var modelData
            width: ListView.view.width
            height: 40
            text: modelData
            onClicked: list.activate(index)
        }
    }
    Item {
        anchors.fill: list
        visible: screen.wheelOverlayVisible
        WheelHandler {
            onWheel: ++screen.overlayWheelEvents
        }
    }
})",
                      QUrl("qrc:/ThemeAuthoringTest.qml"));
    INFO(component.errorString().toStdString());
    REQUIRE(component.isReady());
    std::unique_ptr<QObject> screen(component.create());
    INFO(component.errorString().toStdString());
    REQUIRE(screen);
    QQuickWindow window;
    auto* root = qobject_cast<QQuickItem*>(screen.get());
    root->setParentItem(window.contentItem());
    window.resize(640, 480);
    window.show();
    auto* list = screen->property("list").value<QQuickItem*>();
    REQUIRE(list);
    list->forceActiveFocus();
    engine.globalObject().setProperty("screen",
                                      engine.newQObject(screen.get()));
    const auto settle = [] {
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
    };
    const auto run = [&](const QString& script) {
        const auto result = engine.evaluate(script);
        INFO(script.toStdString());
        INFO(result.toString().toStdString());
        REQUIRE_FALSE(result.isError());
        settle();
    };
    const auto check = [&](const QString& expression) {
        const auto result = engine.evaluate(expression);
        INFO(expression.toStdString());
        INFO(result.toString().toStdString());
        INFO(engine
               .evaluate("JSON.stringify({count: screen.list.count, "
                         "index: screen.list.currentIndex, "
                         "focused: screen.selection.focusedItem, "
                         "history: screen.selection.historyStack})")
               .toString()
               .toStdString());
        REQUIRE_FALSE(result.isError());
        CHECK(result.toBool());
    };
    settle();

    SECTION(
      "arrows, activation, empty folders and return restore the same selection")
    {
        check("screen.list.count === 2 && screen.list.currentIndex === 0");
        QTest::keyClick(&window, Qt::Key_Down);
        settle();
        check("screen.list.currentIndex === 1 && screen.selection.focusedItem "
              "=== 'B/'");
        QTest::keyClick(&window, Qt::Key_Return);
        settle();
        check(
          "screen.list.count === 1 && screen.selection.focusedItem === 'B/C/'");
        run("screen.list.activate(0)");
        check("screen.list.count === 0 && screen.list.currentIndex === -1 && "
              "screen.selection.focusedItem === null");
        QTest::keyClick(&window, Qt::Key_Down);
        settle();
        check("screen.list.currentIndex === -1");
        run("screen.selection.goBack(); screen.selection.goBack()");
        check("screen.list.currentIndex === 1 && screen.selection.focusedItem "
              "=== 'B/'");
        QTest::keyClick(&window, Qt::Key_Down);
        settle();
        check("screen.list.currentIndex === 0 && screen.selection.focusedItem "
              "=== 'A/'");
        run("screen.selection.inputEnabled = false");
        QTest::keyClick(&window, Qt::Key_Down);
        QTest::keyClick(&window, Qt::Key_Return);
        settle();
        check("screen.list.count === 2 && screen.list.currentIndex === 0");
        run(
          "screen.selection.inputEnabled = true; screen.list.currentIndex = 1");
        check("screen.selection.focusedItem === 'B/'");
        database.execute(
          "INSERT INTO parent_dir(parent_dir,dir) VALUES (NULL,'0/')");
        emit folders.contentsChanged();
        settle();
        check("screen.list.currentIndex === 2 && screen.selection.focusedItem "
              "=== 'B/'");
        for (const auto size : { QSize(400, 800), QSize(1280, 480) }) {
            root->setSize(size);
            window.resize(size);
            settle();
            CHECK(list->size() == QSizeF(size));
            check("screen.list.currentIndex === 2 && "
                  "screen.selection.focusedItem === 'B/'");
        }
    }

    SECTION("wheel input selects rows through the controller")
    {
        database.execute("INSERT INTO parent_dir(parent_dir,dir) VALUES "
                         "(NULL,'C/'),(NULL,'D/')");
        emit folders.contentsChanged();
        run("screen.list.anchors.margins = 180");
        check("screen.list.count === 4 && screen.list.currentIndex === 0");
        auto* selection = screen->property("selection").value<QObject*>();
        REQUIRE(selection);
        QSignalSpy moves(selection, SIGNAL(moveRequested(int, bool, bool)));
        REQUIRE(moves.isValid());
        const auto wheel =
          [&](QPoint angle, QPoint pixels = {}, bool inside = true) {
              const auto position =
                inside ? list->mapToScene(QPointF(20, 20)) : QPointF(10, 10);
              QWheelEvent event(position,
                                window.mapToGlobal(position.toPoint()),
                                pixels,
                                angle,
                                Qt::NoButton,
                                Qt::NoModifier,
                                Qt::NoScrollPhase,
                                false);
              QCoreApplication::sendEvent(&window, &event);
              settle();
          };

        wheel({ 0, -120 });
        check("screen.list.currentIndex === 1 && "
              "screen.selection.focusedItem === 'B/'");
        REQUIRE(moves.count() == 1);
        CHECK(moves.last().at(0).toInt() == 1);
        CHECK(moves.last().at(2).toBool());
        wheel({ 0, 120 });
        check("screen.list.currentIndex === 0");
        wheel({ 0, 120 });
        check("screen.list.currentIndex === 3 && "
              "screen.selection.focusedItem === 'D/'");
        wheel({ 0, -240 });
        check("screen.list.currentIndex === 1");

        // Fine wheel deltas accumulate without moving the viewport separately.
        const auto beforeContentY = list->property("contentY").toReal();
        moves.clear();
        wheel({ 0, -50 });
        check("screen.list.currentIndex === 1");
        CHECK(moves.isEmpty());
        CHECK(list->property("contentY").toReal() == beforeContentY);
        wheel({ 0, -70 });
        check("screen.list.currentIndex === 2");
        CHECK(moves.count() == 1);
        wheel({}, { 0, 120 });
        check("screen.list.currentIndex === 1");

        moves.clear();
        wheel({ 120, 0 });
        wheel({ 0, -120 }, {}, false);
        run("screen.wheelOverlayVisible = true");
        wheel({ 0, -120 });
        check("screen.overlayWheelEvents === 1 && "
              "screen.list.currentIndex === 1");
        CHECK(moves.isEmpty());
        run("screen.wheelOverlayVisible = false");

        // Disabling input clears a partial notch rather than saving it for
        // later.
        wheel({ 0, -60 });
        run("screen.selection.inputEnabled = false");
        wheel({ 0, -120 });
        run("screen.selection.inputEnabled = true");
        wheel({ 0, -60 });
        CHECK(moves.isEmpty());
        wheel({ 0, -60 });
        check("screen.list.currentIndex === 2");
        moves.clear();
        run("screen.enabled = false");
        wheel({ 0, -120 });
        run("screen.enabled = true; screen.selection.enabled = false");
        wheel({ 0, -120 });
        CHECK(moves.isEmpty());
        check("screen.list.currentIndex === 2");
        run("screen.selection.enabled = true; screen.list.enabled = false");
        wheel({ 0, -120 });
        CHECK(moves.isEmpty());
        run("screen.list.enabled = true; screen.list.activate(2)");
        check("screen.list.count === 0 && screen.list.currentIndex === -1");
        wheel({ 0, -120 });
        check("screen.list.currentIndex === -1 && "
              "screen.selection.focusedItem === null");
    }

    SECTION("saved choices follow the active profile in both directions")
    {
        run("screen.selection.setKeymodeFilter(screen.k7); "
            "screen.selection.setSortMode(screen.levelSort); "
            "screen.selection.setReplayType(2)");
        CHECK(first->getVars()->getGeneralVars()->getSelectKeymodeFilter() ==
              resource_managers::SelectKeymodeFilter::K7);
        CHECK(first->getVars()->getGeneralVars()->getReplayType() == 2);
        CHECK(first->getVars()->getGeneralVars()->getSelectSortMode() ==
              resource_managers::SelectSortMode::Level);
        profiles.setMainProfile(second);
        settle();
        check("screen.selection.keymodeFilter === 0 && "
              "screen.selection.replayType === 0 && "
              "screen.selection.sortMode === screen.titleSort");
        check("screen.selection.setKeymodeFilter(screen.k5) && "
              "screen.selection.setReplayType(3)");
        CHECK(second->getVars()->getGeneralVars()->getSelectKeymodeFilter() ==
              resource_managers::SelectKeymodeFilter::K5);
        CHECK(second->getVars()->getGeneralVars()->getReplayType() == 3);
        profiles.setMainProfile(first);
        settle();
        check("screen.selection.keymodeFilter === screen.k7 && "
              "screen.selection.replayType === 2 && "
              "screen.selection.sortMode === screen.levelSort");
        first->getVars()->getGeneralVars()->setSelectKeymodeFilter(
          resource_managers::SelectKeymodeFilter::K5);
        first->getVars()->getGeneralVars()->setSelectSortMode(
          resource_managers::SelectSortMode::Title);
        first->getVars()->getGeneralVars()->setReplayType(1);
        settle();
        check("screen.selection.keymodeFilter === screen.k5 && "
              "screen.selection.sortMode === screen.titleSort && "
              "screen.selection.replayType === 1");
    }

    SECTION("assignments cannot detach a selector from saved choices")
    {
        for (const auto* property :
             { "keymodeFilter", "sortMode", "replayType" }) {
            const auto script =
              QString("'use strict'; screen.selection.%1 = 999").arg(property);
            const auto result = engine.evaluate(script);
            INFO(script.toStdString());
            CHECK(result.isError());
        }
        check("screen.selection.keymodeFilter === screen.allKeys && "
              "screen.selection.sortMode === screen.titleSort && "
              "screen.selection.replayType === 0");
        check("screen.selection.setKeymodeFilter(screen.k7) && "
              "screen.selection.setSortMode(screen.levelSort) && "
              "screen.selection.setReplayType(2)");
        check("screen.selection.keymodeFilter === screen.k7 && "
              "screen.selection.sortMode === screen.levelSort && "
              "screen.selection.replayType === 2");
    }

    SECTION("setters reject unknown values and inactive requests")
    {
        run("screen.selection.setKeymodeFilter(screen.k7); "
            "screen.selection.setSortMode(screen.levelSort); "
            "screen.selection.setReplayType(2)");
        for (const auto* value : { "999",
                                   "-1",
                                   "1.5",
                                   "'1'",
                                   "NaN",
                                   "Infinity",
                                   "null",
                                   "undefined" }) {
            for (const auto* method :
                 { "setKeymodeFilter", "setSortMode", "setReplayType" }) {
                check(QString("screen.selection.%1(%2) === false")
                        .arg(method, value));
            }
        }
        // A custom cycle list cannot make an unknown sort value valid.
        run("screen.selection.sortModes = [999]");
        check("screen.selection.setSortMode(999) === false");
        check("screen.selection.cycleSortMode() === false");
        check("screen.selection.setReplayType(4) === false");
        check("screen.selection.keymodeFilter === screen.k7 && "
              "screen.selection.sortMode === screen.levelSort && "
              "screen.selection.replayType === 2");
        CHECK(first->getVars()->getGeneralVars()->getSelectKeymodeFilter() ==
              resource_managers::SelectKeymodeFilter::K7);
        run("screen.selection.enabled = false");
        check("screen.selection.setKeymodeFilter(screen.allKeys) === false");
        check("screen.selection.setSortMode(screen.titleSort) === false");
        check("screen.selection.setReplayType(0) === false");
        check("screen.selection.cycleSortMode() === false");
        check("screen.selection.cycleReplayType() === false");
        CHECK(first->getVars()->getGeneralVars()->getReplayType() == 2);
        CHECK(first->getVars()->getGeneralVars()->getSelectSortMode() ==
              resource_managers::SelectSortMode::Level);
    }

    SECTION("cycling wraps choices and its order does not restrict setters")
    {
        run("screen.selection.sortModes = [screen.levelSort]");
        check("screen.selection.setSortMode(screen.titleSort)");
        check("screen.selection.cycleSortMode() && "
              "screen.selection.sortMode === screen.levelSort");
        run(
          "screen.selection.sortModes = [screen.levelSort, screen.titleSort]");
        check("screen.selection.cycleSortMode(-1) && "
              "screen.selection.sortMode === screen.titleSort");
        check("screen.selection.cycleSortMode() && "
              "screen.selection.sortMode === screen.levelSort");
        check("screen.selection.cycleSortMode(0.5) === false");
        check("screen.selection.cycleSortMode('1') === false");
        run("screen.selection.sortModes = []");
        check("screen.selection.cycleSortMode() === false");
        check("screen.selection.setSortMode(screen.titleSort)");
        check("screen.selection.setReplayType(3)");
        check("screen.selection.cycleReplayType() && "
              "screen.selection.replayType === 0");
        check("screen.selection.cycleReplayType() && "
              "screen.selection.replayType === 1");
    }
    INFO(warnings.join('\n').toStdString());
    CHECK(warnings.isEmpty());
}

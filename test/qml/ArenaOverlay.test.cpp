#include "support/CreateQmlPropertyMap.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>

#include <memory>

namespace {
void
settle()
{
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
}

void
storeRect(QQmlPropertyMap& settings,
          const QRectF& rect,
          const QString& prefix = "arenaOverlayK7")
{
    settings.insert(prefix + "XNormalized", rect.x());
    settings.insert(prefix + "YNormalized", rect.y());
    settings.insert(prefix + "WidthNormalized", rect.width());
    settings.insert(prefix + "HeightNormalized", rect.height());
}

QRectF
storedRect(const QQmlPropertyMap& settings,
           const QString& prefix = "arenaOverlayK7")
{
    return { settings.value(prefix + "XNormalized").toDouble(),
             settings.value(prefix + "YNormalized").toDouble(),
             settings.value(prefix + "WidthNormalized").toDouble(),
             settings.value(prefix + "HeightNormalized").toDouble() };
}

QRectF
geometry(const QQuickItem* item)
{
    return { item->x(), item->y(), item->width(), item->height() };
}

void
checkRect(const QRectF& actual, const QRectF& expected)
{
    CHECK(actual.x() == Catch::Approx(expected.x()));
    CHECK(actual.y() == Catch::Approx(expected.y()));
    CHECK(actual.width() == Catch::Approx(expected.width()));
    CHECK(actual.height() == Catch::Approx(expected.height()));
}

struct OverlayHarness
{
    std::unique_ptr<QQmlPropertyMap> settings{
        support::createQmlPropertyMap()
    };
    QStringList warnings;
    QQmlEngine engine;
    QQuickWindow window;
    std::unique_ptr<QObject> screen;
    QQuickItem* frame = nullptr;
    QObject* session = nullptr;

    void load(const QByteArray& overlay = R"(
        ArenaOverlayPlacementFrame {
            id: overlay
            objectName: "overlay"
            themeVars: screen.settings
            layoutVariant: "k7"
        }
    )")
    {
        QObject::connect(
          &engine, &QQmlEngine::warnings, [&](const QList<QQmlError>& errors) {
              for (const auto& error : errors)
                  warnings.append(error.toString());
          });
        QQmlComponent component(&engine);
        const auto source = QString::fromUtf8(RHYTHMGAME_SOURCE_DIR) +
                            "/share/RhythmGame/themes/Default/scripts/result/";
        component.setData(R"(
pragma ComponentBehavior: Bound
import QtQuick
import RhythmGameQml
import "." as DefaultResult
Item {
    id: screen
    width: 1920
    height: 1080
    required property var settings
    property QtObject session: QtObject {
        property bool chatOpen: false
        property int chatChanges: 0
        property string roomName: "Test room"
        property string selfMemberId: "self"
        property var arenaRunner: null
        property var presentedResult: null
        property var liveStandings: null
        property var chat: null
        property int unreadChatCount: 0
        function setChatOpen(open) { ++chatChanges; chatOpen = open; }
        function toggleChat() { setChatOpen(!chatOpen); }
    }
    )" + overlay + "\n}",
                          QUrl::fromLocalFile(source + "OverlayTest.qml"));
        INFO(component.errorString().toStdString());
        REQUIRE(component.isReady());
        screen.reset(component.createWithInitialProperties(
          { { "settings", QVariant::fromValue(settings.get()) } }));
        INFO(component.errorString().toStdString());
        REQUIRE(screen);
        auto* item = qobject_cast<QQuickItem*>(screen.get());
        REQUIRE(item);
        item->setParentItem(window.contentItem());
        window.resize(1920, 1080);
        frame = screen->findChild<QQuickItem*>("overlay");
        REQUIRE(frame);
        session = screen->property("session").value<QObject*>();
        REQUIRE(session);
        settle();
    }

    void checkWarnings() const
    {
        INFO(warnings.join('\n').toStdString());
        CHECK(warnings.isEmpty());
    }

    void action(const char* method, bool value)
    {
        REQUIRE(QMetaObject::invokeMethod(
          frame, method, Q_ARG(QVariant, QVariant(value))));
        settle();
    }

    void drag(QPoint from, QPoint delta)
    {
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, from);
        // Cross the drag threshold, then move with the handler's grab active.
        QTest::mouseMove(&window, from + delta / 2);
        QTest::mouseMove(&window, from + delta);
        QTest::mouseRelease(
          &window, Qt::LeftButton, Qt::NoModifier, from + delta);
        settle();
    }
};

const QByteArray standardOverlay = R"(
    StandardArenaOverlay {
        id: overlay
        objectName: "overlay"
        themeVars: screen.settings
        session: screen.session
        layoutVariant: "k7"
        active: true
        panelComponent: Item { readonly property Item dragHandle: this }
    }
)";
} // namespace

TEST_CASE("Arena placement follows settings without saving window changes",
          "[qml][ArenaOverlay]")
{
    OverlayHarness h;
    const QRectF original(0.2, 0.2, 0.3, 0.4);
    storeRect(*h.settings, original);
    int writes = 0;
    QObject::connect(
      h.settings.get(), &QQmlPropertyMap::valueChanged, [&] { ++writes; });
    h.load();
    checkRect(geometry(h.frame), { 384, 216, 576, 432 });

    for (const auto size : { QSize(900, 1400),
                             QSize(800, 450),
                             QSize(2560, 1080),
                             QSize(1920, 1080) }) {
        h.screen->setProperty("width", size.width());
        h.screen->setProperty("height", size.height());
        settle();
        checkRect(geometry(h.frame),
                  { original.x() * size.width(),
                    original.y() * size.height(),
                    original.width() * size.width(),
                    original.height() * size.height() });
        checkRect(storedRect(*h.settings), original);
    }
    CHECK(writes == 0);

    // C++ updates do not emit QQmlPropertyMap::valueChanged. Bindings must
    // observe the individual properties, including reset-to-default values.
    storeRect(*h.settings, { 0.3, 0.25, 0.25, 0.3 });
    h.settings->insert("arenaOverlayK7Expanded", true);
    settle();
    checkRect(geometry(h.frame), { 576, 270, 480, 324 });
    CHECK(h.frame->property("expanded").toBool());
    h.action("setExpanded", false);
    h.settings->insert("arenaOverlayK7Expanded", true);
    settle();
    CHECK(h.frame->property("expanded").toBool());
    CHECK(writes == 1);

    h.screen->setProperty("width", 200);
    h.screen->setProperty("height", 1200);
    settle();
    CHECK(h.frame->x() >= 0);
    CHECK(h.frame->x() + h.frame->width() <= 200);
    h.screen->setProperty("width", 1920);
    h.screen->setProperty("height", 1080);
    settle();
    checkRect(geometry(h.frame), { 576, 270, 480, 324 });
    h.checkWarnings();
}

TEST_CASE("Arena placement defaults and optional persistence stay reactive",
          "[qml][ArenaOverlay]")
{
    OverlayHarness h;
    storeRect(*h.settings, { -1, -1, -1, -1 });
    h.load();
    h.frame->setProperty("defaultPixelRectHint", QRectF(100, 150, 450, 350));
    settle();
    checkRect(geometry(h.frame), { 100, 150, 450, 350 });
    h.frame->setProperty("minimumPixelSize", QSizeF(500, 400));
    settle();
    checkRect(geometry(h.frame), { 100, 150, 500, 400 });
    checkRect(storedRect(*h.settings), { -1, -1, -1, -1 });

    h.screen->setProperty("settings", QVariant::fromValue<QObject*>(nullptr));
    h.action("setExpanded", true);
    h.action("setChatSelected", true);
    h.action("setOverlayVisible", false);
    CHECK(h.frame->property("expanded").toBool());
    CHECK(h.frame->property("chatSelected").toBool());
    CHECK_FALSE(h.frame->isVisible());
    h.action("setOverlayVisible", true);
    CHECK(h.frame->isVisible());
    h.frame->setProperty("layoutVariant", "result");
    h.frame->setProperty("defaultExpanded", true);
    settle();
    CHECK(h.frame->property("expanded").toBool());
    h.checkWarnings();
}

TEST_CASE(
  "Arena resize commits once the gesture ends and cancels on profile change",
  "[qml][ArenaOverlay]")
{
    OverlayHarness h;
    storeRect(*h.settings, { 0.2, 0.2, 0.3, 0.4 });
    h.load();
    auto* handle = h.frame->findChild<QObject*>("arenaResizeBottomRight");
    REQUIRE(handle);
    REQUIRE(QMetaObject::invokeMethod(
      handle, "interactionStarted", Q_ARG(int, 1), Q_ARG(int, 1)));
    REQUIRE(QMetaObject::invokeMethod(handle,
                                      "interactionDelta",
                                      Q_ARG(double, 100),
                                      Q_ARG(double, 80),
                                      Q_ARG(int, 1),
                                      Q_ARG(int, 1)));
    checkRect(geometry(h.frame), { 384, 216, 676, 512 });
    checkRect(storedRect(*h.settings), { 0.2, 0.2, 0.3, 0.4 });
    REQUIRE(QMetaObject::invokeMethod(handle, "interactionEnded"));
    checkRect(storedRect(*h.settings),
              { 0.2, 0.2, 676.0 / 1920, 512.0 / 1080 });

    REQUIRE(QMetaObject::invokeMethod(
      handle, "interactionStarted", Q_ARG(int, 1), Q_ARG(int, 1)));
    std::unique_ptr<QQmlPropertyMap> otherProfile{
        support::createQmlPropertyMap()
    };
    storeRect(*otherProfile, { 0.1, 0.1, 0.4, 0.4 });
    h.screen->setProperty("settings", QVariant::fromValue(otherProfile.get()));
    REQUIRE(QMetaObject::invokeMethod(handle,
                                      "interactionDelta",
                                      Q_ARG(double, 500),
                                      Q_ARG(double, 300),
                                      Q_ARG(int, 1),
                                      Q_ARG(int, 1)));
    REQUIRE(QMetaObject::invokeMethod(handle, "interactionEnded"));
    checkRect(storedRect(*otherProfile), { 0.1, 0.1, 0.4, 0.4 });
    checkRect(geometry(h.frame), { 192, 108, 768, 432 });
    h.screen->setProperty("settings", QVariant::fromValue(h.settings.get()));
    h.checkWarnings();
}

TEST_CASE("Arena repeated pointer drags preserve geometry bindings",
          "[qml][ArenaOverlay]")
{
    OverlayHarness h;
    storeRect(*h.settings, { 0.2, 0.2, 0.3, 0.4 });
    h.load();
    h.window.show();
    settle();
    for (int i = 0; i < 2; ++i) {
        const auto before = geometry(h.frame);
        h.drag(before.center().toPoint(), { 100, 60 });
        CHECK(h.frame->x() == Catch::Approx(before.x() + 100));
        CHECK(h.frame->y() == Catch::Approx(before.y() + 60));
        CHECK(storedRect(*h.settings).x() ==
              Catch::Approx(h.frame->x() / 1920));
    }
    for (const auto* name : { "arenaResizeTopLeft",
                              "arenaResizeTop",
                              "arenaResizeTopRight",
                              "arenaResizeRight",
                              "arenaResizeBottomRight",
                              "arenaResizeBottom",
                              "arenaResizeBottomLeft",
                              "arenaResizeLeft" }) {
        CAPTURE(name);
        auto* handle = h.frame->findChild<QQuickItem*>(name);
        REQUIRE(handle);
        for (int i = 0; i < 2; ++i) {
            const int horizontal = handle->property("horizontalEdge").toInt();
            const int vertical = handle->property("verticalEdge").toInt();
            const auto before = geometry(h.frame);
            const auto from =
              handle
                ->mapToScene(QPointF(handle->width() / 2, handle->height() / 2))
                .toPoint();
            h.drag(from, { horizontal * 30, vertical * 30 });
            CHECK(h.frame->width() ==
                  Catch::Approx(before.width() + (horizontal ? 30 : 0)));
            CHECK(h.frame->height() ==
                  Catch::Approx(before.height() + (vertical ? 30 : 0)));
        }
    }
    storeRect(*h.settings, { 0.2, 0.2, 0.3, 0.4 });
    settle();
    checkRect(geometry(h.frame), { 384, 216, 576, 432 });
    h.checkWarnings();
}

TEST_CASE("Only the presented Arena overlay restores chat preferences",
          "[qml][ArenaOverlay]")
{
    OverlayHarness h;
    h.settings->insert("arenaOverlayK7ChatSelected", true);
    h.load(standardOverlay);
    CHECK(h.session->property("chatOpen").toBool());
    // A server-side close should last until the screen is presented again.
    h.session->setProperty("chatOpen", false);
    settle();
    CHECK_FALSE(h.session->property("chatOpen").toBool());
    CHECK(h.settings->value("arenaOverlayK7ChatSelected").toBool());

    for (const auto* property : { "visible", "enabled" }) {
        h.screen->setProperty(property, false);
        settle();
        const int calls = h.session->property("chatChanges").toInt();
        h.settings->insert("arenaOverlayK7ChatSelected", false);
        h.settings->insert("arenaOverlayK7Visible", false);
        h.settings->insert("arenaOverlayK7ChatSelected", true);
        h.settings->insert("arenaOverlayK7Visible", true);
        settle();
        CHECK(h.session->property("chatChanges").toInt() == calls);
        h.screen->setProperty(property, true);
        settle();
        CHECK(h.session->property("chatOpen").toBool());
    }
    h.action("setOverlayVisible", false);
    CHECK_FALSE(h.session->property("chatOpen").toBool());
    h.frame->setProperty("customizeMode", true);
    settle();
    CHECK_FALSE(h.session->property("chatOpen").toBool());
    h.action("setOverlayVisible", true);
    CHECK(h.session->property("chatOpen").toBool());
    REQUIRE(QMetaObject::invokeMethod(h.frame, "toggleChat"));
    settle();
    CHECK_FALSE(h.session->property("chatOpen").toBool());
    CHECK_FALSE(h.settings->value("arenaOverlayK7ChatSelected").toBool());
    h.checkWarnings();
}

TEST_CASE("Arena expand buttons keep following saved preferences",
          "[qml][ArenaOverlay]")
{
    struct Panel
    {
        QByteArray type;
        const char* button;
        const char* setting;
    };
    for (const auto& panel : { Panel{ "StandardArenaGameplayOverlay { "
                                      "gameplay: null; layoutVariant: 'k7';",
                                      "arenaGameplayExpand",
                                      "arenaOverlayK7Expanded" },
                               Panel{ "StandardArenaResultOverlay {",
                                      "arenaResultExpand",
                                      "arenaOverlayResultExpanded" },
                               Panel{ R"(StandardArenaResultOverlay {
               panelComponent: DefaultResult.ArenaResultPanel {
                   result: null
                   session: overlay.session
                   localMemberId: "self"
                   statsFontFamily: "Arial"
                   textFontFamily: "Arial"
                   expanded: overlay.expanded
                   onExpandedChangeRequested: expanded => overlay.setExpanded(expanded)
               }
           )",
                                      "arenaNativeResultExpand",
                                      "arenaOverlayResultExpanded" } }) {
        CAPTURE(panel.button);
        OverlayHarness h;
        h.settings->insert(panel.setting, false);
        h.load(panel.type + R"(
            id: overlay
            objectName: "overlay"
            themeVars: screen.settings
            session: screen.session
            active: true
        })");
        auto* button = h.frame->findChild<QObject*>(panel.button);
        REQUIRE(button);
        REQUIRE(QMetaObject::invokeMethod(button, "clicked"));
        settle();
        CHECK(h.settings->value(panel.setting).toBool());
        h.settings->insert(panel.setting, false);
        settle();
        CHECK_FALSE(h.frame->property("expanded").toBool());
        REQUIRE(QMetaObject::invokeMethod(button, "clicked"));
        settle();
        CHECK(h.settings->value(panel.setting).toBool());
        h.checkWarnings();
    }
}

#include <catch2/catch_test_macros.hpp>

#include <QDir>
#include <QFile>
#include <QString>

#include <initializer_list>

namespace {

auto
qmlSource(const char* relativePath) -> QString
{
    const auto path = QDir(QStringLiteral(ARENA_QML_SOURCE_ROOT))
                        .filePath(QString::fromUtf8(relativePath));
    QFile file(path);
    INFO("QML source: " << path.toStdString());
    REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    return QString::fromUtf8(file.readAll());
}
void
requireContains(const QString& source,
                std::initializer_list<const char*> fragments)
{
    for (const auto* fragment : fragments) {
        const auto expected = QString::fromUtf8(fragment);
        INFO("Expected fragment: " << expected.toStdString());
        CHECK(source.contains(expected));
    }
}

auto
sectionFrom(const QString& source, const QString& marker, qsizetype length)
  -> QString
{
    const auto start = source.indexOf(marker);
    REQUIRE(start >= 0);
    return source.mid(start, length);
}

} // namespace

TEST_CASE("ArenaOverlayPolicy: overlay host follows only the current session runner",
          "[arena][ArenaOverlayPolicy]")
{
    const auto source = qmlSource("RhythmGameQml/Arena/ArenaOverlayHost.qml");
    requireContains(source,
                    { "required property var session",
                      "required property var currentItem",
                      "required property var themeVars",
                      "required property var generalVars",
                      "readonly property bool ownsArenaRunner",
                      "session.arenaGameplayActive === true",
                      "root.currentItem.chart === root.session.arenaRunner",
                      "ArenaOverlayPlacementFrame {",
                      "objectName: \"arenaGameplayPlacementFrame\"",
                      "themeVars: root.themeVars",
                      "viewport: root",
                      "customizeMode: root.customizeMode",
                      "sequence: \"F8\"",
                      "session.toggleGameplayChat()",
                      "placementKind: \"gameplayLeaderboard\"",
                      "property int unreadCount: 0",
                      "session.liveStandings.roundId",
                      "function onRowsInserted",
                      "root.unreadCount += last - first + 1" });

    CHECK_FALSE(source.contains(QStringLiteral("Settings {")));
    CHECK_FALSE(source.contains(QStringLiteral("property bool hidden")));
    CHECK_FALSE(source.contains(QStringLiteral("hideOverlay")));
    CHECK_FALSE(source.contains(
      QStringLiteral("required property string resolvedSkinId")));
    CHECK_FALSE(source.contains(
      QStringLiteral("resolvedSkinId: root.resolvedSkinId")));
}

TEST_CASE("ArenaOverlayPolicy: gameplay overlay is forced, bounded, and complete",
          "[arena][ArenaOverlayPolicy]")
{
    const auto source =
      qmlSource("RhythmGameQml/Arena/ArenaGameplayOverlay.qml");
    requireContains(source,
                    { "required property var session",
                      "property bool expanded: false",
                      "model: root.session.liveStandings",
                      "required property string memberId",
                      "required property string displayName",
                      "required property string competitionState",
                      "required property int rank",
                      "required property bool hasScore",
                      "required property var exScore",
                      "required property int maxCombo",
                      "required property int badPoorCount",
                      "required property int perfect",
                      "required property int great",
                      "required property int good",
                      "required property int bad",
                      "required property int poor",
                      "required property int emptyPoor",
                      "required property string gaugeType",
                      "required property int gaugeValueMilli",
                      "required property string clearType",
                      "required property int lobbyWinsAfter",
                      "required property string dnfReason",
                      "root.session.selfMemberId",
                      "root.session.opponentTarget.memberId",
                      "ArenaCompetitionText",
                      "return competitionText.stateText",
                      "competitionText.currentClearText",
                      "return competitionText.outcomeText",
                      "clip: true",
                      "ScrollBar.vertical",
                      "textFormat: Text.PlainText" });

    CHECK_FALSE(source.contains(
      QStringLiteral("required property string placementKind")));
    CHECK_FALSE(source.contains(
      QStringLiteral("required property string resolvedSkinId")));
    CHECK_FALSE(source.contains(
      QStringLiteral("required property string layoutVariant")));
    CHECK_FALSE(source.contains(QStringLiteral("text: standingDelegate.memberId")));
    CHECK_FALSE(source.contains(QStringLiteral("case \"result_unavailable\"")));
    CHECK_FALSE(source.contains(QStringLiteral("case \"aeasy\"")));

    CHECK_FALSE(source.contains(QStringLiteral("MouseArea")));
    CHECK_FALSE(source.contains(QStringLiteral("DragHandler")));
    CHECK_FALSE(source.contains(QStringLiteral("TapHandler")));
    CHECK_FALSE(source.contains(QStringLiteral("Settings {")));
}

TEST_CASE("ArenaOverlayPolicy: chat drawer geometry is adjacent, clipped, and transient",
          "[arena][ArenaOverlayPolicy]")
{
    const auto frame =
      qmlSource("RhythmGameQml/Arena/ArenaOverlayPlacementFrame.qml");
    requireContains(frame,
                    { "function adjacentChatRect",
                      "function largestAdjacentRect",
                      "const targetWidth = Math.min(420, Math.max(320",
                      "const targetHeight = Math.min(360",
                      "safePixelRect()" });

    const auto host = qmlSource("RhythmGameQml/Arena/ArenaOverlayHost.qml");
    requireContains(host,
                    { "objectName: \"arenaGameplayChatDrawer\"",
                      "active: root.arenaShortcutEnabled",
                      "visible: root.session.gameplayChatOpen === true",
                      "placementFrame.adjacentChatRect()",
                      "root.unreadCount = 0" });
    CHECK_FALSE(host.contains(QStringLiteral("Settings {")));
    CHECK_FALSE(host.contains(QStringLiteral("chatDrawerXNormalized")));
    CHECK_FALSE(host.contains(QStringLiteral("generalVars.unread"),
                              Qt::CaseInsensitive));
}

TEST_CASE("ArenaOverlayPolicy: first gameplay hint uses only the profile-wide version",
          "[arena][ArenaOverlayPolicy]")
{
    const auto source = qmlSource("RhythmGameQml/Arena/ArenaOverlayHost.qml");
    requireContains(source,
                    { "generalVars.arenaOverlayHintVersion < 1",
                      "generalVars.arenaOverlayHintVersion = 1",
                      "Press F2 to move Arena standings",
                      "interval: 6000",
                      "objectName: \"arenaOverlayPlacementHint\"" });
    CHECK_FALSE(source.contains(QStringLiteral("hintVersion.json")));
}

TEST_CASE("ArenaOverlayPolicy: gameplay chat stays plain text and owns keyboard submission",
          "[arena][ArenaOverlayPolicy]")
{
    const auto source = qmlSource("RhythmGameQml/Arena/ArenaGameplayChat.qml");
    requireContains(source,
                    { "required property var session",
                      "model: root.session.chat",
                      "root.session.sendChat(message)",
                      "root.session.setGameplayChatOpen(false)",
                      "event.key === Qt.Key_Escape",
                      "Qt.Key_Return",
                      "Qt.Key_Enter",
                      "Qt.ShiftModifier",
                      "textFormat: Text.PlainText" });
    CHECK(source.count(QStringLiteral("textFormat: Text.PlainText")) >= 2);
    CHECK_FALSE(source.contains(QStringLiteral("pause("), Qt::CaseInsensitive));
}

TEST_CASE("ArenaOverlayPolicy: gameplay Escape closes chat before one abandon command",
          "[arena][ArenaOverlayPolicy]")
{
    const auto defaultSource = qmlSource(
      "share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml");
    requireContains(defaultSource,
                    { "readonly property var arenaSession: Rg.arenaSession",
                      "readonly property bool arenaGameplayOwned",
                      "root.arenaSession.gameplayChatOpen",
                      "root.arenaSession.setGameplayChatOpen(false)",
                      "root.arenaSession.abandonCurrentRound()" });
    const auto defaultEscape = sectionFrom(
      defaultSource, QStringLiteral("id: escapeShortcut"), 1800);
    const auto defaultClose =
      defaultEscape.indexOf(QStringLiteral("setGameplayChatOpen(false)"));
    const auto defaultAbandon =
      defaultEscape.indexOf(QStringLiteral("abandonCurrentRound()"));
    const auto defaultExisting =
      defaultEscape.indexOf(QStringLiteral("if (nothingWasHit)"));
    REQUIRE(defaultClose >= 0);
    REQUIRE(defaultAbandon >= 0);
    REQUIRE(defaultExisting >= 0);
    CHECK(defaultClose < defaultAbandon);
    CHECK(defaultAbandon < defaultExisting);
    CHECK(defaultEscape.count(QStringLiteral("abandonCurrentRound()")) == 1);

    const auto legacySource =
      qmlSource("RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml");
    requireContains(legacySource,
                    { "readonly property var arenaSession: Rg.arenaSession",
                      "readonly property bool arenaGameplayOwned",
                      "root.arenaSession.gameplayChatOpen",
                      "root.arenaSession.setGameplayChatOpen(false)",
                      "root.arenaSession.abandonCurrentRound()" });
    const auto legacyEscape = sectionFrom(
      legacySource, QStringLiteral("function handleGameplayEscape()"), 1800);
    const auto legacyClose =
      legacyEscape.indexOf(QStringLiteral("setGameplayChatOpen(false)"));
    const auto legacyAbandon =
      legacyEscape.indexOf(QStringLiteral("abandonCurrentRound()"));
    const auto legacyExisting =
      legacyEscape.indexOf(QStringLiteral("if (root.gameplayNothingWasHit)"));
    REQUIRE(legacyClose >= 0);
    REQUIRE(legacyAbandon >= 0);
    REQUIRE(legacyExisting >= 0);
    CHECK(legacyClose < legacyAbandon);
    CHECK(legacyAbandon < legacyExisting);
    CHECK(legacyEscape.count(QStringLiteral("abandonCurrentRound()")) == 1);
}

TEST_CASE("ArenaOverlayPolicy: browser makes pre-competition connections browse-only",
          "[arena][ArenaOverlayPolicy]")
{
    const auto browser = qmlSource("RhythmGameQml/Arena/ArenaBrowser.qml");
    requireContains(browser,
                    { "required property var session",
                      "session.directoryReady",
                      "!session.competitionAvailable",
                      "&& !updateRequired",
                      "active: !root.updateRequired",
                      "admissionAllowed: !root.updateRequired",
                      "Update RhythmGame to create or join Arena rooms." });

    const auto login = qmlSource("RhythmGameQml/Arena/ArenaLoginPanel.qml");
    requireContains(login,
                    { "required property bool admissionAllowed",
                      "enabled: root.admissionAllowed",
                      "if (!root.admissionAllowed" });
}

TEST_CASE("ArenaOverlayPolicy: ContentFrame hosts Arena above the active gameplay skin",
          "[arena][ArenaOverlayPolicy]")
{
    const auto source = qmlSource("RhythmGameQml/ContentFrame.qml");
    requireContains(source,
                    { "readonly property var arenaSession: Rg.arenaSession",
                      "function gameplayLayoutVariant",
                      "function gameplayThemeVars",
                      "mainProfile.themeConfig[layoutVariant]",
                      "ArenaOverlayHost {",
                      "session: globalRoot.arenaSession",
                      "currentItem: sceneStack.currentItem",
                      "themeVars: globalRoot.gameplayThemeVars(layoutVariant)",
                      "generalVars: globalRoot.mainProfile.vars.generalVars",
                      "layoutVariant:" });
    CHECK(source.indexOf(QStringLiteral("id: sceneStack")) <
          source.indexOf(QStringLiteral("ArenaOverlayHost {")));
}

TEST_CASE("ArenaOverlayPolicy: Default select preserves its composition and "
          "uses the authored Arena gap",
          "[arena][ArenaOverlayPolicy][accessibility]")
{
    const auto source =
      qmlSource("share/RhythmGame/themes/Default/scripts/select/Select.qml");

    const auto stageFileStart =
      source.indexOf(QStringLiteral("StageFile {"));
    const auto stageFileEnd =
      source.indexOf(QStringLiteral("Loader {"), stageFileStart);
    REQUIRE(stageFileStart >= 0);
    REQUIRE(stageFileEnd > stageFileStart);
    const auto stageFile =
      source.mid(stageFileStart, stageFileEnd - stageFileStart);
    requireContains(stageFile,
                    { "fillMode: Image.Stretch",
                      "height: 480",
                      "width: 640" });
    CHECK_FALSE(stageFile.contains(QStringLiteral("arenaSeated")));
    CHECK_FALSE(
      stageFile.contains(QStringLiteral("Image.PreserveAspectCrop")));

    const auto loaderStart =
      source.indexOf(QStringLiteral("id: arenaPanelLoader"));
    const auto loaderEnd =
      source.indexOf(QStringLiteral("Banner {"), loaderStart);
    REQUIRE(loaderStart >= 0);
    REQUIRE(loaderEnd > loaderStart);
    const auto loader = source.mid(loaderStart, loaderEnd - loaderStart);
    requireContains(loader,
                    { "objectName: \"arenaNativeSelectPanelLoader\"",
                      "active: root.arenaSeated",
                      "parent: root",
                      "anchors.fill: parent",
                      "enabled: !options.visible",
                      "sourceComponent: ArenaSelectOverlay {",
                      "session: Rg.arenaSession",
                      "themeVars: root.themeVars",
                      "viewport: root",
                      "defaultPixelRectHint: Qt.rect(root.contentLeft + 728 * root.contentScale,",
                      "root.contentTop + 120 * root.contentScale,",
                      "520 * root.contentScale,",
                      "480 * root.contentScale)",
                      "z: options.visible ? 0 : 3" });
    CHECK_FALSE(loader.contains(QStringLiteral("x: Math.max")));
    CHECK_FALSE(loader.contains(QStringLiteral("y: Math.max")));
    CHECK_FALSE(loader.contains(QStringLiteral("width: Math.min")));
    CHECK_FALSE(loader.contains(QStringLiteral("height: Math.min")));

    const auto frameStart =
      source.indexOf(QStringLiteral("id: stageFileFrame"));
    const auto frameEnd =
      source.indexOf(QStringLiteral("List {"), frameStart);
    REQUIRE(frameStart >= 0);
    REQUIRE(frameEnd > frameStart);
    const auto stageFileFrame =
      source.mid(frameStart, frameEnd - frameStart);
    requireContains(stageFileFrame,
                    { "source: root.imagesUrl + \"stageFileFrame.png\"" });
    CHECK_FALSE(stageFileFrame.contains(QStringLiteral("arenaSeated")));
    CHECK_FALSE(stageFileFrame.contains(QStringLiteral("visible:")));

    const auto overlay =
      qmlSource("RhythmGameQml/Arena/ArenaSelectOverlay.qml");
    requireContains(overlay,
                    { "property rect defaultPixelRectHint: Qt.rect(0, 0, 0, 0)",
                      "defaultPixelRectHint: root.defaultPixelRectHint" });
}

TEST_CASE("ArenaOverlayPolicy: browser and room lists expose keyboard and assistive semantics",
          "[arena][ArenaOverlayPolicy][accessibility]")
{
    const auto browser = qmlSource("RhythmGameQml/Arena/ArenaBrowser.qml");
    requireContains(browser,
                    { "objectName: \"arenaRoomList\"",
                      "Accessible.name: qsTr(\"Arena rooms\")",
                      "Accessible.role: Accessible.List",
                      "objectName: \"arenaRoom-\" + roomDelegate.roomId",
                      "Accessible.role: Accessible.ListItem",
                      "keyNavigationEnabled: true",
                      "roomList.currentItem.activate()",
                      "activeFocusOnTab: true",
                      "border.width: ListView.isCurrentItem && ListView.view.activeFocus ? 2 : 0" });

    const auto room = qmlSource("RhythmGameQml/Arena/ArenaRoom.qml");
    requireContains(room,
                    { "objectName: \"arenaRoomMemberList\"",
                      "Accessible.name: qsTr(\"Arena players\")",
                      "Accessible.role: Accessible.List",
                      "objectName: \"arenaRoomMember-\" + memberDelegate.memberId",
                      "Accessible.role: Accessible.ListItem",
                      "activeFocusOnTab: true",
                      "Accessible.description: root.moderationDisabledReason",
                      "objectName: \"arenaRoomChatList\"",
                      "Accessible.name: qsTr(\"Arena chat\")",
                      "objectName: \"arenaRoomChat-\" + chatDelegate.messageId",
                      "border.width: ListView.isCurrentItem && ListView.view.activeFocus ? 2 : 0" });

    const auto login = qmlSource("RhythmGameQml/Arena/ArenaLoginPanel.qml");
    requireContains(login,
                    { "Accessible.name: qsTr(\"Arena login\")",
                      "Accessible.role: Accessible.Grouping",
                      "Accessible.role: Accessible.AlertMessage" });
}

TEST_CASE("ArenaOverlayPolicy: room surfaces announce only bounded competition status",
          "[arena][ArenaOverlayPolicy][accessibility]")
{
    for (const auto* path : {
           "RhythmGameQml/Arena/ArenaBrowser.qml",
           "RhythmGameQml/Arena/ArenaRoom.qml",
           "RhythmGameQml/Arena/ArenaSelectStrip.qml",
           "RhythmGameQml/Arena/ArenaSelectPanel.qml" }) {
        const auto source = qmlSource(path);
        requireContains(source,
                        { "readonly property alias lastAnnouncementKey",
                          "readonly property alias lastAnnouncementText",
                          "readonly property alias announcementCount",
                          "ArenaStatusAnnouncer {",
                          "roundLaunchCancellationStatusKey:" });
    }

    const auto announcer =
      qmlSource("RhythmGameQml/Arena/ArenaStatusAnnouncer.qml");
    requireContains(announcer,
                    { "property string lastAnnouncementKey",
                      "property string lastAnnouncementText",
                      "property int announcementCount",
                      "required property string roundLaunchCancellationStatusKey",
                      "arena.status.reconnecting",
                      "arena.status.selectionInvalidated",
                      "arena.status.roundLaunchCancelled.missingFile",
                      "arena.status.roundLaunchCancelled.hashMismatch",
                      "arena.status.roundLaunchCancelled.readFailed",
                      "arena.status.roundLaunchCancelled.parseFailed",
                      "arena.status.roundLaunchCancelled.unsupportedConfig",
                      "arena.status.roundLaunchCancelled.resourceFailed",
                      "arena.status.roundLaunchCancelled.probeTimeout",
                      "arena.status.roundLaunchCancelled.loadTimeout",
                      "arena.status.roundLaunchCancelled.participantLeft",
                      "arena.status.roundLaunchCancelled.participantKicked",
                      "arena.status.roundLaunchCancelled.chartLengthMismatch",
                      "arena.status.roundLaunchCancelled.serverShutdown",
                      "arena.status.roundLaunchCancelled.cancelled",
                      "root.target.Accessible.announce(text)" });
}

TEST_CASE("ArenaOverlayPolicy: select confirm keys share chart activation and Escape owns room exit",
          "[arena][ArenaOverlayPolicy][select-input]")
{
    const auto defaultList = qmlSource(
      "share/RhythmGame/themes/Default/scripts/select/List.qml");
    requireContains(defaultList,
                    { "Keys.onReturnPressed", "Keys.onEnterPressed",
                      "goForward(current)" });

    const auto legacy =
      qmlSource("RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml");
    requireContains(legacy,
                    { "function handleConfirmKey", "Keys.onReturnPressed",
                      "Keys.onEnterPressed", "root.handleConfirmKey(event)",
                      "root.selectGoForward()",
                      "sequence: \"Esc\"", "Rg.arenaSession.leaveRoom()" });

    const auto defaultSelect = qmlSource(
      "share/RhythmGame/themes/Default/scripts/select/Select.qml");
    requireContains(defaultSelect,
                    { "sequence: \"Esc\"", "Rg.arenaSession.leaveRoom()" });

    const auto panel =
      qmlSource("RhythmGameQml/Arena/ArenaSelectPanel.qml");
    CHECK_FALSE(panel.contains(QStringLiteral("arenaSelectLeave")));
}

pragma ComponentBehavior: Bound

import QtQuick
import QtTest
import RhythmGameQml

TestCase {
    id: testCase

    name: "ArenaLegacySelect"
    when: windowShown
    width: 1200
    height: 800
    visible: true

    Component {
        id: fakeSessionComponent

        FakeArenaSession {}
    }

    Component {
        id: themeVarsComponent

        FakeArenaThemeVars {}
    }

    Component {
        id: hostComponent

        Item {
            id: harness

            property int backgroundTaps: 0
            property alias overlay: overlay
            property alias presentedScreen: presentedScreen
            property alias shell: shell
            property alias unknownScreen: unknownScreen
            property var session
            property var themeVars

            Item {
                anchors.fill: parent

                TapHandler {
                    onTapped: harness.backgroundTaps += 1
                }
            }

            Item {
                id: presentedScreen

                property bool arenaNativeSelectPresentation: false
            }

            Item {
                id: unknownScreen
            }

            Item {
                id: shell

                property bool arenaSelectPresentationActive: true
                property var arenaSelectPresentationItem: presentedScreen
            }

            ArenaLegacySelectOverlay {
                id: overlay

                presentationItem: shell.arenaSelectPresentationItem
                session: harness.session
                themeVars: harness.themeVars
                viewport: harness
            }
        }
    }

    Component {
        id: loaderHostComponent

        Item {
            id: loaderHarness

            property int backgroundTaps: 0
            property alias overlayLoader: overlayLoader
            property var session
            property var themeVars

            Item {
                anchors.fill: parent

                TapHandler {
                    onTapped: loaderHarness.backgroundTaps += 1
                }
            }

            Item {
                id: loaderPresentedScreen

                property bool arenaNativeSelectPresentation: false
            }

            Loader {
                id: overlayLoader

                anchors.fill: parent
                sourceComponent: ArenaLegacySelectOverlay {
                    presentationItem: loaderPresentedScreen
                    session: loaderHarness.session
                    themeVars: loaderHarness.themeVars
                    viewport: loaderHarness
                }
            }
        }
    }

    function createSession() {
        const session = createTemporaryObject(fakeSessionComponent, testCase);
        verify(session !== null);
        for (let index = 0; index < 16; ++index) {
            session.members.append({
                "memberId": "member-" + (index + 1),
                "displayName": "Player " + (index + 1),
                "avatarUrl": "",
                "connected": index < 12,
                "owner": index === 0,
                "self": index === 0,
                "lobbyWins": index,
                "ready": index % 2 === 0,
                "inventoryState": "ready",
                "inventoryRevision": 1,
                "availabilityAppliedRevision": 1,
                "roundState": index === 3 ? "loading" : "eligible"
            });
        }
        session.selectedTitle = "Selected chart";
        session.lastResult.valid = true;
        session.lastResult.finalized = true;
        session.lastResult.winnerMemberIds = ["member-2"];
        session.lastResult.winnerNames = ["Player 2"];
        return session;
    }

    function createHarness(width = 1200, height = 800) {
        const session = createSession();
        const themeVars = createTemporaryObject(themeVarsComponent, testCase);
        verify(themeVars !== null);
        const harness = createTemporaryObject(hostComponent, testCase, {
            "height": height,
            "session": session,
            "themeVars": themeVars,
            "width": width
        });
        verify(harness !== null);
        session.parent = harness;
        wait(1);
        return {
            "harness": harness,
            "session": session,
            "themeVars": themeVars
        };
    }

    function createLoaderHarness(width = 1280, height = 720) {
        const session = createSession();
        const themeVars = createTemporaryObject(themeVarsComponent, testCase);
        verify(themeVars !== null);
        const harness = createTemporaryObject(loaderHostComponent, testCase, {
            "height": height,
            "session": session,
            "themeVars": themeVars,
            "width": width
        });
        verify(harness !== null);
        session.parent = harness;
        tryCompare(harness.overlayLoader, "status", Loader.Ready);
        verify(harness.overlayLoader.item !== null);
        return {
            "harness": harness,
            "overlay": harness.overlayLoader.item,
            "session": session,
            "themeVars": themeVars
        };
    }

    function isDescendantOf(item, ancestor) {
        let current = item;
        while (current !== null) {
            if (current === ancestor)
                return true;
            current = current.parent;
        }
        return false;
    }

    function hasFullscreenPaintedDirectChild(item) {
        for (const child of item.children) {
            if ("color" in child && child.visible
                    && child.width >= item.width - 0.01
                    && child.height >= item.height - 0.01) {
                return true;
            }
        }
        return false;
    }

    function selectPanelFor(item) {
        let current = item;
        while (current !== null) {
            if (current.arenaNativeSelectPresentation !== undefined
                    && current.arenaNativeSelectPresentation === true) {
                return current;
            }
            current = current.parent;
        }
        return null;
    }

    function test_capability_alone_routes_the_fallback() {
        const state = createHarness();
        let overlay = state.harness.overlay;
        verify(overlay !== null);

        state.harness.presentedScreen.arenaNativeSelectPresentation = true;
        tryCompare(overlay, "visible", false);
        state.harness.presentedScreen.arenaNativeSelectPresentation = false;
        tryCompare(overlay, "visible", true);

        state.harness.shell.arenaSelectPresentationItem = state.harness.unknownScreen;
        compare(overlay.visible, true);
        state.harness.shell.arenaSelectPresentationItem = state.harness.presentedScreen;

        state.harness.presentedScreen.objectName = "beatoraja";
        compare(overlay.visible, true);
        state.harness.presentedScreen.objectName = "lr2";
        compare(overlay.visible, true);
    }

    function test_shared_panel_is_always_expanded_and_exclusive() {
        const state = createHarness(1280, 720);
        const overlay = state.harness.overlay;
        verify(overlay !== null);
        const details = findChild(overlay, "arenaSelectDetailsTab");
        const chat = findChild(overlay, "arenaSelectChatTab");
        const roster = findChild(overlay, "arenaSelectRoster");
        const selection = findChild(overlay, "arenaSelectSelection");
        const ready = findChild(overlay, "arenaSelectReady");
        verify(details !== null);
        verify(chat !== null);
        verify(roster !== null);
        verify(selection !== null);
        verify(ready !== null);
        compare(findChild(overlay, "arenaSelectLeave"), null);
        const panel = selectPanelFor(details);
        verify(panel !== null);
        compare(findChild(overlay, "arenaLegacyExpand"), null);
        tryCompare(roster, "memberCount", 16);
        verify(roster.contentHeight > roster.height);

        compare(details.checked, true);
        compare(chat.checked, false);
        compare(panel.detailMode, "details");
        verify(findChild(overlay, "arenaSelectSelection") !== null);
        compare(findChild(overlay, "arenaSelectChat"), null);
        mouseClick(details, details.width / 2, details.height / 2,
                   Qt.LeftButton);
        compare(details.checked, true);
        compare(chat.checked, false);
        compare(panel.detailMode, "details");
        verify(findChild(overlay, "arenaSelectSelection") !== null);
        compare(findChild(overlay, "arenaSelectChat"), null);

        details.forceActiveFocus();
        keyClick(Qt.Key_Space);
        compare(details.checked, true);
        compare(chat.checked, false);
        compare(panel.detailMode, "details");
        verify(findChild(overlay, "arenaSelectSelection") !== null);
        compare(findChild(overlay, "arenaSelectChat"), null);

        mouseClick(chat, chat.width / 2, chat.height / 2, Qt.LeftButton);
        compare(details.checked, false);
        compare(chat.checked, true);
        compare(panel.detailMode, "chat");
        tryVerify(function() {
            return findChild(overlay, "arenaSelectChat") !== null
                    && findChild(overlay, "arenaSelectSelection") === null;
        });

        mouseClick(chat, chat.width / 2, chat.height / 2, Qt.LeftButton);
        compare(details.checked, false);
        compare(chat.checked, true);
        compare(panel.detailMode, "chat");
        verify(findChild(overlay, "arenaSelectChat") !== null);
        compare(findChild(overlay, "arenaSelectSelection"), null);

        chat.forceActiveFocus();
        keyClick(Qt.Key_Space);
        compare(details.checked, false);
        compare(chat.checked, true);
        compare(panel.detailMode, "chat");
        verify(findChild(overlay, "arenaSelectChat") !== null);
        compare(findChild(overlay, "arenaSelectSelection"), null);
    }

    function test_shared_actions_route_to_the_existing_session() {
        const state = createHarness();
        const overlay = state.harness.overlay;

        const ready = findChild(overlay, "arenaSelectReady");
        mouseClick(ready, ready.width / 2, ready.height / 2, Qt.LeftButton);
        compare(state.session.readyRequests, [true]);

        tryVerify(function() {
            return findChild(overlay, "arenaRosterKick-member-2") !== null;
        });
        const kick = findChild(overlay, "arenaRosterKick-member-2");
        mouseClick(kick, kick.width / 2, kick.height / 2, Qt.LeftButton);
        compare(state.session.kickedMemberIds, ["member-2"]);

        const chatTab = findChild(overlay, "arenaSelectChatTab");
        mouseClick(chatTab, chatTab.width / 2, chatTab.height / 2, Qt.LeftButton);
        const chatView = findChild(overlay, "arenaSelectChat");
        verify(chatView !== null);
        const chatInput = findChild(chatView, "arenaChatInput");
        verify(chatInput !== null);
        chatInput.forceActiveFocus();
        chatInput.text = "legacy hello";
        keyClick(Qt.Key_Return);
        compare(state.session.sentMessages, ["legacy hello"]);

        compare(findChild(overlay, "arenaSelectLeave"), null);
    }

    function test_ready_disabled_reason_stays_accessible_on_chat_tab() {
        const state = createHarness();
        const overlay = state.harness.overlay;
        state.session.canReady = false;

        const ready = findChild(overlay, "arenaSelectReady");
        const reason = findChild(overlay, "arenaSelectReadyDisabledReason");
        verify(ready !== null);
        verify(reason !== null);
        compare(ready.enabled, false);
        verify(reason.text.length > 0);
        compare(ready.Accessible.description, reason.text);

        const chatTab = findChild(overlay, "arenaSelectChatTab");
        mouseClick(chatTab, chatTab.width / 2, chatTab.height / 2, Qt.LeftButton);
        compare(ready.Accessible.description, reason.text);
    }

    function test_fullscreen_loader_keeps_transparent_root_and_bounded_panel() {
        const state = createLoaderHarness(1280, 720);
        const overlay = state.overlay;
        compare(overlay.x, 0);
        compare(overlay.y, 0);
        compare(overlay.width, 1280);
        compare(overlay.height, 720);
        compare(hasFullscreenPaintedDirectChild(overlay), false);

        const frame = findChild(overlay, "arenaSelectPlacementFrame");
        verify(frame !== null);
        verify(frame.width < overlay.width);
        verify(frame.height < overlay.height);
        verify(frame.x >= 24);
        verify(frame.y >= 24);
        verify(frame.x + frame.width <= overlay.width - 24);
        verify(frame.y + frame.height <= overlay.height - 24);
        const details = findChild(overlay, "arenaSelectDetailsTab");
        verify(details !== null);
        verify(isDescendantOf(details, frame));
        compare(findChild(overlay, "arenaLegacyExpand"), null);
    }

    function test_outside_the_panel_remains_pointer_transparent() {
        const state = createHarness();
        const overlay = state.harness.overlay;
        const frame = findChild(overlay, "arenaSelectPlacementFrame");
        verify(frame !== null);
        verify(frame.x >= 24);
        mouseClick(state.harness, 4, 4, Qt.LeftButton);
        compare(state.harness.backgroundTaps, 1);
    }
}

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
        id: hostComponent

        Item {
            id: harness

            property int backgroundTaps: 0
            property alias overlay: overlay
            property alias presentedScreen: presentedScreen
            property alias shell: shell
            property alias unknownScreen: unknownScreen
            property var session

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
                viewport: harness
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
        const harness = createTemporaryObject(hostComponent, testCase, {
            "height": height,
            "session": session,
            "width": width
        });
        verify(harness !== null);
        wait(1);
        return {
            "harness": harness,
            "session": session
        };
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

    function test_compact_header_is_bounded_and_expandable() {
        const state = createHarness(1280, 720);
        const overlay = state.harness.overlay;
        verify(overlay !== null);
        compare(overlay.expanded, false);
        compare(overlay.width, 420);
        compare(overlay.x, 1280 - 24 - 420);
        compare(overlay.y, 24);
        verify(overlay.height <= 720 - 48);

        const strip = findChild(overlay, "arenaLegacyCompactHeader");
        verify(strip !== null);
        compare(strip.connectedCount, 12);
        compare(strip.reservedCount, 4);
        const room = findChild(strip, "arenaStripRoom");
        const selection = findChild(strip, "arenaStripSelection");
        const stateLabel = findChild(strip, "arenaStripReadyState");
        compare(room.text, "Test room");
        compare(room.textFormat, Text.PlainText);
        verify(selection.text.indexOf("Selected chart") >= 0);
        verify(stateLabel.text.indexOf("Not ready") >= 0);
        const expand = findChild(overlay, "arenaLegacyExpand");
        mouseClick(expand, expand.width / 2, expand.height / 2, Qt.LeftButton);
        compare(overlay.expanded, true);
        verify(overlay.height <= 720 - 48);
        const roster = findChild(overlay, "arenaLegacyRoster");
        tryCompare(roster, "memberCount", 16);
        verify(roster.contentHeight > roster.height);

        mouseClick(expand, expand.width / 2, expand.height / 2, Qt.LeftButton);
        compare(overlay.expanded, false);
    }

    function test_expanded_actions_route_to_the_existing_session() {
        const state = createHarness();
        const overlay = state.harness.overlay;
        overlay.expanded = true;
        wait(1);

        const ready = findChild(overlay, "arenaLegacyReady");
        mouseClick(ready, ready.width / 2, ready.height / 2, Qt.LeftButton);
        compare(state.session.readyRequests, [true]);

        tryVerify(function() {
            return findChild(overlay, "arenaRosterKick-member-2") !== null;
        });
        const kick = findChild(overlay, "arenaRosterKick-member-2");
        mouseClick(kick, kick.width / 2, kick.height / 2, Qt.LeftButton);
        compare(state.session.kickedMemberIds, ["member-2"]);

        const chatTab = findChild(overlay, "arenaLegacyChatTab");
        mouseClick(chatTab, chatTab.width / 2, chatTab.height / 2, Qt.LeftButton);
        wait(1);
        const chatInput = findChild(overlay, "arenaChatInput");
        chatInput.forceActiveFocus();
        chatInput.text = "legacy hello";
        keyClick(Qt.Key_Return);
        compare(state.session.sentMessages, ["legacy hello"]);

        const winners = findChild(overlay, "arenaLastWinners");
        verify(winners.text.indexOf("Player 2") >= 0);

        const leave = findChild(overlay, "arenaLegacyLeave");
        mouseClick(leave, leave.width / 2, leave.height / 2, Qt.LeftButton);
        compare(state.session.leaveCount, 1);
    }

    function test_outside_the_panel_remains_pointer_transparent() {
        const state = createHarness();
        const overlay = state.harness.overlay;
        verify(overlay.x > 24);
        mouseClick(state.harness, 12, 12, Qt.LeftButton);
        compare(state.harness.backgroundTaps, 1);
    }
}

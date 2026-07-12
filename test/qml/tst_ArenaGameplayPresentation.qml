import QtQuick
import QtTest
import RhythmGameQml

TestCase {
    id: testCase

    name: "ArenaGameplayPresentation"
    when: windowShown
    width: 1280
    height: 720
    visible: true

    Component {
        id: runnerComponent

        QtObject {}
    }

    Component {
        id: generalVarsComponent

        QtObject {
            property int arenaOverlayHintVersion: 0
        }
    }

    Component {
        id: themeVarsComponent

        FakeArenaThemeVars {}
    }

    Component {
        id: viewportComponent

        Item {}
    }

    Component {
        id: sessionComponent

        Item {
            id: session

            property bool arenaGameplayActive: true
            property var arenaRunner: null
            property bool resultPresentationActive: false
            property bool gameplayChatOpen: false
            property bool overlayCustomizationActive: false
            property string selfMemberId: "member-local"
            property string arenaOptionsSummary: "MIRROR · FLIP"
            property alias liveStandings: standingsModel
            property alias chat: chatModel
            property alias opponentTarget: target
            property var sentMessages: []
            property int toggleCount: 0

            function sendChat(message) {
                sentMessages = sentMessages.concat([message]);
            }

            function setGameplayChatOpen(open) {
                gameplayChatOpen = !!open && arenaGameplayActive;
            }

            function toggleGameplayChat() {
                toggleCount += 1;
                setGameplayChatOpen(!gameplayChatOpen);
            }

            function setOverlayCustomizationActive(active) {
                overlayCustomizationActive = !!active && arenaGameplayActive && arenaRunner !== null;
            }

            ListModel {
                id: standingsModel

                property string roundId: "round-1"
            }

            ListModel {
                id: chatModel
            }

            QtObject {
                id: target

                property bool available: true
                property string memberId: "member-target"
                property string displayName: "Target"
                property int exScore: 300
                property bool finished: false
            }
        }
    }

    Component {
        id: gameplayScreenComponent

        Item {
            id: screen

            required property var session
            property var chart: null
            property bool arenaCustomizeMode: false
            property int abortCount: 0
            property int controllerCount: 0

            function setArenaCustomizeMode(active) {
                arenaCustomizeMode = !!active;
            }

            function controllerPulse() {
                controllerCount += 1;
            }

            Shortcut {
                context: Qt.ApplicationShortcut
                enabled: !screen.session.overlayCustomizationActive
                sequence: "Esc"

                onActivated: {
                    if (screen.session.gameplayChatOpen) {
                        screen.session.setGameplayChatOpen(false);
                    } else {
                        screen.abortCount += 1;
                    }
                }
            }
        }
    }

    Component {
        id: hostComponent

        ArenaOverlayHost {}
    }

    Component {
        id: overlayComponent

        ArenaGameplayOverlay {}
    }

    Component {
        id: frameComponent

        ArenaOverlayPlacementFrame {}
    }

    function standingRecord(index) {
        const local = index === 0;
        const target = index === 1;
        const dnf = index === 2;
        return {
            "memberId": local ? "member-local" : (target ? "member-target" : "member-" + index),
            "displayName": local ? "Local" : (target ? "Target" : "Player " + index),
            "connected": !dnf,
            "competitionState": target ? "finished" : (dnf ? "dnf" : "playing"),
            "rank": dnf ? 0 : index + 1,
            "hasScore": !dnf,
            "exScore": local ? 200 : (target ? 300 : index * 10),
            "progressPermille": target ? 1000 : (dnf ? 0 : 500),
            "maxCombo": 40 + index,
            "badPoorCount": 3,
            "perfect": 90,
            "great": 20,
            "good": 4,
            "bad": 1,
            "poor": 1,
            "emptyPoor": 1,
            "gaugeType": "hard",
            "gaugeValueMilli": 76500,
            "clearType": target ? "hard" : "",
            "lobbyWinsAfter": target ? 3 : -1,
            "dnfReason": dnf ? "aborted" : ""
        };
    }

    function appendStanding(model, index) {
        model.append(standingRecord(index));
    }

    function populateStandings(session) {
        session.liveStandings.clear();
        for (let index = 0; index < 16; ++index) {
            appendStanding(session.liveStandings, index);
        }
    }

    function createHost(hintVersion = 1) {
        const runner = createTemporaryObject(runnerComponent, testCase);
        verify(runner !== null);
        const session = createTemporaryObject(sessionComponent, testCase, {
            "arenaRunner": runner
        });
        verify(session !== null);
        populateStandings(session);
        const screen = createTemporaryObject(gameplayScreenComponent, testCase, {
            "chart": runner,
            "session": session
        });
        verify(screen !== null);
        const themeVars = createTemporaryObject(themeVarsComponent, testCase);
        verify(themeVars !== null);
        const generalVars = createTemporaryObject(generalVarsComponent, testCase, {
            "arenaOverlayHintVersion": hintVersion
        });
        verify(generalVars !== null);
        const host = createTemporaryObject(hostComponent, testCase, {
            "currentItem": screen,
            "generalVars": generalVars,
            "layoutVariant": "k7",
            "session": session,
            "themeVars": themeVars,
            "width": testCase.width,
            "height": testCase.height
        });
        verify(host !== null);
        wait(1);
        return {
            "runner": runner,
            "session": session,
            "screen": screen,
            "themeVars": themeVars,
            "generalVars": generalVars,
            "host": host
        };
    }

    function rectanglesOverlap(left, right) {
        return left.x < right.x + right.width && left.x + left.width > right.x && left.y < right.y + right.height && left.y + left.height > right.y;
    }

    function verifySafe(rect, viewport) {
        verify(rect.width > 0);
        verify(rect.height > 0);
        verify(rect.x >= 24 - 0.01);
        verify(rect.y >= 24 - 0.01);
        verify(rect.x + rect.width <= viewport.width - 24 + 0.01);
        verify(rect.y + rect.height <= viewport.height - 24 + 0.01);
    }

    function test_drawer_uses_right_left_below_above_then_largest_safe_rect() {
        const viewport = createTemporaryObject(viewportComponent, testCase, {
            "width": 1280,
            "height": 720
        });
        verify(viewport !== null);
        const themeVars = createTemporaryObject(themeVarsComponent, testCase);
        verify(themeVars !== null);
        const frame = createTemporaryObject(frameComponent, viewport, {
            "customizeMode": false,
            "layoutVariant": "k7",
            "placementKind": "gameplayLeaderboard",
            "themeVars": themeVars,
            "viewport": viewport
        });
        verify(frame !== null);

        const cases = [
            {
                "leaderboard": Qt.rect(100, 100, 320, 240),
                "side": "right"
            },
            {
                "leaderboard": Qt.rect(860, 100, 320, 240),
                "side": "left"
            },
            {
                "leaderboard": Qt.rect(300, 80, 680, 200),
                "side": "below"
            },
            {
                "leaderboard": Qt.rect(300, 420, 680, 200),
                "side": "above"
            }
        ];
        for (const entry of cases) {
            frame.resolvedPixelRect = entry.leaderboard;
            const drawer = frame.adjacentChatRect();
            verifySafe(drawer, viewport);
            verify(!rectanglesOverlap(entry.leaderboard, drawer));
            if (entry.side === "right")
                verify(drawer.x >= entry.leaderboard.x + entry.leaderboard.width);
            else if (entry.side === "left")
                verify(drawer.x + drawer.width <= entry.leaderboard.x);
            else if (entry.side === "below")
                verify(drawer.y >= entry.leaderboard.y + entry.leaderboard.height);
            else
                verify(drawer.y + drawer.height <= entry.leaderboard.y);
        }

        frame.resolvedPixelRect = Qt.rect(250, 200, 780, 350);
        const fallback = frame.adjacentChatRect();
        verifySafe(fallback, viewport);
        verify(!rectanglesOverlap(frame.resolvedPixelRect, fallback));
        verify(fallback.y + fallback.height <= frame.resolvedPixelRect.y);
    }

    function test_compact_rows_are_forced_complete_and_scroll_sixteen_members() {
        const session = createTemporaryObject(sessionComponent, testCase);
        verify(session !== null);
        populateStandings(session);
        const overlay = createTemporaryObject(overlayComponent, testCase, {
            "session": session,
            "width": 420,
            "height": 360
        });
        verify(overlay !== null);
        compare(overlay.visible, true);

        const standings = findChild(overlay, "arenaGameplayStandings");
        verify(standings !== null);
        compare(standings.count, 16);
        compare(standings.clip, true);
        compare(standings.Accessible.role, Accessible.List);
        compare(standings.Accessible.focusable, true);
        compare(standings.activeFocusOnTab, true);
        verify(standings.Accessible.name.length > 0);
        verify(standings.contentHeight > standings.height);
        tryVerify(function () {
            return findChild(overlay, "arenaStandingRow2") !== null;
        });

        const localRow = findChild(overlay, "arenaStandingRow0");
        const targetRow = findChild(overlay, "arenaStandingRow1");
        const dnfRow = findChild(overlay, "arenaStandingRow2");
        verify(localRow !== null);
        verify(targetRow !== null);
        verify(dnfRow !== null);
        compare(localRow.Accessible.role, Accessible.ListItem);
        compare(localRow.Accessible.focusable, true);
        compare(localRow.activeFocusOnTab, false);
        verify(localRow.Accessible.name.indexOf("Local") >= 0);
        verify(localRow.Accessible.description.indexOf("EX 200") >= 0);
        verify(localRow.Accessible.description.indexOf("76.5%") >= 0);
        compare(findChild(localRow, "arenaStandingRank").text, "1");
        compare(findChild(dnfRow, "arenaStandingRank").text, "—");
        compare(findChild(localRow, "arenaStandingName").text, "Local");
        compare(findChild(localRow, "arenaStandingScore").text, "EX 200");
        const life = findChild(localRow, "arenaStandingLife");
        verify(life !== null);
        compare(life.text, "76.5%");
        compare(life.visible, true);
        compare(findChild(localRow, "arenaStandingState").text, "Playing");
        compare(findChild(localRow, "arenaStandingLocalMark").visible, true);
        compare(findChild(targetRow, "arenaStandingTargetMark").visible, true);
        verify(findChild(targetRow, "arenaStandingOutcome").text.indexOf("Hard clear") >= 0);
        verify(findChild(targetRow, "arenaStandingOutcome").text.indexOf("3 win") >= 0);
        verify(findChild(dnfRow, "arenaStandingOutcome").text.indexOf("Aborted") >= 0);
        compare(findChild(localRow, "arenaStandingName").Accessible.ignored, true);

        standings.currentIndex = 0;
        standings.forceActiveFocus();
        tryCompare(standings, "activeFocus", true);
        compare(localRow.focusIndicatorVisible, true);
        keyClick(Qt.Key_Down);
        tryCompare(standings, "currentIndex", 1);
        compare(standings.activeFocus, true);
        compare(targetRow.focusIndicatorVisible, true);

        const expand = findChild(overlay, "arenaGameplayExpand");
        verify(expand !== null);
        mouseClick(expand, expand.width / 2, expand.height / 2);
        compare(overlay.expanded, true);
        compare(findChild(localRow, "arenaStandingDetails").visible, true);
        verify(findChild(overlay, "arenaGameplayOptions") === null);
    }

    function test_host_is_current_runner_only_and_resets_round_local_unread() {
        const harness = createHost(1);
        const frame = findChild(harness.host, "arenaGameplayPlacementFrame");
        verify(frame !== null);
        verify(frame.width < harness.host.width);
        verify(frame.height < harness.host.height);
        verify(frame.x > 0);
        verify(frame.y >= 0);
        compare(frame.directMoveEnabled, true);
        compare(frame.directResizeEnabled, true);
        verify(frame.moveHandle !== null);
        compare(harness.host.unreadCount, 0);

        harness.session.chat.append({
            "displayName": "Before",
            "text": "first"
        });
        tryCompare(harness.host, "unreadCount", 1);
        compare(harness.session.gameplayChatOpen, false);

        keyClick(Qt.Key_F8);
        tryCompare(harness.session, "gameplayChatOpen", true);
        compare(harness.host.unreadCount, 0);
        const drawer = findChild(harness.host, "arenaGameplayChatDrawer");
        verify(drawer !== null);
        compare(drawer.visible, true);
        harness.session.chat.append({
            "displayName": "While open",
            "text": "second"
        });
        compare(harness.host.unreadCount, 0);

        harness.session.setGameplayChatOpen(false);
        harness.session.chat.append({
            "displayName": "Closed",
            "text": "third"
        });
        tryCompare(harness.host, "unreadCount", 1);
        harness.session.liveStandings.roundId = "round-2";
        tryCompare(harness.host, "unreadCount", 0);
        compare(harness.session.gameplayChatOpen, false);

        harness.host.currentItem = createTemporaryObject(runnerComponent, testCase);
        wait(1);
        verify(findChild(harness.host, "arenaGameplayPlacementFrame") === null);
    }

    function test_chat_keyboard_focus_and_escape_priority_leave_controller_live() {
        const harness = createHost(1);
        harness.session.chat.append({
            "displayName": "Alice <Admin>",
            "text": "<b>hello</b>"
        });
        keyClick(Qt.Key_F8);
        tryCompare(harness.session, "gameplayChatOpen", true);
        const chatList = findChild(harness.host, "arenaGameplayChatList");
        verify(chatList !== null);
        compare(chatList.Accessible.role, Accessible.List);
        compare(chatList.Accessible.focusable, true);
        compare(chatList.activeFocusOnTab, true);
        verify(chatList.Accessible.name.length > 0);
        tryVerify(function () {
            return findChild(harness.host, "arenaGameplayChatRow0") !== null;
        });
        const chatRow = findChild(harness.host, "arenaGameplayChatRow0");
        verify(chatRow !== null);
        compare(chatRow.Accessible.role, Accessible.ListItem);
        compare(chatRow.Accessible.focusable, true);
        compare(chatRow.activeFocusOnTab, false);
        compare(chatRow.Accessible.name, "Alice <Admin>");
        compare(chatRow.Accessible.description, "<b>hello</b>");
        chatList.currentIndex = 0;
        chatList.forceActiveFocus();
        tryCompare(chatList, "activeFocus", true);
        compare(chatRow.focusIndicatorVisible, true);
        const chatText = findChild(chatRow, "arenaGameplayChatText0");
        verify(chatText !== null);
        compare(chatText.text, "<b>hello</b>");
        compare(chatText.textFormat, Text.PlainText);

        const message = findChild(harness.host, "arenaGameplayMessage");
        verify(message !== null);
        message.forceActiveFocus();
        tryCompare(message, "activeFocus", true);
        compare(harness.screen.enabled, true);
        harness.screen.controllerPulse();
        compare(harness.screen.controllerCount, 1);

        message.text = "hello";
        keyClick(Qt.Key_Return);
        compare(harness.session.sentMessages, ["hello"]);
        compare(message.text, "");
        compare(message.activeFocus, true);

        message.text = "line one";
        compare(message.activeFocus, true);
        keyClick(Qt.Key_Return, Qt.ShiftModifier);
        compare(harness.session.sentMessages, ["hello"]);
        compare(message.activeFocus, true);
        verify(message.text.indexOf("\n") >= 0);

        keyClick(Qt.Key_Escape);
        tryCompare(harness.session, "gameplayChatOpen", false);
        compare(harness.screen.abortCount, 0);

        keyClick(Qt.Key_F2);
        tryCompare(harness.host, "customizeMode", true);
        keyClick(Qt.Key_Escape);
        tryCompare(harness.host, "customizeMode", false);
        compare(harness.screen.abortCount, 0);
        keyClick(Qt.Key_Escape);
        tryCompare(harness.screen, "abortCount", 1);
    }

    function test_first_use_hint_is_profile_versioned_nonmodal_and_dismissible() {
        const harness = createHost(0);
        const hint = findChild(harness.host, "arenaOverlayPlacementHint");
        verify(hint !== null);
        compare(hint.visible, true);
        compare(hint.activeFocus, false);
        compare(hint.activeFocusOnTab, true);
        compare(hint.Accessible.role, Accessible.Button);
        verify(hint.Accessible.name.length > 0);
        verify(hint.Accessible.description.indexOf("F2") >= 0);
        hint.forceActiveFocus();
        compare(hint.border.width, 2);
        keyClick(Qt.Key_Space);
        tryCompare(harness.generalVars, "arenaOverlayHintVersion", 1);
        compare(hint.visible, false);
        compare(hint.activeFocus, false);
        compare(hint.activeFocusOnTab, false);
        compare(harness.screen.activeFocus, true);

        const next = createHost(1);
        const nextHint = findChild(next.host, "arenaOverlayPlacementHint");
        verify(nextHint !== null);
        compare(nextHint.visible, false);
    }

    function test_gameplay_announcements_are_transition_only_and_round_scoped() {
        const session = createTemporaryObject(sessionComponent, testCase);
        verify(session !== null);
        appendStanding(session.liveStandings, 0);
        const overlay = createTemporaryObject(overlayComponent, testCase, {
            "session": session,
            "width": 420,
            "height": 360
        });
        verify(overlay !== null);
        wait(1);
        compare(overlay.announcementCount, 0);
        compare(overlay.lastAnnouncementText, "");
        compare(overlay.stateText(false, "finished"), "Finished");
        compare(overlay.stateText(false, "dnf"), "DNF");
        compare(overlay.stateText(false, "playing"), "Disconnected");

        session.liveStandings.setProperty(0, "connected", false);
        tryCompare(overlay, "announcementCount", 1);
        verify(overlay.lastAnnouncementText.indexOf("Local") >= 0);
        verify(overlay.lastAnnouncementText.indexOf("disconnected") >= 0);

        session.liveStandings.setProperty(0, "exScore", 250);
        wait(1);
        compare(overlay.announcementCount, 1);
        session.liveStandings.setProperty(0, "connected", true);
        tryCompare(overlay, "announcementCount", 2);
        verify(overlay.lastAnnouncementText.indexOf("reconnected") >= 0);

        session.liveStandings.setProperty(0, "dnfReason", "aborted");
        session.liveStandings.setProperty(0, "competitionState", "dnf");
        tryCompare(overlay, "announcementCount", 3);
        verify(overlay.lastAnnouncementText.indexOf("Local") >= 0);
        verify(overlay.lastAnnouncementText.indexOf("Aborted") >= 0);

        session.liveStandings.setProperty(0, "rank", 1);
        session.liveStandings.setProperty(0, "dnfReason", "");
        session.liveStandings.setProperty(0, "competitionState", "finished");
        tryCompare(overlay, "announcementCount", 4);
        verify(overlay.lastAnnouncementText.indexOf("Local") >= 0);
        verify(overlay.lastAnnouncementText.indexOf("first") >= 0);

        session.liveStandings.setProperty(0, "competitionState", "finished");
        wait(1);
        compare(overlay.announcementCount, 4);

        session.liveStandings.roundId = "round-2";
        session.liveStandings.clear();
        appendStanding(session.liveStandings, 0);
        appendStanding(session.liveStandings, 1);
        appendStanding(session.liveStandings, 3);
        wait(1);
        compare(overlay.announcementCount, 4);

        const winner = standingRecord(0);
        winner.connected = false;
        winner.competitionState = "finished";
        winner.clearType = "hard";
        const finisher = standingRecord(1);
        finisher.connected = false;
        finisher.competitionState = "finished";
        const finalDnf = standingRecord(3);
        finalDnf.connected = false;
        finalDnf.competitionState = "dnf";
        finalDnf.rank = 0;
        finalDnf.dnfReason = "left";
        session.liveStandings.clear();
        session.liveStandings.append(winner);
        session.liveStandings.append(finisher);
        session.liveStandings.append(finalDnf);
        tryCompare(overlay, "announcementCount", 6);
        verify(overlay.lastAnnouncementText.indexOf("Left the room") >= 0);
        verify(overlay.lastAnnouncementText.indexOf("disconnected") < 0);
    }

    function test_standings_focus_survives_reset_and_member_reorder() {
        const session = createTemporaryObject(sessionComponent, testCase);
        verify(session !== null);
        appendStanding(session.liveStandings, 0);
        appendStanding(session.liveStandings, 1);
        appendStanding(session.liveStandings, 3);
        const overlay = createTemporaryObject(overlayComponent, testCase, {
            "session": session,
            "width": 420,
            "height": 360
        });
        verify(overlay !== null);
        const standings = findChild(overlay, "arenaGameplayStandings");
        verify(standings !== null);
        standings.forceActiveFocus();
        tryCompare(standings, "activeFocus", true);
        keyClick(Qt.Key_Down);
        tryCompare(standings, "currentIndex", 1);
        compare(standings.currentItem.memberId, "member-target");

        const local = standingRecord(0);
        const target = standingRecord(1);
        const third = standingRecord(3);
        session.liveStandings.clear();
        session.liveStandings.append(third);
        session.liveStandings.append(local);
        session.liveStandings.append(target);
        tryCompare(standings, "count", 3);
        tryCompare(standings, "currentIndex", 2);
        tryVerify(function () {
            return standings.currentItem !== null && standings.currentItem.memberId === "member-target";
        });
        compare(standings.activeFocus, true);
        compare(standings.currentItem.focusIndicatorVisible, true);
    }
}

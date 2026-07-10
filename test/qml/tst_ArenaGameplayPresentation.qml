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

        QtObject {
        }
    }

    Component {
        id: generalVarsComponent

        QtObject {
            property int arenaOverlayHintVersion: 0
        }
    }

    Component {
        id: themeVarsComponent

        FakeArenaThemeVars {
        }
    }

    Component {
        id: viewportComponent

        Item {
        }
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
                overlayCustomizationActive = !!active && arenaGameplayActive
                    && arenaRunner !== null;
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

        ArenaOverlayHost {
        }
    }

    Component {
        id: overlayComponent

        ArenaGameplayOverlay {
        }
    }

    Component {
        id: frameComponent

        ArenaOverlayPlacementFrame {
        }
    }

    function appendStanding(model, index) {
        const local = index === 0;
        const target = index === 1;
        const dnf = index === 2;
        model.append({
            "memberId": local ? "member-local"
                              : (target ? "member-target" : "member-" + index),
            "displayName": local ? "Local"
                                 : (target ? "Target" : "Player " + index),
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
        });
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
        const generalVars = createTemporaryObject(generalVarsComponent,
                                                  testCase, {
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
        return left.x < right.x + right.width
            && left.x + left.width > right.x
            && left.y < right.y + right.height
            && left.y + left.height > right.y;
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
            { "leaderboard": Qt.rect(100, 100, 320, 240), "side": "right" },
            { "leaderboard": Qt.rect(860, 100, 320, 240), "side": "left" },
            { "leaderboard": Qt.rect(300, 80, 680, 200), "side": "below" },
            { "leaderboard": Qt.rect(300, 420, 680, 200), "side": "above" }
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
        verify(standings.contentHeight > standings.height);
        tryVerify(function() {
            return findChild(overlay, "arenaStandingRow2") !== null;
        });

        const localRow = findChild(overlay, "arenaStandingRow0");
        const targetRow = findChild(overlay, "arenaStandingRow1");
        const dnfRow = findChild(overlay, "arenaStandingRow2");
        verify(localRow !== null);
        verify(targetRow !== null);
        verify(dnfRow !== null);
        compare(findChild(localRow, "arenaStandingRank").text, "1");
        compare(findChild(dnfRow, "arenaStandingRank").text, "—");
        compare(findChild(localRow, "arenaStandingName").text, "Local");
        compare(findChild(localRow, "arenaStandingScore").text, "EX 200");
        compare(findChild(localRow, "arenaStandingProgress").text, "50%");
        compare(findChild(localRow, "arenaStandingState").text, "Playing");
        compare(findChild(localRow, "arenaStandingLocalMark").visible, true);
        compare(findChild(targetRow, "arenaStandingTargetMark").visible, true);
        verify(findChild(targetRow, "arenaStandingOutcome").text.indexOf("Hard clear") >= 0);
        verify(findChild(targetRow, "arenaStandingOutcome").text.indexOf("3 win") >= 0);
        verify(findChild(dnfRow, "arenaStandingOutcome").text.indexOf("Aborted") >= 0);

        const expand = findChild(overlay, "arenaGameplayExpand");
        verify(expand !== null);
        mouseClick(expand, expand.width / 2, expand.height / 2);
        compare(overlay.expanded, true);
        compare(findChild(localRow, "arenaStandingDetails").visible, true);
        verify(findChild(localRow, "arenaStandingGauge").text
               .indexOf("76.5%") >= 0);
        compare(findChild(overlay, "arenaGameplayOptions").visible, true);
    }

    function test_host_is_current_runner_only_and_resets_round_local_unread() {
        const harness = createHost(1);
        const frame = findChild(harness.host,
                                "arenaGameplayPlacementFrame");
        verify(frame !== null);
        verify(frame.width < harness.host.width);
        verify(frame.height < harness.host.height);
        verify(frame.x > 0);
        verify(frame.y >= 0);
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

        harness.host.currentItem = createTemporaryObject(runnerComponent,
                                                         testCase);
        wait(1);
        verify(findChild(harness.host,
                         "arenaGameplayPlacementFrame") === null);
    }

    function test_chat_keyboard_focus_and_escape_priority_leave_controller_live() {
        const harness = createHost(1);
        keyClick(Qt.Key_F8);
        tryCompare(harness.session, "gameplayChatOpen", true);
        const message = findChild(harness.host, "arenaGameplayMessage");
        verify(message !== null);
        tryCompare(message, "activeFocus", true);
        compare(harness.screen.enabled, true);
        harness.screen.controllerPulse();
        compare(harness.screen.controllerCount, 1);

        message.text = "hello";
        keyClick(Qt.Key_Return);
        compare(harness.session.sentMessages, ["hello"]);
        compare(message.text, "");

        message.text = "line one";
        keyClick(Qt.Key_Return, Qt.ShiftModifier);
        compare(harness.session.sentMessages, ["hello"]);
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
        mouseClick(hint, hint.width / 2, hint.height / 2);
        tryCompare(harness.generalVars, "arenaOverlayHintVersion", 1);
        compare(hint.visible, false);

        const next = createHost(1);
        const nextHint = findChild(next.host, "arenaOverlayPlacementHint");
        verify(nextHint !== null);
        compare(nextHint.visible, false);
    }
}

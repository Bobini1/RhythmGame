import QtQuick
import QtTest
import RhythmGameQml

TestCase {
    id: testCase

    name: "ArenaResultPresentation"
    when: windowShown
    width: 1280
    height: 720
    visible: true

    Component {
        id: sessionComponent

        Item {
            id: fakeSession

            property string selfMemberId: "member-local"
            property alias presentedResult: resultState
            property bool resultPresentationActive: true
            property bool arenaGameplayActive: false
            property var arenaRunner: null
            property bool gameplayChatOpen: false
            property bool overlayCustomizationActive: false
            property alias liveStandings: liveStandingsModel
            property alias chat: chatModel

            function setGameplayChatOpen(open) {
                gameplayChatOpen = !!open && arenaGameplayActive;
            }

            function toggleGameplayChat() {
                setGameplayChatOpen(!gameplayChatOpen);
            }

            function setOverlayCustomizationActive(active) {
                overlayCustomizationActive = !!active && arenaGameplayActive;
            }

            QtObject {
                id: resultState

                property bool valid: true
                property bool finalized: false
                property string roundId: "round-1"
                property int participantCount: 4
                property var winnerNames: []
                property int localRank: 0
                property bool localDnf: false
                property bool localWinner: false
                property string selectionTitle: "Test chart"
                property string selectionOptionsSummary: "MIRROR · OFF"
                property var standings: standingsModel
            }

            ListModel {
                id: standingsModel
            }

            ListModel {
                id: liveStandingsModel

                property string roundId: ""
            }

            ListModel {
                id: chatModel
            }

            function installFinalRows() {
                standingsModel.clear();
                standingsModel.append({
                    "memberId": "member-a",
                    "displayName": "Alice",
                    "connected": false,
                    "competitionState": "finished",
                    "rank": 1,
                    "hasScore": true,
                    "exScore": 1000,
                    "progressPermille": 1000,
                    "maxCombo": 500,
                    "badPoorCount": 2,
                    "perfect": 450,
                    "great": 100,
                    "good": 0,
                    "bad": 1,
                    "poor": 1,
                    "emptyPoor": 0,
                    "gaugeType": "hard",
                    "gaugeValueMilli": 82000,
                    "clearType": "hard",
                    "lobbyWinsAfter": 3,
                    "dnfReason": ""
                });
                standingsModel.append({
                    "memberId": "member-b",
                    "displayName": "Bob",
                    "connected": false,
                    "competitionState": "finished",
                    "rank": 1,
                    "hasScore": true,
                    "exScore": 1000,
                    "progressPermille": 1000,
                    "maxCombo": 480,
                    "badPoorCount": 3,
                    "perfect": 450,
                    "great": 100,
                    "good": 0,
                    "bad": 1,
                    "poor": 1,
                    "emptyPoor": 1,
                    "gaugeType": "normal",
                    "gaugeValueMilli": 76000,
                    "clearType": "normal",
                    "lobbyWinsAfter": 8,
                    "dnfReason": ""
                });
                standingsModel.append({
                    "memberId": "member-local",
                    "displayName": "Local player",
                    "connected": false,
                    "competitionState": "finished",
                    "rank": 3,
                    "hasScore": true,
                    "exScore": 900,
                    "progressPermille": 1000,
                    "maxCombo": 400,
                    "badPoorCount": 5,
                    "perfect": 400,
                    "great": 100,
                    "good": 0,
                    "bad": 2,
                    "poor": 2,
                    "emptyPoor": 1,
                    "gaugeType": "easy",
                    "gaugeValueMilli": 70000,
                    "clearType": "easy",
                    "lobbyWinsAfter": 1,
                    "dnfReason": ""
                });
                standingsModel.append({
                    "memberId": "member-dnf",
                    "displayName": "Disconnected player",
                    "connected": false,
                    "competitionState": "dnf",
                    "rank": 0,
                    "hasScore": false,
                    "exScore": 0,
                    "progressPermille": 0,
                    "maxCombo": 0,
                    "badPoorCount": 0,
                    "perfect": 0,
                    "great": 0,
                    "good": 0,
                    "bad": 0,
                    "poor": 0,
                    "emptyPoor": 0,
                    "gaugeType": "",
                    "gaugeValueMilli": 0,
                    "clearType": "",
                    "lobbyWinsAfter": -1,
                    "dnfReason": "left"
                });
                resultState.winnerNames = ["Alice", "Bob"];
                resultState.localRank = 3;
                resultState.localDnf = false;
                resultState.finalized = true;
            }
        }
    }

    Component {
        id: overlayComponent

        ArenaResultOverlay {}
    }

    Component {
        id: themeVarsComponent

        FakeArenaThemeVars {}
    }

    Component {
        id: competitionTextComponent

        ArenaCompetitionText {}
    }

    Component {
        id: generalVarsComponent

        QtObject {
            property int arenaOverlayHintVersion: 1
        }
    }

    Component {
        id: placementFrameComponent

        ArenaOverlayPlacementFrame {}
    }

    Component {
        id: overlayHostComponent

        ArenaOverlayHost {}
    }

    Component {
        id: legacyResultItemComponent

        Item {
            property string arenaRoundId: "round-1"
            property bool arenaNativeResultPresentation: false
            property bool resultCustomizationActive: false

            function setArenaResultCustomizationActive(active) {
                resultCustomizationActive = active;
            }
        }
    }

    Component {
        id: viewportComponent

        Item {}
    }

    function createOverlay() {
        const session = createTemporaryObject(sessionComponent, testCase);
        verify(session !== null);
        const overlay = createTemporaryObject(overlayComponent, testCase, {
            "width": 520,
            "height": 520,
            "session": session,
            "placementKind": "resultStandings",
            "resolvedSkinId": "legacy-result",
            "layoutVariant": "result"
        });
        verify(overlay !== null);
        wait(1);
        return {
            "session": session,
            "overlay": overlay
        };
    }

    function test_pending_and_all_dnf_summaries_stay_visible() {
        const harness = createOverlay();
        compare(harness.overlay.expanded, false);
        compare(harness.overlay.statusText, "Waiting for players…");
        compare(harness.overlay.winnerSummaryText, "Waiting for final standings");
        compare(harness.overlay.localStandingText, "— / 4");

        harness.session.presentedResult.finalized = true;
        harness.session.presentedResult.localDnf = true;
        wait(1);
        compare(harness.overlay.statusText, "Final");
        compare(harness.overlay.winnerSummaryText, "No winner");
        compare(harness.overlay.localStandingText, "DNF / 4");
    }

    function test_joint_winners_competition_ranks_local_marker_and_wins() {
        const harness = createOverlay();
        harness.session.installFinalRows();
        wait(1);
        compare(harness.overlay.winnerSummaryText, "Winners: Alice, Bob");
        compare(harness.overlay.localStandingText, "#3 / 4");

        harness.overlay.expanded = true;
        wait(1);
        const standings = findChild(harness.overlay, "arenaResultStandings");
        verify(standings !== null);
        compare(standings.visible, true);
        compare(standings.count, 4);
        tryVerify(function () {
            return standings.width > 0 && standings.height > 0;
        }, 1000);
        standings.forceLayout();
        wait(1);

        const winnerA = standings.itemAtIndex(0);
        const winnerB = standings.itemAtIndex(1);
        const local = standings.itemAtIndex(2);
        const dnf = standings.itemAtIndex(3);
        verify(winnerA !== null, "Expected delegate 0; size=" + standings.width + "x" + standings.height + ", contentHeight=" + standings.contentHeight);
        verify(winnerB !== null);
        verify(local !== null);
        verify(dnf !== null);
        compare(winnerA.rankLabel, "#1");
        compare(winnerB.rankLabel, "#1");
        compare(local.rankLabel, "#3");
        compare(local.localMarkerVisible, true);
        compare(local.winsLabel, "1 win(s)");
        compare(dnf.rankLabel, "DNF");
        compare(dnf.winsLabel, "Wins —");
        compare(dnf.detailsLabel, "Did not finish · Left the room");
        compare(winnerB.gaugeLabel, "Normal · 76.0%");
        compare(local.activeFocusOnTab, true);
        tryCompare(harness.overlay, "lastAnnouncementText", "Arena result. Winners: Alice, Bob. Your standing: #3 / 4");
    }

    function test_competition_text_never_exposes_protocol_tokens() {
        const labels = createTemporaryObject(competitionTextComponent, testCase);
        verify(labels !== null);
        compare(labels.gaugeTypeText("aeasy"), "Assist Easy");
        compare(labels.clearTypeText("exhard"), "EX Hard");
        compare(labels.dnfReasonText("result_unavailable"), "Result unavailable");
        compare(labels.dnfReasonText("grace_expired"), "Reconnect grace expired");
        compare(labels.dnfReasonText("not-a-protocol-value"), "Unknown");
    }

    function test_native_result_controls_keep_screen_space_targets() {
        const session = createTemporaryObject(sessionComponent, testCase);
        verify(session !== null);
        session.installFinalRows();
        const component = Qt.createComponent(Qt.resolvedUrl("../../share/RhythmGame/themes/Default/scripts/result/ArenaResultPanel.qml"));
        tryCompare(component, "status", Component.Ready, 3000);
        const panel = createTemporaryObject(component, testCase, {
            "width": 480,
            "height": 624,
            "result": session.presentedResult,
            "localMemberId": "member-local",
            "statsFontFamily": "",
            "textFontFamily": ""
        });
        verify(panel !== null);
        wait(1);
        const expand = findChild(panel, "arenaNativeResultExpand");
        verify(expand !== null);
        verify(expand.width >= 32);
        verify(expand.height >= 32);
        const standings = findChild(panel, "arenaNativeResultStandings");
        verify(standings !== null);
        tryVerify(function () {
            return standings.count === 4 && standings.itemAtIndex(0) !== null;
        }, 1000);
        compare(standings.itemAtIndex(0).activeFocusOnTab, true);
    }

    function test_result_placement_uses_independent_theme_fields() {
        const harness = createOverlay();
        const viewport = createTemporaryObject(viewportComponent, testCase, {
            "width": 1280,
            "height": 720
        });
        verify(viewport !== null);
        const themeVars = createTemporaryObject(themeVarsComponent, testCase, {
            "arenaOverlayK7XNormalized": 0.1,
            "arenaOverlayK7YNormalized": 0.2,
            "arenaOverlayK7WidthNormalized": 0.3,
            "arenaOverlayK7HeightNormalized": 0.4
        });
        verify(themeVars !== null);
        const frame = createTemporaryObject(placementFrameComponent, viewport, {
            "themeVars": themeVars,
            "viewport": viewport,
            "placementKind": "resultStandings",
            "layoutVariant": "result",
            "customizeMode": true
        });
        verify(frame !== null);
        wait(1);
        frame.moveBy(-40, 20, true);
        verify(themeVars.arenaOverlayResultXNormalized >= 0);
        verify(themeVars.arenaOverlayResultYNormalized >= 0);
        compare(themeVars.arenaOverlayK7XNormalized, 0.1);
        compare(themeVars.arenaOverlayK7YNormalized, 0.2);
        compare(harness.overlay.placementKind, "resultStandings");
    }

    function test_result_customization_guard_releases_after_escape_event() {
        const session = createTemporaryObject(sessionComponent, testCase);
        const resultItem = createTemporaryObject(legacyResultItemComponent, testCase);
        const themeVars = createTemporaryObject(themeVarsComponent, testCase);
        const generalVars = createTemporaryObject(generalVarsComponent, testCase);
        verify(session !== null);
        verify(resultItem !== null);
        verify(themeVars !== null);
        verify(generalVars !== null);
        const host = createTemporaryObject(overlayHostComponent, testCase, {
            "width": 1280,
            "height": 720,
            "session": session,
            "currentItem": resultItem,
            "generalVars": generalVars,
            "themeVars": themeVars,
            "layoutVariant": "",
            "resultResolvedSkinId": "legacy-result",
            "resultThemeVars": themeVars
        });
        verify(host !== null);
        wait(1);
        compare(host.legacyArenaResult, true);
        compare(resultItem.resultCustomizationActive, false);
        const mountedFrame = findChild(host, "arenaResultPlacementFrame");
        verify(mountedFrame !== null);
        verify(mountedFrame.width < host.width);
        verify(mountedFrame.height < host.height);
        verify(mountedFrame.x > 0);

        host.resultCustomizeMode = true;
        compare(resultItem.resultCustomizationActive, true);
        host.resultCustomizeMode = false;
        compare(resultItem.resultCustomizationActive, true);
        wait(1);
        compare(resultItem.resultCustomizationActive, false);
    }
}

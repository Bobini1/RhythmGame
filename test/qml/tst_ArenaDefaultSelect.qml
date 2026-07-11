pragma ComponentBehavior: Bound

import QtQuick
import QtTest
import RhythmGameQml

TestCase {
    id: testCase

    name: "ArenaDefaultSelect"
    when: windowShown
    width: 1000
    height: 800
    visible: true

    SignalSpy {
        id: kickSpy

        signalName: "kickRequested"
    }

    Component {
        id: fakeSessionComponent

        FakeArenaSession {}
    }

    Component {
        id: rosterComponent

        ArenaRosterView {}
    }

    Component {
        id: chatComponent

        ArenaChatView {}
    }

    Component {
        id: summaryComponent

        ArenaSelectionSummary {}
    }

    Component {
        id: panelComponent

        ArenaSelectPanel {}
    }

    Component {
        id: themeVarsComponent

        FakeArenaThemeVars {}
    }

    Component {
        id: scenePanelMountComponent

        Item {
            id: sceneViewport

            required property var session
            required property var themeVars
            property alias panelLoader: overlayLoader
            readonly property real contentScale: Math.min(width / 1920,
                                                           height / 1080)
            readonly property real contentLeft:
                (width - 1920 * contentScale) / 2
            readonly property real contentTop:
                (height - 1080 * contentScale) / 2
            readonly property rect scaledGapHint:
                Qt.rect(contentLeft + 728 * contentScale,
                        contentTop + 120 * contentScale,
                        520 * contentScale,
                        480 * contentScale)

            Loader {
                id: overlayLoader

                anchors.fill: parent
                sourceComponent: ArenaSelectOverlay {
                    defaultPixelRectHint: sceneViewport.scaledGapHint
                    session: sceneViewport.session
                    themeVars: sceneViewport.themeVars
                    viewport: sceneViewport
                }
            }
        }
    }

    Component {
        id: stripComponent

        ArenaSelectStrip {}
    }

    function addMembers(session, count = 16) {
        for (let index = 0; index < count; ++index) {
            const number = index + 1;
            session.members.append({
                "memberId": "member-" + number,
                "displayName": index === 0 ? "<b>Remote & local</b>" : "Player " + number,
                "avatarUrl": "",
                "connected": index % 4 !== 3,
                "owner": index === 0,
                "self": index === 0,
                "lobbyWins": index,
                "ready": index % 2 === 0,
                "inventoryState": index === 1 ? "syncing" : "ready",
                "inventoryRevision": 10,
                "availabilityAppliedRevision": index === 2 ? 9 : 10,
                "roundState": index === 3 ? "waiting" : index === 4 ? "loading" : "eligible"
            });
        }
    }

    function addChat(session, count = 20) {
        for (let index = 0; index < count; ++index) {
            session.chat.append({
                "messageId": "message-" + index,
                "memberId": "member-" + ((index % 4) + 1),
                "displayName": index === 0 ? "<i>Remote name</i>" : "Player",
                "text": index === 0 ? "<img src='bad'> plain" : "Message " + index,
                "timestamp": index,
                "self": index % 4 === 0
            });
        }
    }

    function createSession() {
        const session = createTemporaryObject(fakeSessionComponent, testCase);
        verify(session !== null);
        addMembers(session);
        addChat(session);
        session.selectedTitle = "<b>Selected chart</b>";
        session.selectedByMemberId = "member-2";
        session.lastResult.valid = true;
        session.lastResult.finalized = true;
        session.lastResult.participantCount = 16;
        session.lastResult.winnerMemberIds = ["member-1", "member-2"];
        session.lastResult.winnerNames = ["<u>Winner one</u>", "Winner two"];
        session.lastResult.selectionTitle = "Previous chart";
        return session;
    }

    function verifyMinimumSceneTarget(target, viewport, label) {
        verify(target !== null, label);
        const topLeft = target.mapToItem(viewport, 0, 0);
        const bottomRight = target.mapToItem(viewport, target.width,
                                             target.height);
        verify(Math.abs(bottomRight.x - topLeft.x) >= 32, label + " width");
        verify(Math.abs(bottomRight.y - topLeft.y) >= 32, label + " height");
    }

    function closeEnough(actual, expected, label, epsilon = 0.01) {
        verify(Math.abs(actual - expected) <= epsilon,
               label + ": expected " + actual + " to be within "
               + epsilon + " of " + expected);
    }

    function compareRect(actual, expected, label) {
        closeEnough(actual.x, expected.x, label + " x");
        closeEnough(actual.y, expected.y, label + " y");
        closeEnough(actual.width, expected.width, label + " width");
        closeEnough(actual.height, expected.height, label + " height");
    }

    function test_roster_exposes_all_room_states_and_owner_moderation() {
        const session = createSession();
        const roster = createTemporaryObject(rosterComponent, testCase, {
            "height": 520,
            "moderationEnabled": true,
            "session": session,
            "width": 520
        });
        verify(roster !== null);
        tryCompare(roster, "memberCount", 16);
        compare(roster.connectedCount, 12);
        compare(roster.reservedCount, 4);

        const localName = findChild(roster, "arenaRosterName-member-1");
        verify(localName !== null);
        compare(localName.textFormat, Text.PlainText);
        compare(localName.text, "<b>Remote & local</b>");
        const localMarkers = findChild(roster, "arenaRosterMarkers-member-1");
        verify(localMarkers !== null);
        verify(localMarkers.text.indexOf("you") >= 0);
        verify(localMarkers.text.indexOf("owner") >= 0);
        verify(localMarkers.text.indexOf("winner") >= 0);

        const syncStatus = findChild(roster, "arenaRosterStatus-member-2");
        verify(syncStatus !== null);
        verify(syncStatus.text.indexOf("library") >= 0);
        const staleStatus = findChild(roster, "arenaRosterStatus-member-3");
        verify(staleStatus.text.indexOf("availability") >= 0);
        const reservedStatus = findChild(roster, "arenaRosterStatus-member-4");
        verify(reservedStatus.text.indexOf("Reserved") >= 0);
        verify(reservedStatus.text.indexOf("Waiting") >= 0);
        const loadingStatus = findChild(roster, "arenaRosterStatus-member-5");
        verify(loadingStatus.text.indexOf("Loading") >= 0);

        const kick = findChild(roster, "arenaRosterKick-member-2");
        verify(kick !== null);
        compare(kick.visible, true);
        kickSpy.target = roster;
        kickSpy.clear();
        mouseClick(kick, kick.width / 2, kick.height / 2, Qt.LeftButton);
        compare(kickSpy.count, 1);
        compare(kickSpy.signalArguments[0][0], "member-2");

        session.isOwner = false;
        wait(1);
        compare(kick.visible, false);
    }

    function test_roster_rows_fit_content_and_pause_moderation_during_reconnect() {
        const session = createSession();
        const roster = createTemporaryObject(rosterComponent, testCase, {
            "compact": true,
            "height": 520,
            "moderationEnabled": true,
            "session": session,
            "width": 520
        });
        verify(roster !== null);

        const member = findChild(roster, "arenaRosterMember-member-2");
        const nextMember = findChild(roster, "arenaRosterMember-member-3");
        const rosterList = findChild(roster, "arenaRosterList");
        verify(member !== null);
        verify(nextMember !== null);
        verify(rosterList !== null);
        verify(member.height > 48);
        compare(member.activeFocusOnTab, false);
        rosterList.currentIndex = 1;
        roster.forceActiveFocus();
        keyClick(Qt.Key_Tab);
        tryCompare(rosterList, "activeFocus", true);
        compare(member.border.width, 2);
        keyClick(Qt.Key_Down);
        compare(member.border.width, 0);
        compare(nextMember.border.width, 2);

        const kick = findChild(roster, "arenaRosterKick-member-2");
        verify(kick !== null);
        compare(kick.visible, true);
        compare(kick.enabled, true);

        session.reconnecting = true;
        compare(kick.visible, true);
        compare(kick.enabled, false);
        verify(kick.Accessible.description.indexOf("reconnect") >= 0);
    }

    function test_selection_summary_formats_existing_session_state() {
        const session = createSession();
        const summary = createTemporaryObject(summaryComponent, testCase, {
            "session": session,
            "width": 500
        });
        verify(summary !== null);

        const title = findChild(summary, "arenaSelectionTitle");
        compare(title.text, "<b>Selected chart</b>");
        compare(title.textFormat, Text.PlainText);
        const selector = findChild(summary, "arenaSelectionSelector");
        verify(selector.text.indexOf("member-2") >= 0);
        const options = findChild(summary, "arenaSelectionOptions");
        compare(options.text, "MIRROR · FLIP");
        const winners = findChild(summary, "arenaLastWinners");
        compare(winners.textFormat, Text.PlainText);
        verify(winners.text.indexOf("<u>Winner one</u>") >= 0);

        session.roundsAvailable = false;
        compare(summary.readyDisabledReason.length > 0, true);
        session.roundsAvailable = true;
        session.availabilitySyncing = true;
        verify(summary.syncText.indexOf("libraries") >= 0);
        session.availabilitySyncing = false;
        session.currentRoundId = "round-1";
        verify(summary.syncText.indexOf("Preparing") >= 0);
        session.currentRoundId = "";
        session.errorMessageKey = "arena.error.resourceFailed";
        verify(summary.syncText.indexOf("cancelled") >= 0);
    }

    function test_chat_tail_follow_plain_text_and_send() {
        const session = createSession();
        const chat = createTemporaryObject(chatComponent, testCase, {
            "chatModel": session.chat,
            "height": 360,
            "session": session,
            "width": 520
        });
        verify(chat !== null);
        tryCompare(chat, "messageCount", 20);
        tryVerify(() => chat.atTail);

        chat.scrollToBeginning();
        wait(1);
        const firstName = findChild(chat, "arenaChatName-message-0");
        const firstBody = findChild(chat, "arenaChatBody-message-0");
        const firstMessage = findChild(chat, "arenaChatMessage-message-0");
        const secondMessage = findChild(chat, "arenaChatMessage-message-1");
        const chatList = findChild(chat, "arenaChatList");
        verify(firstMessage !== null);
        verify(secondMessage !== null);
        verify(chatList !== null);
        compare(firstMessage.activeFocusOnTab, false);
        chatList.currentIndex = 0;
        chat.forceActiveFocus();
        keyClick(Qt.Key_Tab);
        tryCompare(chatList, "activeFocus", true);
        compare(firstMessage.border.width, 2);
        keyClick(Qt.Key_Down);
        compare(firstMessage.border.width, 0);
        compare(secondMessage.border.width, 2);
        compare(firstName.textFormat, Text.PlainText);
        compare(firstBody.textFormat, Text.PlainText);
        compare(firstBody.text, "<img src='bad'> plain");

        const input = findChild(chat, "arenaChatInput");
        verify(input !== null);
        input.forceActiveFocus();
        input.text = "hello arena";
        keyClick(Qt.Key_Return);
        compare(session.sentMessages, ["hello arena"]);
        compare(input.text, "");

        compare(chat.followTail, false);
        const oldPosition = chat.scrollPosition;
        session.chat.append({
            "messageId": "message-new",
            "memberId": "member-2",
            "displayName": "Player 2",
            "text": "new message",
            "timestamp": 99,
            "self": false
        });
        wait(1);
        compare(chat.scrollPosition, oldPosition);
    }

    function test_default_panel_uses_shared_exclusive_tabs_and_routes_actions() {
        const session = createSession();
        const panel = createTemporaryObject(panelComponent, testCase, {
            "height": 480,
            "session": session,
            "width": 640
        });
        verify(panel !== null);
        compare(panel.arenaNativeSelectPresentation, true);
        const details = findChild(panel, "arenaSelectDetailsTab");
        const chat = findChild(panel, "arenaSelectChatTab");
        const roster = findChild(panel, "arenaSelectRoster");
        const selection = findChild(panel, "arenaSelectSelection");
        const ready = findChild(panel, "arenaSelectReady");
        const leave = findChild(panel, "arenaSelectLeave");
        verify(details !== null);
        verify(chat !== null);
        verify(roster !== null);
        verify(selection !== null);
        verify(ready !== null);
        verify(leave !== null);
        compare(findChild(panel, "arenaLegacyExpand"), null);

        compare(details.checked, true);
        compare(chat.checked, false);
        mouseClick(details, details.width / 2, details.height / 2,
                   Qt.LeftButton);
        compare(details.checked, true);
        compare(chat.checked, false);

        mouseClick(ready, ready.width / 2, ready.height / 2, Qt.LeftButton);
        compare(session.readyRequests, [true]);

        tryVerify(function() {
            return findChild(panel, "arenaRosterKick-member-2") !== null;
        });
        const kick = findChild(panel, "arenaRosterKick-member-2");
        mouseClick(kick, kick.width / 2, kick.height / 2, Qt.LeftButton);
        compare(session.kickedMemberIds, ["member-2"]);

        mouseClick(chat, chat.width / 2, chat.height / 2, Qt.LeftButton);
        compare(details.checked, false);
        compare(chat.checked, true);
        compare(panel.detailMode, "chat");
        chat.forceActiveFocus();
        keyClick(Qt.Key_Space);
        compare(chat.checked, true);
        const chatView = findChild(panel, "arenaSelectChat");
        verify(chatView !== null);
        const chatInput = findChild(chatView, "arenaChatInput");
        verify(chatInput !== null);
        chatInput.forceActiveFocus();
        chatInput.text = "default hello";
        keyClick(Qt.Key_Return);
        compare(session.sentMessages, ["default hello"]);

        mouseClick(leave, leave.width / 2, leave.height / 2, Qt.LeftButton);
        compare(session.leaveCount, 1);
    }

    function test_ready_disabled_reason_stays_visible_on_chat_tab() {
        const session = createSession();
        session.canReady = false;
        const panel = createTemporaryObject(panelComponent, testCase, {
            "height": 480,
            "session": session,
            "width": 640
        });
        verify(panel !== null);

        const reason = findChild(panel, "arenaSelectReadyDisabledReason");
        verify(reason !== null);
        compare(reason.text, panel.readyDisabledReason);
        compare(reason.visible, true);
        compare(reason.Accessible.name, panel.readyDisabledReason);

        const chatTab = findChild(panel, "arenaSelectChatTab");
        mouseClick(chatTab, chatTab.width / 2, chatTab.height / 2,
                   Qt.LeftButton);
        compare(panel.detailMode, "chat");
        compare(reason.text, panel.readyDisabledReason);
        compare(reason.visible, true);
        compare(reason.Accessible.name, panel.readyDisabledReason);
    }

    function test_unstored_panel_uses_scaled_gap_hint_data() {
        return [
            {
                "tag": "1920x1080",
                "expectedRect": Qt.rect(728, 120, 520, 480),
                "viewportWidth": 1920,
                "viewportHeight": 1080
            },
            {
                "tag": "2560x1440",
                "expectedRect": Qt.rect(970.6666666667, 160,
                                        693.3333333333, 640),
                "viewportWidth": 2560,
                "viewportHeight": 1440
            },
            {
                "tag": "1024x768",
                "expectedRect": Qt.rect(388.2666666667, 160, 520, 320),
                "viewportWidth": 1024,
                "viewportHeight": 768
            }
        ];
    }

    function test_unstored_panel_uses_scaled_gap_hint(data) {
        const session = createSession();
        const themeVars = createTemporaryObject(themeVarsComponent, testCase);
        verify(themeVars !== null);
        themeVars.beginTracking();
        const viewport = createTemporaryObject(scenePanelMountComponent,
                                               testCase, {
            "height": data.viewportHeight,
            "session": session,
            "themeVars": themeVars,
            "width": data.viewportWidth
        });
        verify(viewport !== null);
        session.parent = viewport;
        tryCompare(viewport.panelLoader, "status", Loader.Ready);
        compare(viewport.panelLoader.parent, viewport);
        compare(viewport.panelLoader.x, 0);
        compare(viewport.panelLoader.y, 0);
        compare(viewport.panelLoader.width, viewport.width);
        compare(viewport.panelLoader.height, viewport.height);
        const overlay = viewport.panelLoader.item;
        verify(overlay !== null);
        compare(overlay.defaultPixelRectHint, viewport.scaledGapHint);

        const frame = findChild(overlay, "arenaSelectPlacementFrame");
        verify(frame !== null);
        compare(frame.sourcePlacement.stored, false);
        tryCompare(frame, "width", data.expectedRect.width);
        compareRect(frame.resolvedPixelRect, data.expectedRect,
                    data.tag + " placement");
        compare(themeVars.writeCount, 0);
        tryVerify(function() {
            return findChild(viewport, "arenaRosterKick-member-2") !== null;
        });

        verifyMinimumSceneTarget(findChild(viewport, "arenaSelectDetailsTab"),
                                 viewport, "Details tab");
        verifyMinimumSceneTarget(findChild(viewport, "arenaSelectChatTab"),
                                 viewport, "Chat tab");
        verifyMinimumSceneTarget(findChild(viewport, "arenaSelectReady"),
                                 viewport, "Ready button");
        verifyMinimumSceneTarget(findChild(viewport, "arenaSelectLeave"),
                                 viewport, "Leave button");
        verifyMinimumSceneTarget(findChild(viewport,
                                           "arenaRosterKick-member-2"),
                                 viewport, "Kick button");

        viewport.panelLoader.active = false;
        tryCompare(viewport.panelLoader, "status", Loader.Null);
    }

    function test_ready_reason_survives_chat_and_announcements_are_deduplicated() {
        const session = createSession();
        session.canReady = false;
        const panel = createTemporaryObject(panelComponent, testCase, {
            "height": 480,
            "session": session,
            "width": 640
        });
        verify(panel !== null);
        verify(panel.readyDisabledReason.length > 0);

        const ready = findChild(panel, "arenaSelectReady");
        const chatTab = findChild(panel, "arenaSelectChatTab");
        mouseClick(chatTab, chatTab.width / 2, chatTab.height / 2, Qt.LeftButton);
        compare(panel.detailMode, "chat");
        verify(panel.readyDisabledReason.length > 0);
        compare(ready.Accessible.description, panel.readyDisabledReason);

        session.reconnecting = true;
        tryCompare(panel, "lastAnnouncementKey", "arena.status.reconnecting");
        compare(panel.announcementCount, 1);

        session.errorMessageKey = "arena.error.selectionStale";
        compare(panel.announcementCount, 1);
        session.reconnecting = false;
        tryCompare(panel, "lastAnnouncementKey", "arena.status.selectionInvalidated");
        compare(panel.announcementCount, 2);

        session.errorMessageKey = "arena.error.availabilityStale";
        compare(panel.announcementCount, 2);
        session.errorMessageKey = "arena.error.parseFailed";
        compare(panel.announcementCount, 2);
        session.roundLaunchCancellationStatusKey = "arena.status.roundLaunchCancelled.parseFailed";
        tryCompare(panel, "lastAnnouncementKey", "arena.status.roundLaunchCancelled.parseFailed");
        compare(panel.announcementCount, 3);
        verify(panel.lastAnnouncementText.length > 0);
        const lastText = panel.lastAnnouncementText;
        session.roundLaunchCancellationStatusKey = "";
        compare(panel.lastAnnouncementKey, "arena.status.roundLaunchCancelled.parseFailed");
        compare(panel.lastAnnouncementText, lastText);
        compare(panel.announcementCount, 3);
        session.roundLaunchCancellationStatusKey = "arena.status.roundLaunchCancelled.parseFailed";
        tryCompare(panel, "announcementCount", 4);
    }

    function test_select_strip_exposes_ready_reason_and_announces_selection_failure() {
        const session = createSession();
        session.canReady = false;
        const strip = createTemporaryObject(stripComponent, testCase, {
            "session": session,
            "width": 800
        });
        verify(strip !== null);
        verify(strip.readyDisabledReason.length > 0);

        session.errorMessageKey = "arena.error.resourceFailed";
        compare(strip.announcementCount, 0);
        session.roundLaunchCancellationStatusKey = "arena.status.roundLaunchCancelled.resourceFailed";
        tryCompare(strip, "lastAnnouncementKey", "arena.status.roundLaunchCancelled.resourceFailed");
        compare(strip.announcementCount, 1);
        session.errorMessageKey = "arena.error.hashMismatch";
        compare(strip.announcementCount, 1);
        session.roundLaunchCancellationStatusKey = "";
        compare(strip.announcementCount, 1);
        session.roundLaunchCancellationStatusKey = "arena.status.roundLaunchCancelled.resourceFailed";
        tryCompare(strip, "announcementCount", 2);
    }
}

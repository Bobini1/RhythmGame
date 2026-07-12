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
        id: tablesComponent

        QtObject {
            property var matches: []
            property string searchedHash: ""

            signal dataChanged

            function search(hash) {
                searchedHash = String(hash || "");
                return matches;
            }
        }
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
            readonly property real contentScale: Math.min(width / 1920, height / 1080)
            readonly property real contentLeft: (width - 1920 * contentScale) / 2
            readonly property real contentTop: (height - 1080 * contentScale) / 2
            readonly property rect scaledGapHint: Qt.rect(contentLeft + 728 * contentScale, contentTop + 120 * contentScale, 520 * contentScale, 480 * contentScale)

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
        session.selectedByDisplayName = "Bobini";
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
        const bottomRight = target.mapToItem(viewport, target.width, target.height);
        verify(Math.abs(bottomRight.x - topLeft.x) >= 32, label + " width");
        verify(Math.abs(bottomRight.y - topLeft.y) >= 32, label + " height");
    }

    function closeEnough(actual, expected, label, epsilon = 0.01) {
        verify(Math.abs(actual - expected) <= epsilon, label + ": expected " + actual + " to be within " + epsilon + " of " + expected);
    }

    function compareRect(actual, expected, label) {
        closeEnough(actual.x, expected.x, label + " x");
        closeEnough(actual.y, expected.y, label + " y");
        closeEnough(actual.width, expected.width, label + " width");
        closeEnough(actual.height, expected.height, label + " height");
    }

    function verifyItemInside(item, frame, viewport, label) {
        verify(item !== null, label + " exists");
        verify(item.visible, label + " is visible");
        verify(item.width > 0, label + " has positive width: " + item.width);
        verify(item.height > 0, label + " has positive height: " + item.height);

        const frameTopLeft = item.mapToItem(frame, 0, 0);
        const frameBottomRight = item.mapToItem(frame, item.width, item.height);
        verify(frameTopLeft.x >= -0.01, label + " starts inside frame x: " + frameTopLeft.x);
        verify(frameTopLeft.y >= -0.01, label + " starts inside frame y: " + frameTopLeft.y);
        verify(frameBottomRight.x <= frame.width + 0.01, label + " ends inside frame x: " + frameBottomRight.x + " <= " + frame.width);
        verify(frameBottomRight.y <= frame.height + 0.01, label + " ends inside frame y: " + frameBottomRight.y + " <= " + frame.height);

        const viewportTopLeft = item.mapToItem(viewport, 0, 0);
        const viewportBottomRight = item.mapToItem(viewport, item.width, item.height);
        verify(viewportTopLeft.x >= -0.01, label + " starts inside viewport x");
        verify(viewportTopLeft.y >= -0.01, label + " starts inside viewport y");
        verify(viewportBottomRight.x <= viewport.width + 0.01, label + " ends inside viewport x: " + viewportBottomRight.x + " <= " + viewport.width);
        verify(viewportBottomRight.y <= viewport.height + 0.01, label + " ends inside viewport y: " + viewportBottomRight.y + " <= " + viewport.height);
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
        const avatar = findChild(roster, "arenaRosterAvatar-member-2");
        verify(avatar !== null);
        compare(avatar.width, 32);
        compare(avatar.height, 32);
        const fallback = findChild(avatar, "arenaAvatarFallback");
        verify(fallback !== null);
        compare(fallback.text, "P");
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
        compare(selector.text, "Selected by Bobini");
        verify(selector.text.indexOf("member-2") < 0);
        session.selectedByDisplayName = "";
        compare(selector.text, "Selected by another player");
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

    function test_selection_summary_uses_local_table_level_and_falls_back_to_metadata() {
        const session = createSession();
        session.selectedMd5 = "0123456789abcdef0123456789abcdef";
        session.selectedSubtitle = "<i>Subtitle</i>";
        const tables = createTemporaryObject(tablesComponent, testCase, {
            "matches": [{
                "levelName": "12",
                "symbol": "★"
            }]
        });
        verify(tables !== null);
        const summary = createTemporaryObject(summaryComponent, testCase, {
            "session": session,
            "tables": tables,
            "width": 500
        });
        verify(summary !== null);

        const title = findChild(summary, "arenaSelectionTitle");
        verify(title !== null);
        compare(tables.searchedHash, session.selectedMd5);
        compare(title.text, "★12 <b>Selected chart</b> <i>Subtitle</i>");
        compare(title.textFormat, Text.PlainText);

        tables.matches = [];
        tables.dataChanged();
        tryCompare(title, "text", "<b>Selected chart</b> <i>Subtitle</i>");
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
        verify(details !== null);
        verify(chat !== null);
        verify(roster !== null);
        verify(selection !== null);
        verify(ready !== null);
        compare(findChild(panel, "arenaSelectLeave"), null);
        compare(findChild(panel, "arenaLegacyExpand"), null);

        compare(details.checked, true);
        compare(chat.checked, false);
        compare(panel.detailMode, "details");
        verify(findChild(panel, "arenaSelectSelection") !== null);
        compare(findChild(panel, "arenaSelectChat"), null);
        mouseClick(details, details.width / 2, details.height / 2, Qt.LeftButton);
        compare(details.checked, true);
        compare(chat.checked, false);
        compare(panel.detailMode, "details");
        verify(findChild(panel, "arenaSelectSelection") !== null);
        compare(findChild(panel, "arenaSelectChat"), null);

        details.forceActiveFocus();
        keyClick(Qt.Key_Space);
        compare(details.checked, true);
        compare(chat.checked, false);
        compare(panel.detailMode, "details");
        verify(findChild(panel, "arenaSelectSelection") !== null);
        compare(findChild(panel, "arenaSelectChat"), null);

        mouseClick(ready, ready.width / 2, ready.height / 2, Qt.LeftButton);
        compare(session.readyRequests, [true]);

        tryVerify(function () {
            return findChild(panel, "arenaRosterKick-member-2") !== null;
        });
        const kick = findChild(panel, "arenaRosterKick-member-2");
        mouseClick(kick, kick.width / 2, kick.height / 2, Qt.LeftButton);
        compare(session.kickedMemberIds, ["member-2"]);

        mouseClick(chat, chat.width / 2, chat.height / 2, Qt.LeftButton);
        compare(details.checked, false);
        compare(chat.checked, true);
        compare(panel.detailMode, "chat");
        tryVerify(function () {
            return findChild(panel, "arenaSelectChat") !== null && findChild(panel, "arenaSelectSelection") === null;
        });
        mouseClick(chat, chat.width / 2, chat.height / 2, Qt.LeftButton);
        compare(details.checked, false);
        compare(chat.checked, true);
        compare(panel.detailMode, "chat");
        verify(findChild(panel, "arenaSelectChat") !== null);
        compare(findChild(panel, "arenaSelectSelection"), null);
        chat.forceActiveFocus();
        keyClick(Qt.Key_Space);
        compare(details.checked, false);
        compare(chat.checked, true);
        compare(panel.detailMode, "chat");
        verify(findChild(panel, "arenaSelectChat") !== null);
        compare(findChild(panel, "arenaSelectSelection"), null);
        const chatView = findChild(panel, "arenaSelectChat");
        verify(chatView !== null);
        const chatInput = findChild(chatView, "arenaChatInput");
        verify(chatInput !== null);
        chatInput.forceActiveFocus();
        chatInput.text = "default hello";
        keyClick(Qt.Key_Return);
        compare(session.sentMessages, ["default hello"]);
    }

    function test_panel_header_and_body_stay_at_top_across_modes() {
        const session = createSession();
        const panel = createTemporaryObject(panelComponent, testCase, {
            "height": 480,
            "session": session,
            "width": 640
        });
        verify(panel !== null);
        const header = findChild(panel, "arenaSelectHeader");
        const title = findChild(panel, "arenaSelectRoomTitle");
        const details = findChild(panel, "arenaSelectDetailsTab");
        const chat = findChild(panel, "arenaSelectChatTab");
        const roster = findChild(panel, "arenaSelectRoster");
        const summary = findChild(panel, "arenaSelectSelection");
        verify(header !== null);
        verify(title !== null);
        verify(details !== null);
        verify(chat !== null);
        verify(roster !== null);
        verify(summary !== null);
        const headerY = header.mapToItem(panel, 0, 0).y;
        const detailsY = details.mapToItem(panel, 0, 0).y;
        const chatY = chat.mapToItem(panel, 0, 0).y;
        const rosterY = roster.mapToItem(panel, 0, 0).y;
        const summaryY = summary.mapToItem(panel, 0, 0).y;
        compare(headerY, 10);
        verify(title.width >= 80);
        closeEnough(chat.x, details.x + details.width, "tabs remain adjacent", 1);

        mouseClick(chat, chat.width / 2, chat.height / 2, Qt.LeftButton);
        const chatView = findChild(panel, "arenaSelectChat");
        verify(chatView !== null);
        compare(header.mapToItem(panel, 0, 0).y, headerY);
        compare(details.mapToItem(panel, 0, 0).y, detailsY);
        compare(chat.mapToItem(panel, 0, 0).y, chatY);
        compare(roster.mapToItem(panel, 0, 0).y, rosterY);
        compare(chatView.mapToItem(panel, 0, 0).y, summaryY);
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
        mouseClick(chatTab, chatTab.width / 2, chatTab.height / 2, Qt.LeftButton);
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
                "expectedRect": Qt.rect(970.6666666667, 160, 693.3333333333, 640),
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
        const viewport = createTemporaryObject(scenePanelMountComponent, testCase, {
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
        compareRect(frame.resolvedPixelRect, data.expectedRect, data.tag + " placement");
        compare(themeVars.writeCount, 0);
        tryVerify(function () {
            return findChild(viewport, "arenaRosterKick-member-2") !== null;
        });

        verifyMinimumSceneTarget(findChild(viewport, "arenaSelectDetailsTab"), viewport, "Details tab");
        verifyMinimumSceneTarget(findChild(viewport, "arenaSelectChatTab"), viewport, "Chat tab");
        verifyMinimumSceneTarget(findChild(viewport, "arenaSelectReady"), viewport, "Ready button");
        compare(findChild(viewport, "arenaSelectLeave"), null);
        verifyMinimumSceneTarget(findChild(viewport, "arenaRosterKick-member-2"), viewport, "Kick button");

        viewport.panelLoader.active = false;
        tryCompare(viewport.panelLoader, "status", Loader.Null);
    }

    function test_overlay_keeps_content_inside_narrow_viewports_data() {
        return [
            {
                "tag": "500x400",
                "viewportWidth": 500,
                "viewportHeight": 400
            },
            {
                "tag": "320x240",
                "viewportWidth": 320,
                "viewportHeight": 240
            }
        ];
    }

    function test_overlay_keeps_content_inside_narrow_viewports(data) {
        const session = createSession();
        const themeVars = createTemporaryObject(themeVarsComponent, testCase);
        verify(themeVars !== null);
        const viewport = createTemporaryObject(scenePanelMountComponent, testCase, {
            "height": data.viewportHeight,
            "session": session,
            "themeVars": themeVars,
            "width": data.viewportWidth
        });
        verify(viewport !== null);
        session.parent = viewport;
        tryCompare(viewport.panelLoader, "status", Loader.Ready);

        const overlay = viewport.panelLoader.item;
        const frame = findChild(overlay, "arenaSelectPlacementFrame");
        verify(frame !== null);
        const details = findChild(frame, "arenaSelectDetailsTab");
        const chat = findChild(frame, "arenaSelectChatTab");
        const ready = findChild(frame, "arenaSelectReady");
        const roster = findChild(frame, "arenaSelectRoster");
        const selection = findChild(frame, "arenaSelectSelection");
        tryVerify(function () {
            return roster !== null && selection !== null && roster.height > 0 && selection.height > 0;
        });

        const frameTopLeft = frame.mapToItem(viewport, 0, 0);
        const frameBottomRight = frame.mapToItem(viewport, frame.width, frame.height);
        verify(frameTopLeft.x >= -0.01, "frame starts inside viewport x");
        verify(frameTopLeft.y >= -0.01, "frame starts inside viewport y");
        verify(frameBottomRight.x <= viewport.width + 0.01, "frame ends inside viewport x");
        verify(frameBottomRight.y <= viewport.height + 0.01, "frame ends inside viewport y");

        const detailsItems = [details, chat, ready, roster, selection];
        const detailsLabels = ["Details tab", "Chat tab", "Ready button", "Roster", "Summary"];
        for (let index = 0; index < detailsItems.length; ++index) {
            verifyItemInside(detailsItems[index], frame, viewport, data.tag + " " + detailsLabels[index]);
        }

        const rosterTopLeft = roster.mapToItem(frame, 0, 0);
        const summaryTopLeft = selection.mapToItem(frame, 0, 0);
        verify(summaryTopLeft.y >= rosterTopLeft.y + roster.height - 0.01, data.tag + " summary stacks below roster");
        closeEnough(summaryTopLeft.x, rosterTopLeft.x, data.tag + " summary aligns with roster x");
        closeEnough(selection.width, roster.width, data.tag + " summary matches roster width");

        mouseClick(chat, chat.width / 2, chat.height / 2, Qt.LeftButton);
        tryVerify(function () {
            const item = findChild(frame, "arenaSelectChat");
            return item !== null && item.height > 0;
        });
        const chatView = findChild(frame, "arenaSelectChat");
        const chatItems = [details, chat, ready, roster, chatView];
        const chatLabels = ["Details tab", "Chat tab", "Ready button", "Roster", "Chat surface"];
        for (let index = 0; index < chatItems.length; ++index) {
            verifyItemInside(chatItems[index], frame, viewport, data.tag + " " + chatLabels[index]);
        }

        const chatTopLeft = chatView.mapToItem(frame, 0, 0);
        verify(chatTopLeft.y >= rosterTopLeft.y + roster.height - 0.01, data.tag + " chat stacks below roster");
        closeEnough(chatTopLeft.x, rosterTopLeft.x, data.tag + " chat aligns with roster x");
        closeEnough(chatView.width, roster.width, data.tag + " chat matches roster width");
    }

    function test_overlay_keeps_wide_body_side_by_side() {
        const session = createSession();
        const themeVars = createTemporaryObject(themeVarsComponent, testCase);
        const viewport = createTemporaryObject(scenePanelMountComponent, testCase, {
            "height": 1080,
            "session": session,
            "themeVars": themeVars,
            "width": 1920
        });
        verify(viewport !== null);
        session.parent = viewport;
        tryCompare(viewport.panelLoader, "status", Loader.Ready);

        const frame = findChild(viewport.panelLoader.item, "arenaSelectPlacementFrame");
        verify(frame !== null);
        const roster = findChild(frame, "arenaSelectRoster");
        const selection = findChild(frame, "arenaSelectSelection");
        verify(roster !== null);
        verify(selection !== null);

        tryVerify(function () {
            const rosterPosition = roster.mapToItem(frame, 0, 0);
            const selectionPosition = selection.mapToItem(frame, 0, 0);
            return selectionPosition.x >= rosterPosition.x + roster.width - 0.01;
        });

        const rosterTopLeft = roster.mapToItem(frame, 0, 0);
        const selectionTopLeft = selection.mapToItem(frame, 0, 0);
        verify(selectionTopLeft.x >= rosterTopLeft.x + roster.width - 0.01, "wide summary remains beside roster: summary x " + selectionTopLeft.x + ", roster right " + (rosterTopLeft.x + roster.width) + ", columns " + roster.parent.columns);
        verify(selectionTopLeft.y < rosterTopLeft.y + roster.height, "wide summary overlaps roster vertically");
        closeEnough(selectionTopLeft.y, rosterTopLeft.y, "wide summary aligns with roster y");
        closeEnough(selection.height, roster.height, "wide summary matches roster height");
    }

    function test_hidden_resize_does_not_steal_edge_content_input() {
        const session = createSession();
        const themeVars = createTemporaryObject(themeVarsComponent, testCase);
        const viewport = createTemporaryObject(scenePanelMountComponent, testCase, {
            "height": 1080,
            "session": session,
            "themeVars": themeVars,
            "width": 1920
        });
        verify(viewport !== null);
        session.parent = viewport;
        tryCompare(viewport.panelLoader, "status", Loader.Ready);

        const overlay = viewport.panelLoader.item;
        const frame = findChild(overlay, "arenaSelectPlacementFrame");
        verify(frame !== null);
        const title = overlay.panel.dragHandle;
        const chat = findChild(frame, "arenaSelectChatTab");
        const ready = findChild(frame, "arenaSelectReady");
        const topLeft = findChild(frame, "arenaResizeTopLeft");
        const topRight = findChild(frame, "arenaResizeTopRight");
        const bottomRight = findChild(frame, "arenaResizeBottomRight");
        verify(title !== null);
        verify(chat !== null);
        verify(ready !== null);
        verify(topLeft !== null);
        verify(topRight !== null);
        verify(bottomRight !== null);

        compare(topLeft.width, 16);
        compare(topLeft.height, 16);
        compare(topRight.width, 16);
        compare(bottomRight.height, 16);
        const titleTopLeft = title.mapToItem(frame, 0, 0);
        const chatBottomRight = chat.mapToItem(frame, chat.width, chat.height);
        const readyBottomRight = ready.mapToItem(frame, ready.width, ready.height);
        verify(titleTopLeft.x >= topLeft.x + topLeft.width, "title starts outside left resize zone");
        verify(titleTopLeft.y >= topLeft.y + topLeft.height, "title starts outside top resize zone");
        verify(chatBottomRight.x <= topRight.x, "tab ends outside right resize zone");
        verify(readyBottomRight.x <= bottomRight.x, "Ready ends outside right resize zone");
        verify(readyBottomRight.y <= bottomRight.y, "Ready ends outside bottom resize zone");

        frame.placementCommitted.connect(function () {
            themeVars.commitCount += 1;
        });
        themeVars.beginTracking();
        const initialRect = Qt.rect(frame.x, frame.y, frame.width, frame.height);
        mousePress(title, 1, 1, Qt.LeftButton);
        mouseMove(title, 21, 11, 10);
        mouseMove(title, 61, 31, 10);
        mouseRelease(title, 61, 31, Qt.LeftButton);
        wait(1);
        verify(frame.x > initialRect.x, "edge title drag moves panel x");
        verify(frame.y > initialRect.y, "edge title drag moves panel y");
        compare(themeVars.commitCount, 1);

        mouseClick(chat, chat.width - 1, 1, Qt.LeftButton);
        compare(chat.checked, true);
        compare(overlay.panel.detailMode, "chat");
        mouseClick(ready, ready.width - 1, ready.height - 1, Qt.LeftButton);
        compare(session.readyRequests, [true]);
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

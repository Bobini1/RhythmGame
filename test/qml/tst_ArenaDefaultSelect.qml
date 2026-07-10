import QtQuick
import QtTest
import RhythmGameQml
import "../../share/RhythmGame/themes/Default/scripts/select" as DefaultSelect

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

        DefaultSelect.ArenaSelectPanel {}
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

    function test_default_panel_keeps_roster_and_routes_actions() {
        const session = createSession();
        const panel = createTemporaryObject(panelComponent, testCase, {
            "height": 480,
            "session": session,
            "width": 640
        });
        verify(panel !== null);
        compare(panel.arenaNativeSelectPresentation, true);
        verify(findChild(panel, "arenaDefaultRoster") !== null);
        verify(findChild(panel, "arenaDefaultSelection") !== null);

        const ready = findChild(panel, "arenaDefaultReady");
        mouseClick(ready, ready.width / 2, ready.height / 2, Qt.LeftButton);
        compare(session.readyRequests, [true]);

        const chatTab = findChild(panel, "arenaDefaultChatTab");
        mouseClick(chatTab, chatTab.width / 2, chatTab.height / 2, Qt.LeftButton);
        compare(panel.detailMode, "chat");
        verify(findChild(panel, "arenaDefaultChat") !== null);

        const leave = findChild(panel, "arenaDefaultLeave");
        mouseClick(leave, leave.width / 2, leave.height / 2, Qt.LeftButton);
        compare(session.leaveCount, 1);
    }
}

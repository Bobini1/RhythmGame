import QtQuick
import QtTest
import RhythmGameQml

TestCase {
    id: testCase

    name: "ArenaChatNavigation"
    when: windowShown
    width: 800
    height: 600
    visible: true

    Component {
        id: fakeSessionComponent

        FakeArenaSession {}
    }

    Component {
        id: roomChatComponent

        ArenaChatView {}
    }

    Component {
        id: gameplayChatComponent

        ArenaGameplayChat {}
    }

    function appendMessage(session, index) {
        session.chat.append({
            "messageId": "message-" + index,
            "memberId": "member-" + ((index % 4) + 1),
            "displayName": "Player " + index,
            "text": "Message " + index,
            "timestamp": index,
            "self": index % 4 === 0
        });
    }

    function populateChat(session, count = 120) {
        for (let index = 0; index < count; ++index) {
            appendMessage(session, index);
        }
    }

    function test_room_chat_keyboard_review_survives_long_list_insertions() {
        const session = createTemporaryObject(fakeSessionComponent, testCase);
        verify(session !== null);
        populateChat(session);
        const chat = createTemporaryObject(roomChatComponent, testCase, {
            "chatModel": session.chat,
            "height": 180,
            "session": session,
            "width": 420
        });
        verify(chat !== null);
        const chatList = findChild(chat, "arenaChatList");
        verify(chatList !== null);
        tryCompare(chat, "messageCount", 120);
        tryCompare(chatList, "currentIndex", 119);
        tryVerify(() => chat.atTail);

        chatList.forceActiveFocus();
        tryCompare(chatList, "activeFocus", true);
        tryVerify(function() {
            const newest = findChild(chat, "arenaChatMessage-message-119");
            return newest !== null && newest.border.width === 2;
        });

        keyClick(Qt.Key_Home);
        tryCompare(chatList, "currentIndex", 0);
        compare(chat.followTail, false);
        tryVerify(function() {
            const reviewed = findChild(chat, "arenaChatMessage-message-0");
            return reviewed !== null && reviewed.border.width === 2;
        });

        chatList.positionViewAtEnd();
        chatList.movementEnded();
        tryCompare(chat, "followTail", true);
        tryCompare(chatList, "currentIndex", 119);
        tryVerify(function() {
            const newest = findChild(chat, "arenaChatMessage-message-119");
            return newest !== null && newest.border.width === 2;
        });

        keyClick(Qt.Key_Home);
        tryCompare(chatList, "currentIndex", 0);
        compare(chat.followTail, false);
        appendMessage(session, 120);
        tryCompare(chat, "messageCount", 121);
        compare(chatList.currentIndex, 0);
        compare(chat.followTail, false);
        compare(chat.atTail, false);
        tryVerify(function() {
            const reviewed = findChild(chat, "arenaChatMessage-message-0");
            return reviewed !== null && reviewed.border.width === 2;
        });

        keyClick(Qt.Key_End);
        tryCompare(chat, "followTail", true);
        tryCompare(chatList, "currentIndex", 120);
        tryVerify(() => chat.atTail);
        appendMessage(session, 121);
        tryCompare(chat, "messageCount", 122);
        tryCompare(chatList, "currentIndex", 121);
        tryVerify(() => chat.atTail);
        tryVerify(function() {
            const newest = findChild(chat, "arenaChatMessage-message-121");
            return newest !== null && newest.border.width === 2;
        });
    }

    function test_gameplay_chat_keyboard_review_survives_long_list_insertions() {
        const session = createTemporaryObject(fakeSessionComponent, testCase);
        verify(session !== null);
        populateChat(session);
        const chat = createTemporaryObject(gameplayChatComponent, testCase, {
            "height": 180,
            "session": session,
            "width": 420
        });
        verify(chat !== null);
        const chatList = findChild(chat, "arenaGameplayChatList");
        verify(chatList !== null);
        tryCompare(chatList, "count", 120);
        tryCompare(chatList, "currentIndex", 119);
        tryVerify(() => chatList.atYEnd);

        chatList.forceActiveFocus();
        tryCompare(chatList, "activeFocus", true);
        tryVerify(function() {
            const newest = findChild(chat, "arenaGameplayChatRow119");
            return newest !== null && newest.focusIndicatorVisible;
        });

        keyClick(Qt.Key_Home);
        tryCompare(chatList, "currentIndex", 0);
        compare(chatList.followTail, false);
        tryVerify(function() {
            const reviewed = findChild(chat, "arenaGameplayChatRow0");
            return reviewed !== null && reviewed.focusIndicatorVisible;
        });

        chatList.positionViewAtEnd();
        chatList.movementEnded();
        tryCompare(chatList, "followTail", true);
        tryCompare(chatList, "currentIndex", 119);
        tryVerify(function() {
            const newest = findChild(chat, "arenaGameplayChatRow119");
            return newest !== null && newest.focusIndicatorVisible;
        });

        keyClick(Qt.Key_Home);
        tryCompare(chatList, "currentIndex", 0);
        compare(chatList.followTail, false);
        appendMessage(session, 120);
        tryCompare(chatList, "count", 121);
        compare(chatList.currentIndex, 0);
        compare(chatList.followTail, false);
        compare(chatList.atYEnd, false);
        tryVerify(function() {
            const reviewed = findChild(chat, "arenaGameplayChatRow0");
            return reviewed !== null && reviewed.focusIndicatorVisible;
        });

        keyClick(Qt.Key_End);
        tryCompare(chatList, "followTail", true);
        tryCompare(chatList, "currentIndex", 120);
        tryVerify(() => chatList.atYEnd);
        appendMessage(session, 121);
        tryCompare(chatList, "count", 122);
        tryCompare(chatList, "currentIndex", 121);
        tryVerify(() => chatList.atYEnd);
        tryVerify(function() {
            const newest = findChild(chat, "arenaGameplayChatRow121");
            return newest !== null && newest.focusIndicatorVisible;
        });
    }
}

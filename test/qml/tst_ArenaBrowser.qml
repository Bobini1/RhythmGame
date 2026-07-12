pragma ComponentBehavior: Bound

import QtQuick
import QtTest
import RhythmGameQml

TestCase {
    id: testCase

    name: "ArenaBrowser"
    when: windowShown
    width: 600
    height: 400
    visible: true

    Component {
        id: memberStackComponent

        ArenaRoomMemberStack {}
    }

    function test_room_preview_shows_first_four_members_and_overflow() {
        const members = [];
        for (let index = 0; index < 6; ++index) {
            members.push({
                "avatarUrl": "",
                "connected": index !== 5,
                "displayName": index === 0 ? "Alice" : "Player " + (index + 1)
            });
        }
        const stack = createTemporaryObject(memberStackComponent, testCase, {
            "members": members,
            "objectNamePrefix": "arenaRoomAvatar-room-1-"
        });
        verify(stack !== null);

        for (let index = 0; index < 4; ++index) {
            const avatar = findChild(stack, "arenaRoomAvatar-room-1-" + index);
            verify(avatar !== null);
            compare(avatar.width, 24);
            compare(avatar.height, 24);
        }
        compare(findChild(stack, "arenaRoomAvatar-room-1-4"), null);
        const overflow = findChild(stack, "arenaRoomAvatar-room-1-overflow");
        verify(overflow !== null);
        compare(overflow.text, "+2");
        compare(stack.memberNames, "Alice, Player 2, Player 3, Player 4, Player 5, Player 6");

        const firstAvatar = findChild(stack, "arenaRoomAvatar-room-1-0");
        const fallback = findChild(firstAvatar, "arenaAvatarFallback");
        verify(fallback !== null);
        compare(fallback.text, "A");
    }
}

import QtQuick

Item {
    id: root

    property string roomName: "Test room"
    property string selfMemberId: "member-1"
    property string ownerMemberId: "member-1"
    property bool isOwner: true
    property bool roundsAvailable: true
    property bool availabilitySyncing: false
    property bool reconnecting: false
    property bool canReady: true
    property bool ready: false
    property string selectedTitle: ""
    property string selectedByMemberId: ""
    property string currentRoundId: ""
    property int roomPhase: 0
    property string errorMessageKey: ""
    property string arenaOptionsSummary: "MIRROR · FLIP"
    property bool arenaGameplayActive: false
    property var arenaRunner: null
    property bool gameplayChatOpen: false
    property alias members: memberModel
    property alias chat: chatModel
    property alias lastResult: resultState
    property var kickedMemberIds: []
    property var sentMessages: []
    property var readyRequests: []
    property int leaveCount: 0

    function kickMember(memberId) {
        const next = root.kickedMemberIds.slice();
        next.push(memberId);
        root.kickedMemberIds = next;
    }

    function sendChat(text) {
        const next = root.sentMessages.slice();
        next.push(text);
        root.sentMessages = next;
    }

    function setReady(value) {
        const next = root.readyRequests.slice();
        next.push(value);
        root.readyRequests = next;
        root.ready = value;
    }

    function leaveRoom() {
        root.leaveCount += 1;
    }

    ListModel {
        id: memberModel
    }

    ListModel {
        id: chatModel
    }

    QtObject {
        id: resultState

        property bool valid: false
        property bool finalized: false
        property int participantCount: 0
        property var winnerMemberIds: []
        property var winnerNames: []
        property int localRank: 0
        property bool localDnf: false
        property string selectionTitle: ""
        property string selectionOptionsSummary: ""
    }
}

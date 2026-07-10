import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml

FocusScope {
    id: root

    required property ArenaSession session

    signal leaveRequested()
    signal kickRequested(string memberId)
    signal chatRequested(string text)
    signal retryRequested()
    signal exitRequested()

    property string kickMemberId: ""
    property string kickDisplayName: ""
    property Item kickOrigin: null
    readonly property alias announcementCount: statusAnnouncer.announcementCount
    readonly property alias lastAnnouncementKey: statusAnnouncer.lastAnnouncementKey
    readonly property alias lastAnnouncementText: statusAnnouncer.lastAnnouncementText
    readonly property string moderationDisabledReason: session.state === ArenaSession.Reconnecting
        ? qsTr("Unavailable while reconnecting to Arena.")
        : ""
    readonly property bool mutableRoom: session.state === ArenaSession.InRoom

    Accessible.name: root.session.roomName || qsTr("Arena room")
    Accessible.role: Accessible.Grouping

    function errorText(key) : string {
        switch (key) {
        case "arena.error.notInRoom":
            return qsTr("You are no longer in this room.");
        case "arena.error.permissionDenied":
            return qsTr("Only the room owner can do that.");
        case "arena.error.targetNotFound":
            return qsTr("That player is no longer in the room.");
        case "arena.error.cannotKickSelf":
            return qsTr("You cannot remove yourself with the kick action.");
        case "arena.error.chatEmpty":
            return qsTr("Enter a chat message first.");
        case "arena.error.chatTooLong":
            return qsTr("That chat message is too long.");
        case "arena.error.rateLimited":
            return qsTr("You are sending messages too quickly.");
        case "arena.error.roomGenerationStale":
        case "arena.error.connectionGenerationStale":
            return qsTr("Room state changed. Please try again.");
        case "arena.error.tlsFailed":
            return qsTr("A secure connection to Arena could not be established.");
        case "arena.error.connectionFailed":
        case "arena.error.remoteClosed":
        case "arena.error.transport":
            return qsTr("The Arena connection was interrupted.");
        default:
            return qsTr("Arena request failed. Please try again.");
        }
    }

    function submitChat() : void {
        if (!mutableRoom || chatField.text.trim().length === 0) {
            return;
        }
        const message = chatField.text;
        chatField.clear();
        chatRequested(message);
    }

    function openKickDialog(memberId, displayName, origin) : void {
        kickMemberId = memberId;
        kickDisplayName = displayName;
        kickOrigin = origin;
        kickDialogLoader.active = true;
    }

    function finishKickDialog() : void {
        const origin = kickOrigin;
        Qt.callLater(function() {
            kickDialogLoader.active = false;
            kickMemberId = "";
            kickDisplayName = "";
            kickOrigin = null;
            if (origin && origin.enabled && origin.visible) {
                origin.forceActiveFocus();
            } else {
                leaveButton.forceActiveFocus();
            }
        });
    }

    Component.onCompleted: leaveButton.forceActiveFocus()

    Keys.onEscapePressed: event => {
        if (kickDialogLoader.status === Loader.Ready && kickDialogLoader.item) {
            kickDialogLoader.item.close();
        } else {
            root.leaveRequested();
        }
        event.accepted = true;
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            Button {
                id: leaveButton

                text: qsTr("Leave room")
                onClicked: root.leaveRequested()
            }

            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.pixelSize: 26
                horizontalAlignment: Text.AlignHCenter
                text: root.session.roomName
                textFormat: Text.PlainText
            }

            Button {
                text: qsTr("Exit Arena")
                onClicked: root.exitRequested()
            }
        }

        Frame {
            id: statusBanner

            Layout.fillWidth: true
            implicitHeight: statusRow.implicitHeight + topPadding + bottomPadding
            visible: root.session.state === ArenaSession.Reconnecting
                || root.session.errorMessageKey.length > 0

            RowLayout {
                id: statusRow

                anchors.fill: parent
                spacing: 8

                BusyIndicator {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: 28
                    running: visible && root.visible
                    visible: root.session.state === ArenaSession.Reconnecting
                }

                Label {
                    Layout.fillWidth: true
                    text: root.session.state === ArenaSession.Reconnecting
                        ? qsTr("Reconnecting... Your seat is reserved for up to 60 seconds.")
                        : root.errorText(root.session.errorMessageKey)
                    wrapMode: Text.Wrap
                }

                Button {
                    visible: root.session.state === ArenaSession.Reconnecting
                    text: qsTr("Retry")
                    onClicked: root.retryRequested()
                }
            }
        }

        GridLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            columns: width >= 860 ? 2 : 1
            columnSpacing: 12
            rowSpacing: 12

            Frame {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumHeight: 220

                ColumnLayout {
                    anchors.fill: parent

                    Label {
                        Layout.fillWidth: true
                        font.bold: true
                        text: qsTr("Players")
                    }

                    ListView {
                        id: memberList

                        objectName: "arenaRoomMemberList"
                        Accessible.name: qsTr("Arena players")
                        Accessible.role: Accessible.List
                        activeFocusOnTab: true
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        clip: true
                        model: root.session.members
                        reuseItems: true
                        keyNavigationEnabled: true
                        spacing: 6

                        function ensureCurrentItem(): void {
                            if (memberList.count === 0) {
                                memberList.currentIndex = -1;
                            } else if (memberList.currentIndex < 0
                                       || memberList.currentIndex >= memberList.count) {
                                memberList.currentIndex = 0;
                            }
                        }

                        Component.onCompleted: memberList.ensureCurrentItem()
                        onCountChanged: memberList.ensureCurrentItem()

                        ScrollBar.vertical: ScrollBar {}

                        delegate: Rectangle {
                            id: memberDelegate

                            required property string memberId
                            required property string displayName
                            required property string avatarUrl
                            required property bool connected
                            required property bool owner
                            required property bool self
                            required property int lobbyWins

                            objectName: "arenaRoomMember-" + memberDelegate.memberId
                            Accessible.description: qsTr("%1. %2")
                                .arg(memberStatus.text)
                                .arg(qsTr("%n win(s)", "Arena lobby wins", memberDelegate.lobbyWins))
                            Accessible.name: memberName.text
                            Accessible.role: Accessible.ListItem
                            border.color: ListView.isCurrentItem && ListView.view.activeFocus ? "#2387d9" : "transparent"
                            border.width: ListView.isCurrentItem && ListView.view.activeFocus ? 2 : 0
                            color: "transparent"
                            height: memberRow.implicitHeight + 8
                            radius: 3
                            width: ListView.view.width

                            RowLayout {
                                id: memberRow

                                anchors.fill: parent
                                spacing: 8

                                Item {
                                    Layout.preferredHeight: 40
                                    Layout.preferredWidth: 40

                                    Image {
                                        id: avatar

                                        Accessible.ignored: true
                                        anchors.fill: parent
                                        asynchronous: true
                                        fillMode: Image.PreserveAspectCrop
                                        source: memberDelegate.avatarUrl
                                        sourceSize.height: 40
                                        sourceSize.width: 40
                                    }

                                    Label {
                                        Accessible.ignored: true
                                        anchors.centerIn: parent
                                        font.bold: true
                                        text: "?"
                                        visible: memberDelegate.avatarUrl.length === 0
                                            || avatar.status === Image.Error
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0

                                    Label {
                                        id: memberName

                                        Accessible.ignored: true
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                        text: memberDelegate.self
                                            ? qsTr("%1 (you)").arg(memberDelegate.displayName)
                                            : memberDelegate.displayName
                                        textFormat: Text.PlainText
                                    }

                                    Label {
                                        id: memberStatus

                                        Accessible.ignored: true
                                        Layout.fillWidth: true
                                        text: {
                                            const state = memberDelegate.connected
                                                ? qsTr("Connected")
                                                : qsTr("Reserved (reconnecting)");
                                            return memberDelegate.owner
                                                ? qsTr("Owner · %1").arg(state)
                                                : state;
                                        }
                                    }
                                }

                                Button {
                                    id: kickButton

                                    objectName: "arenaRoomKick-" + memberDelegate.memberId
                                    Accessible.description: root.moderationDisabledReason
                                    Accessible.name: qsTr("Kick %1").arg(memberDelegate.displayName)
                                    enabled: root.mutableRoom
                                    text: qsTr("Kick")
                                    visible: root.session.isOwner && !memberDelegate.self
                                    onClicked: root.openKickDialog(memberDelegate.memberId,
                                                                   memberDelegate.displayName,
                                                                   kickButton)
                                }
                            }
                        }
                    }
                }
            }

            Frame {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumHeight: 220

                ColumnLayout {
                    anchors.fill: parent

                    Label {
                        Layout.fillWidth: true
                        font.bold: true
                        text: qsTr("Chat")
                    }

                    ListView {
                        id: chatView

                        property bool followTail: true

                        function scrollToTailIfFollowing() : void {
                            if (!followTail) {
                                return;
                            }
                            Qt.callLater(function() {
                                if (chatView.followTail && chatView.count > 0) {
                                    chatView.positionViewAtEnd();
                                }
                            });
                        }

                        objectName: "arenaRoomChatList"
                        Accessible.name: qsTr("Arena chat")
                        Accessible.role: Accessible.List
                        activeFocusOnTab: true
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        clip: true
                        model: root.session.chat
                        reuseItems: true
                        keyNavigationEnabled: true
                        spacing: 6

                        function ensureCurrentItem(): void {
                            if (chatView.count === 0) {
                                chatView.currentIndex = -1;
                            } else if (chatView.currentIndex < 0
                                       || chatView.currentIndex >= chatView.count) {
                                chatView.currentIndex = 0;
                            }
                        }

                        ScrollBar.vertical: ScrollBar {}

                        Component.onCompleted: {
                            chatView.ensureCurrentItem();
                            scrollToTailIfFollowing();
                        }
                        onCountChanged: chatView.ensureCurrentItem()
                        onMovementStarted: followTail = false
                        onMovementEnded: followTail = atYEnd

                        delegate: Rectangle {
                            id: chatDelegate

                            required property string messageId
                            required property string memberId
                            required property string displayName
                            required property string text
                            required property double timestamp
                            required property bool self

                            objectName: "arenaRoomChat-" + chatDelegate.messageId
                            Accessible.name: qsTr("%1: %2").arg(chatDelegate.displayName).arg(chatDelegate.text)
                            Accessible.role: Accessible.ListItem
                            border.color: ListView.isCurrentItem && ListView.view.activeFocus ? "#2387d9" : "transparent"
                            border.width: ListView.isCurrentItem && ListView.view.activeFocus ? 2 : 0
                            color: "transparent"
                            height: chatContent.implicitHeight + 4
                            radius: 3
                            width: ListView.view.width

                            Column {
                                id: chatContent

                                anchors.fill: parent
                                anchors.margins: 2
                                spacing: 1

                                Label {
                                    Accessible.ignored: true
                                    font.bold: true
                                    text: chatDelegate.displayName
                                    textFormat: Text.PlainText
                                }
                                Label {
                                    Accessible.ignored: true
                                    text: chatDelegate.text
                                    textFormat: Text.PlainText
                                    width: parent.width
                                    wrapMode: Text.Wrap
                                }
                            }
                        }

                        Connections {
                            target: root.session.chat

                            function onModelReset() : void {
                                chatView.scrollToTailIfFollowing();
                            }

                            function onRowsInserted() : void {
                                chatView.scrollToTailIfFollowing();
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        TextField {
                            id: chatField

                            Accessible.name: qsTr("Arena chat message")
                            Layout.fillWidth: true
                            enabled: root.mutableRoom
                            maximumLength: 500
                            placeholderText: qsTr("Message")
                            selectByMouse: true

                            onAccepted: root.submitChat()
                        }

                        Button {
                            enabled: root.mutableRoom
                                && chatField.text.trim().length > 0
                            text: qsTr("Send")
                            onClicked: root.submitChat()
                        }
                    }
                }
            }
        }
    }

    Loader {
        id: kickDialogLoader

        anchors.fill: parent
        active: false
        sourceComponent: kickDialogComponent

        onLoaded: {
            if (status === Loader.Ready && item) {
                item.open();
            }
        }
    }

    Component {
        id: kickDialogComponent

        Dialog {
            anchors.centerIn: parent
            closePolicy: Popup.CloseOnEscape
            modal: true
            standardButtons: Dialog.Ok | Dialog.Cancel
            title: qsTr("Remove player")
            width: Math.min(440, root.width - 48)

            onAccepted: root.kickRequested(root.kickMemberId)
            onClosed: root.finishKickDialog()

            Label {
                text: qsTr("Remove %1 from this room?").arg(root.kickDisplayName)
                textFormat: Text.PlainText
                width: parent.width
                wrapMode: Text.Wrap
            }
        }
    }

    ArenaStatusAnnouncer {
        id: statusAnnouncer

        active: root.visible
        errorMessageKey: root.session.errorMessageKey
        reconnecting: root.session.state === ArenaSession.Reconnecting
        target: root
    }
}

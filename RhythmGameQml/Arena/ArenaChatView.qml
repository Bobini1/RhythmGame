pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root

    required property var session
    required property var chatModel
    property bool inputEnabled: true
    property int unreadCount: 0
    readonly property int messageCount: chatList.count
    readonly property bool atTail: chatList.atYEnd
    readonly property real scrollPosition: chatList.contentY
    readonly property bool inputActiveFocus: input.activeFocus
    property bool followTail: true

    signal inputFocusDismissed
    signal sent

    ArenaTypography {
        id: typography
    }

    function scrollToTail(): void {
        if (!root.followTail) {
            return;
        }
        Qt.callLater(function () {
            if (root.followTail && chatList.count > 0) {
                chatList.currentIndex = chatList.count - 1;
                chatList.positionViewAtEnd();
            }
        });
    }

    function reviewChatRow(index: int): void {
        if (chatList.count <= 0) {
            return;
        }
        root.followTail = false;
        const targetIndex = Math.max(0, Math.min(chatList.count - 1, index));
        chatList.currentIndex = targetIndex;
        chatList.positionViewAtIndex(targetIndex, ListView.Contain);
    }

    function scrollToBeginning(): void {
        root.followTail = false;
        chatList.positionViewAtBeginning();
    }

    function submit(): void {
        const message = input.text.trim();
        if (!root.inputEnabled || message.length === 0) {
            return;
        }
        root.session.sendChat(message);
        input.clear();
        root.sent();
    }

    function dismissInputFocus(): void {
        if (!input.activeFocus) {
            return;
        }
        input.focus = false;
        root.inputFocusDismissed();
    }

    function inputContainsPoint(sourceItem: Item, x: real, y: real): bool {
        const inputPoint = input.mapFromItem(sourceItem, x, y);
        return inputPoint.x >= 0 && inputPoint.x <= input.width
            && inputPoint.y >= 0 && inputPoint.y <= input.height;
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton
        gesturePolicy: TapHandler.WithinBounds

        onTapped: eventPoint => {
            if (!input.activeFocus) {
                return;
            }
            if (root.inputContainsPoint(
                    root, eventPoint.position.x, eventPoint.position.y)) {
                return;
            }
            root.dismissInputFocus();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            visible: root.unreadCount > 0

            Label {
                Layout.fillWidth: true
                font.pixelSize: typography.bodyPixelSize
                text: qsTr("%n unread message(s)", "Arena chat unread count", root.unreadCount)
            }

            Button {
                Accessible.name: qsTr("Jump to newest Arena message")
                focusPolicy: Qt.NoFocus
                font.pixelSize: typography.bodyPixelSize
                text: qsTr("Newest")
                onClicked: {
                    root.followTail = true;
                    root.scrollToTail();
                }
            }
        }

        ListView {
            id: chatList

            objectName: "arenaChatList"
            Accessible.name: qsTr("Arena chat history")
            Accessible.role: Accessible.List
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 72
            activeFocusOnTab: true
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            model: root.chatModel
            reuseItems: true
            keyNavigationEnabled: true
            spacing: 5

            Keys.priority: Keys.BeforeItem
            Keys.onPressed: event => {
                if (event.key === Qt.Key_Up) {
                    root.reviewChatRow(chatList.currentIndex - 1);
                } else if (event.key === Qt.Key_Down) {
                    root.reviewChatRow(chatList.currentIndex + 1);
                } else if (event.key === Qt.Key_Home) {
                    root.reviewChatRow(0);
                } else if (event.key === Qt.Key_End) {
                    root.followTail = true;
                    root.scrollToTail();
                } else {
                    return;
                }
                event.accepted = true;
            }

            function ensureCurrentItem(): void {
                if (chatList.count === 0) {
                    chatList.currentIndex = -1;
                } else if (chatList.currentIndex < 0
                           || chatList.currentIndex >= chatList.count) {
                    chatList.currentIndex = 0;
                }
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            Component.onCompleted: {
                chatList.ensureCurrentItem();
                root.scrollToTail();
            }
            onCountChanged: chatList.ensureCurrentItem()
            onMovementStarted: root.followTail = false
            onMovementEnded: {
                root.followTail = chatList.atYEnd;
                root.scrollToTail();
            }

            delegate: Rectangle {
                id: messageDelegate

                required property string messageId
                required property string displayName
                required property string text
                required property bool self

                objectName: "arenaChatMessage-" + messageDelegate.messageId
                Accessible.name: qsTr("%1: %2").arg(messageDelegate.displayName).arg(messageDelegate.text)
                Accessible.role: Accessible.ListItem
                border.color: ListView.isCurrentItem && ListView.view.activeFocus ? "#8ec5ff" : "transparent"
                border.width: ListView.isCurrentItem && ListView.view.activeFocus ? 2 : 0
                color: "transparent"
                height: messageContent.implicitHeight + 4
                radius: 3
                width: ListView.view.width

                Column {
                    id: messageContent

                    anchors.fill: parent
                    anchors.margins: 2
                    spacing: 1

                    Text {
                        objectName: "arenaChatName-" + messageDelegate.messageId
                        Accessible.ignored: true
                        color: messageDelegate.self ? "#ffe39b" : "#d6deea"
                        elide: Text.ElideRight
                        font.bold: true
                        font.pixelSize: typography.bodyPixelSize
                        text: messageDelegate.displayName
                        textFormat: Text.PlainText
                        width: parent.width
                    }

                    Text {
                        objectName: "arenaChatBody-" + messageDelegate.messageId
                        Accessible.ignored: true
                        color: "white"
                        font.pixelSize: typography.bodyPixelSize
                        text: messageDelegate.text
                        textFormat: Text.PlainText
                        width: parent.width
                        wrapMode: Text.Wrap
                    }
                }
            }

            Connections {
                target: root.chatModel

                function onModelReset(): void {
                    root.scrollToTail();
                }

                function onRowsInserted(): void {
                    root.scrollToTail();
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            TextField {
                id: input

                objectName: "arenaChatInput"
                Accessible.name: qsTr("Arena chat message")
                Layout.fillWidth: true
                enabled: root.inputEnabled
                font.pixelSize: typography.bodyPixelSize
                maximumLength: 500
                placeholderText: qsTr("Message")
                selectByMouse: true
                onAccepted: root.submit()

                Keys.onEscapePressed: event => {
                    root.dismissInputFocus();
                    event.accepted = true;
                }
            }

            Button {
                id: sendButton

                Accessible.name: qsTr("Send Arena chat message")
                enabled: root.inputEnabled && input.text.trim().length > 0
                focusPolicy: Qt.NoFocus
                font.pixelSize: typography.bodyPixelSize
                text: qsTr("Send")
                onClicked: root.submit()
            }
        }
    }
}

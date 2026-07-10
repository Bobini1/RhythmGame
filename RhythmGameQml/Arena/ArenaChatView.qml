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
    property bool followTail: true

    signal sent

    function scrollToTail(): void {
        if (!root.followTail) {
            return;
        }
        Qt.callLater(function () {
            if (root.followTail && chatList.count > 0) {
                chatList.positionViewAtEnd();
            }
        });
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

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            visible: root.unreadCount > 0

            Label {
                Layout.fillWidth: true
                text: qsTr("%n unread message(s)", "Arena chat unread count", root.unreadCount)
            }

            Button {
                Accessible.name: qsTr("Jump to newest Arena message")
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
            onMovementStarted: root.followTail = chatList.atYEnd
            onMovementEnded: root.followTail = chatList.atYEnd

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
                        text: messageDelegate.displayName
                        textFormat: Text.PlainText
                        width: parent.width
                    }

                    Text {
                        objectName: "arenaChatBody-" + messageDelegate.messageId
                        Accessible.ignored: true
                        color: "white"
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
                maximumLength: 500
                placeholderText: qsTr("Message")
                selectByMouse: true
                onAccepted: root.submit()
            }

            Button {
                id: sendButton

                Accessible.name: qsTr("Send Arena chat message")
                enabled: root.inputEnabled && input.text.trim().length > 0
                text: qsTr("Send")
                onClicked: root.submit()
            }
        }
    }
}

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root

    required property var session
    clip: true
    focus: true

    Accessible.role: Accessible.Grouping
    Accessible.name: qsTr("Arena gameplay chat")
    Accessible.description: qsTr("Read messages or send a message to the Arena room")

    function submit() : void {
        const message = messageField.text;
        if (message.trim().length === 0) {
            return;
        }
        root.session.sendChat(message);
        messageField.clear();
    }

    function focusChatRow(index) : void {
        if (chatView.count <= 0) {
            return;
        }
        const targetIndex = Math.max(0, Math.min(chatView.count - 1, index));
        chatView.currentIndex = targetIndex;
        chatView.positionViewAtIndex(targetIndex, ListView.Contain);
    }

    Rectangle {
        anchors.fill: parent
        border.color: "#70ffffff"
        border.width: 1
        color: "#f0101218"
        radius: 6

        Accessible.ignored: true
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 6

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#50ffffff"

            Accessible.ignored: true
        }

        ListView {
            id: chatView

            property bool followTail: true

            objectName: "arenaGameplayChatList"

            function scrollToTailIfFollowing() : void {
                if (!followTail) {
                    return;
                }
                Qt.callLater(function() {
                    if (chatView.followTail && chatView.count > 0) {
                        chatView.currentIndex = chatView.count - 1;
                        chatView.positionViewAtEnd();
                    }
                });
            }

            Layout.fillHeight: true
            Layout.fillWidth: true
            activeFocusOnTab: true
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            model: root.session.chat
            reuseItems: true
            spacing: 4

            Accessible.role: Accessible.List
            Accessible.name: qsTr("Arena chat messages")
            Accessible.description: qsTr("Use the arrow keys to review messages")
            Accessible.focusable: true

            ScrollBar.vertical: ScrollBar {}

            onCountChanged: scrollToTailIfFollowing()
            onMovementStarted: followTail = false
            onMovementEnded: {
                followTail = atYEnd;
                scrollToTailIfFollowing();
            }
            onActiveFocusChanged: {
                if (activeFocus) {
                    root.focusChatRow(Math.max(0, currentIndex));
                }
            }
            Keys.priority: Keys.BeforeItem
            Keys.onPressed: event => {
                if (event.key === Qt.Key_Up) {
                    followTail = false;
                    root.focusChatRow(currentIndex - 1);
                } else if (event.key === Qt.Key_Down) {
                    followTail = false;
                    root.focusChatRow(currentIndex + 1);
                } else if (event.key === Qt.Key_Home) {
                    followTail = false;
                    root.focusChatRow(0);
                } else if (event.key === Qt.Key_End) {
                    followTail = true;
                    scrollToTailIfFollowing();
                } else if (event.key === Qt.Key_Escape) {
                    root.session.setGameplayChatOpen(false);
                } else {
                    return;
                }
                event.accepted = true;
            }

            delegate: Rectangle {
                id: chatDelegate

                required property int index
                required property string displayName
                required property string text
                readonly property bool focusIndicatorVisible: activeFocus
                    || (ListView.isCurrentItem && chatView.activeFocus)

                objectName: "arenaGameplayChatRow" + index
                activeFocusOnTab: false
                border.color: "#8fdcff"
                border.width: focusIndicatorVisible ? 2 : 0
                color: index % 2 === 0 ? "#241b2230" : "transparent"
                height: chatContent.implicitHeight + 8
                radius: 3
                width: ListView.view.width

                Accessible.role: Accessible.ListItem
                Accessible.name: chatDelegate.displayName
                Accessible.description: chatDelegate.text
                Accessible.focusable: true

                onActiveFocusChanged: {
                    if (activeFocus) {
                        chatView.currentIndex = index;
                    }
                }

                Column {
                    id: chatContent

                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 1

                    Text {
                        color: "#ffe38a"
                        font.bold: true
                        text: chatDelegate.displayName
                        textFormat: Text.PlainText

                        Accessible.ignored: true
                    }

                    Text {
                        objectName: "arenaGameplayChatText" + chatDelegate.index
                        color: "white"
                        text: chatDelegate.text
                        textFormat: Text.PlainText
                        width: parent.width
                        wrapMode: Text.Wrap

                        Accessible.ignored: true
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
            spacing: 6

            TextArea {
                id: messageField

                objectName: "arenaGameplayMessage"
                Accessible.name: qsTr("Arena gameplay chat message")
                Accessible.description: qsTr("Enter sends. Shift plus Enter adds a new line. Escape closes chat.")
                KeyNavigation.tab: sendButton
                Keys.priority: Keys.BeforeItem
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                focus: true
                placeholderText: qsTr("Message")
                selectByMouse: true
                wrapMode: TextEdit.Wrap

                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Escape) {
                        root.session.setGameplayChatOpen(false);
                        event.accepted = true;
                        return;
                    }
                    if (event.key !== Qt.Key_Return
                            && event.key !== Qt.Key_Enter) {
                        return;
                    }
                    if ((event.modifiers & Qt.ShiftModifier) !== 0) {
                        event.accepted = false;
                        return;
                    }
                    root.submit();
                    event.accepted = true;
                }
            }

            Button {
                id: sendButton

                enabled: messageField.text.trim().length > 0
                KeyNavigation.backtab: messageField
                text: qsTr("Send")
                onClicked: root.submit()
            }
        }
    }
}

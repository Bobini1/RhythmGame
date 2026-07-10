pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root

    required property var session
    clip: true
    focus: true

    function submit() : void {
        const message = messageField.text;
        if (message.trim().length === 0) {
            return;
        }
        root.session.sendChat(message);
        messageField.clear();
    }

    Rectangle {
        anchors.fill: parent
        border.color: "#70ffffff"
        border.width: 1
        color: "#f0101218"
        radius: 6
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 6

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#50ffffff"
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

            Layout.fillHeight: true
            Layout.fillWidth: true
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            model: root.session.chat
            reuseItems: true
            spacing: 4

            ScrollBar.vertical: ScrollBar {}

            onCountChanged: scrollToTailIfFollowing()
            onMovementStarted: followTail = false
            onMovementEnded: followTail = atYEnd

            delegate: Column {
                id: chatDelegate

                required property string displayName
                required property string text

                spacing: 1
                width: ListView.view.width

                Text {
                    color: "#ffe38a"
                    font.bold: true
                    text: chatDelegate.displayName
                    textFormat: Text.PlainText
                }

                Text {
                    color: "white"
                    text: chatDelegate.text
                    textFormat: Text.PlainText
                    width: parent.width
                    wrapMode: Text.Wrap
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

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

RowLayout {
    id: root

    required property var session
    readonly property bool chatOpen: session && session.chatOpen === true
    readonly property bool hovered: detailsButton.hovered || chatButton.hovered

    signal tabSelected(bool chat)

    spacing: 0

    ArenaTypography {
        id: typography
    }

    component ModeButton: Button {
        id: control

        required property bool selected

        Layout.preferredHeight: implicitHeight
        implicitHeight: Math.max(40, implicitContentHeight + topPadding + bottomPadding)
        implicitWidth: Math.max(64, implicitContentWidth + 20)
        focusPolicy: Qt.NoFocus
        horizontalPadding: 10
        verticalPadding: 6
        hoverEnabled: true

        background: Item {
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                color: control.selected ? "#8b96a6" : "transparent"
                height: 3
                radius: 2
                width: 16
            }
        }

        contentItem: Text {
            color: control.enabled ? "white" : "#72ffffff"
            font.pixelSize: typography.bodyPixelSize
            horizontalAlignment: Text.AlignHCenter
            text: control.text
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
        }
    }

    ModeButton {
        id: detailsButton

        objectName: "arenaDetailsTab"
        selected: !root.chatOpen
        text: qsTr("Details")
        onClicked: {
            if (root.session)
                root.session.setChatOpen(false);
            root.tabSelected(false);
        }
    }

    ModeButton {
        id: chatButton

        objectName: "arenaChatTab"
        selected: root.chatOpen
        text: qsTr("Chat")
        onClicked: {
            if (root.session)
                root.session.setChatOpen(true);
            root.tabSelected(true);
        }
    }
}

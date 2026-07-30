pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    required property var session
    readonly property int unreadCount: session
        ? Number(session.unreadChatCount || 0) : 0

    Accessible.role: Accessible.Button
    Accessible.focusable: visible
    Accessible.focused: activeFocus
    Accessible.onPressAction: openChat()

    activeFocusOnTab: visible
    color: "#d9273444"
    border.color: activeFocus ? "#8fdcff" : "#58718a"
    border.width: activeFocus ? 2 : 1
    clip: true
    implicitHeight: Math.max(38, activityContent.implicitHeight + 12)
    radius: 3
    visible: unreadCount > 0 && session && session.chatOpen !== true

    ArenaTypography {
        id: typography
    }

    function openChat(): void {
        if (session)
            session.setChatOpen(true);
    }

    RowLayout {
        id: activityContent

        anchors.fill: parent
        anchors.leftMargin: 9
        anchors.rightMargin: 7
        spacing: 7

        Text {
            Layout.maximumWidth: Math.max(0, root.width * 0.28)
            color: "white"
            elide: Text.ElideRight
            font.bold: true
            font.pixelSize: typography.bodyPixelSize
            text: root.session
                ? String(root.session.latestUnreadChatDisplayName || "") : ""
            textFormat: Text.PlainText

            Accessible.ignored: true
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            color: "#d6deea"
            elide: Text.ElideRight
            font.pixelSize: typography.bodyPixelSize
            text: root.session
                ? String(root.session.latestUnreadChatText || "") : ""
            textFormat: Text.PlainText

            Accessible.ignored: true
        }

        Rectangle {
            color: "#8fdcff"
            implicitHeight: Math.max(22, countText.implicitHeight + 6)
            implicitWidth: Math.max(implicitHeight, countText.implicitWidth + 10)
            radius: height / 2

            Accessible.ignored: true

            Text {
                id: countText

                anchors.centerIn: parent
                color: "#111821"
                font.bold: true
                font.pixelSize: typography.bodyPixelSize
                text: String(root.unreadCount)
                textFormat: Text.PlainText
            }
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        onTapped: root.openChat()
    }

    Keys.onPressed: event => {
        if (event.key !== Qt.Key_Return && event.key !== Qt.Key_Enter
                && event.key !== Qt.Key_Space) {
            return;
        }
        root.openChat();
        event.accepted = true;
    }
}

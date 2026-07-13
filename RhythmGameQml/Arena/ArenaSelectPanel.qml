pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root

    required property var session
    readonly property bool arenaNativeSelectPresentation: true
    readonly property alias announcementCount: statusAnnouncer.announcementCount
    readonly property alias lastAnnouncementKey: statusAnnouncer.lastAnnouncementKey
    readonly property alias lastAnnouncementText: statusAnnouncer.lastAnnouncementText
    readonly property alias dragHandle: selectHeader
    readonly property string detailMode: tabs.currentIndex === 1 ? "chat" : "details"
    readonly property real rosterColumnWidth: 270
    readonly property real normalBodyMinimumWidth: rosterColumnWidth + 8 + 220
    readonly property bool narrowChatMode: detailMode === "chat"
        && width - 20 < normalBodyMinimumWidth
    readonly property string readyDisabledReason: {
        if (!root.session) {
            return "";
        }
        if (root.session.roundsAvailable === false) {
            return qsTr("Update required to play in this room.");
        }
        if (root.session.availabilitySyncing === true) {
            return qsTr("Song libraries are still being compared.");
        }
        if (String(root.session.currentRoundId || "").length > 0) {
            return qsTr("The synchronized round is already being prepared.");
        }
        if (root.session.ready !== true && root.session.canReady !== true) {
            return qsTr("Choose a chart available to everyone before becoming ready.");
        }
        return "";
    }

    Accessible.name: root.session
        ? (root.session.roomName || qsTr("Arena room")) : qsTr("Arena room")
    Accessible.role: Accessible.Grouping

    Rectangle {
        anchors.fill: parent
        border.color: "#74859a"
        border.width: 1
        color: "#ed111821"
        radius: 5
    }

    MouseArea {
        objectName: "arenaSelectWheelSink"
        anchors.fill: parent
        acceptedButtons: Qt.NoButton

        onWheel: wheel => wheel.accepted = true
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            id: selectHeader

            objectName: "arenaSelectHeader"
            Layout.fillWidth: true
            Layout.fillHeight: false
            Layout.maximumHeight: 40
            Layout.minimumHeight: 40
            Layout.preferredHeight: 40
            spacing: 4

            Item {
                Layout.fillHeight: true
                Layout.preferredWidth: 12

                Accessible.ignored: true

                Column {
                    anchors.centerIn: parent
                    spacing: 3

                    Repeater {
                        model: 3

                        Rectangle {
                            color: "#8b96a6"
                            height: 1
                            width: 10
                        }
                    }
                }
            }

            Text {
                id: roomTitle

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                objectName: "arenaSelectRoomTitle"
                color: "white"
                elide: Text.ElideRight
                font.bold: true
                font.pixelSize: 18
                text: root.session
                    ? (root.session.roomName || qsTr("Arena room"))
                    : qsTr("Arena room")
                textFormat: Text.PlainText
                verticalAlignment: Text.AlignVCenter
            }

            TabBar {
                id: tabs

                readonly property real stableWidth: detailsTab.width
                    + chatTab.width + spacing + leftPadding + rightPadding

                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: false
                Layout.maximumWidth: stableWidth
                Layout.minimumWidth: stableWidth
                Layout.preferredHeight: 40
                Layout.preferredWidth: stableWidth
                hoverEnabled: true

                TabButton {
                    id: detailsTab

                    objectName: "arenaSelectDetailsTab"
                    Accessible.name: qsTr("Show Arena room details")
                    horizontalPadding: 10
                    implicitHeight: Math.max(32,
                                             implicitContentHeight
                                             + topPadding + bottomPadding)
                    implicitWidth: Math.max(64,
                                            implicitContentWidth
                                            + leftPadding + rightPadding)
                    text: qsTr("Details")
                    width: implicitWidth
                }

                TabButton {
                    id: chatTab

                    objectName: "arenaSelectChatTab"
                    Accessible.name: qsTr("Show Arena chat")
                    horizontalPadding: 10
                    implicitHeight: Math.max(32,
                                             implicitContentHeight
                                             + topPadding + bottomPadding)
                    implicitWidth: Math.max(64,
                                            implicitContentWidth
                                            + leftPadding + rightPadding)
                    text: qsTr("Chat")
                    width: implicitWidth
                }
            }

            HoverHandler {
                enabled: !tabs.hovered
                cursorShape: Qt.SizeAllCursor
            }
        }

        GridLayout {
            id: bodyLayout

            objectName: "arenaSelectBody"
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 0
            clip: true
            columnSpacing: 8
            columns: root.width - 20 >= root.normalBodyMinimumWidth ? 2 : 1
            rowSpacing: 8

            ArenaRosterView {
                objectName: "arenaSelectRoster"
                Layout.column: 0
                Layout.fillHeight: true
                Layout.fillWidth: bodyLayout.columns === 1
                Layout.maximumWidth: bodyLayout.columns === 1
                    ? Number.POSITIVE_INFINITY : root.rosterColumnWidth
                Layout.minimumHeight: 0
                Layout.minimumWidth: bodyLayout.columns === 1
                    ? 0 : root.rosterColumnWidth
                Layout.preferredHeight: bodyLayout.columns === 1 ? 1 : -1
                Layout.preferredWidth: bodyLayout.columns === 1
                    ? -1 : root.rosterColumnWidth
                Layout.row: 0
                compact: true
                moderationEnabled: true
                session: root.session
                visible: !root.narrowChatMode
                onKickRequested: memberId => {
                    if (root.session)
                        root.session.kickMember(memberId);
                }
            }

            Loader {
                id: detailsLoader

                Layout.column: bodyLayout.columns === 1 ? 0 : 1
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumHeight: 0
                Layout.minimumWidth: bodyLayout.columns === 1 ? 0 : 220
                Layout.preferredHeight: bodyLayout.columns === 1 ? 1 : -1
                Layout.row: bodyLayout.columns === 1
                    && !root.narrowChatMode ? 1 : 0
                sourceComponent: tabs.currentIndex === 1
                    ? chatComponent : summaryComponent
            }
        }

        Text {
            objectName: "arenaSelectReadyDisabledReason"
            Accessible.name: text
            Accessible.role: Accessible.StaticText
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            color: "#ffb2a8"
            text: root.readyDisabledReason
            textFormat: Text.PlainText
            visible: text.length > 0
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                color: root.session && root.session.ready === true
                    ? "#b8f0c5" : "#d6deea"
                elide: Text.ElideRight
                text: root.session
                    ? (root.session.ready === true ? qsTr("Ready")
                                                   : qsTr("Not ready")) : ""
            }

            Button {
                objectName: "arenaSelectReady"
                Accessible.description: root.readyDisabledReason
                Accessible.name: text
                enabled: root.session
                    && root.session.roundsAvailable !== false
                    && String(root.session.currentRoundId || "").length === 0
                    && (root.session.ready === true || root.session.canReady === true)
                text: root.session && root.session.ready === true
                    ? qsTr("Unready") : qsTr("Ready")
                onClicked: {
                    if (root.session)
                        root.session.setReady(root.session.ready !== true);
                }
            }

        }
    }

    ArenaStatusAnnouncer {
        id: statusAnnouncer

        active: root.visible
        errorMessageKey: root.session ? String(root.session.errorMessageKey || "") : ""
        reconnecting: root.session ? root.session.reconnecting === true : false
        roundLaunchCancellationStatusKey: root.session
            ? String(root.session.roundLaunchCancellationStatusKey || "") : ""
        target: root
    }

    Component {
        id: summaryComponent

        ScrollView {
            id: detailsScroll

            objectName: "arenaSelectSelection"
            clip: true
            contentWidth: availableWidth
            readonly property Flickable scrollFlickable: contentItem as Flickable
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            function scrollByWheel(wheel): void {
                const flickable = detailsScroll.scrollFlickable;
                let delta = wheel.pixelDelta.y;
                if (delta === 0) {
                    delta = wheel.angleDelta.y / 3;
                }
                if (wheel.inverted) {
                    delta = -delta;
                }
                const minimumY = flickable.originY;
                const maximumY = Math.max(minimumY,
                    minimumY + flickable.contentHeight - flickable.height);
                flickable.contentY = Math.max(minimumY,
                    Math.min(maximumY, flickable.contentY - delta));
            }

            WheelHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                target: null
                onWheel: wheel => detailsScroll.scrollByWheel(wheel)
            }

            ArenaSelectionSummary {
                id: selectionSummary

                compact: true
                session: root.session
                width: detailsScroll.availableWidth
            }
        }
    }

    Component {
        id: chatComponent

        ArenaChatView {
            objectName: "arenaSelectChat"
            chatModel: root.session ? root.session.chat : null
            inputEnabled: true
            session: root.session
        }
    }
}

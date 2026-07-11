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
    readonly property alias dragHandle: titleDragHandle
    readonly property string detailMode: tabs.currentIndex === 1 ? "chat" : "details"
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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item {
                id: titleDragHandle

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumHeight: roomTitle.implicitHeight

                Text {
                    id: roomTitle

                    anchors.fill: parent
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
            }

            TabBar {
                id: tabs

                Layout.alignment: Qt.AlignVCenter

                TabButton {
                    objectName: "arenaSelectDetailsTab"
                    Accessible.name: qsTr("Show Arena room details")
                    text: qsTr("Details")
                }

                TabButton {
                    objectName: "arenaSelectChatTab"
                    Accessible.name: qsTr("Show Arena chat")
                    text: qsTr("Chat")
                }
            }
        }

        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 8

            ArenaRosterView {
                objectName: "arenaSelectRoster"
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumWidth: 270
                compact: true
                moderationEnabled: true
                session: root.session
                onKickRequested: memberId => {
                    if (root.session)
                        root.session.kickMember(memberId);
                }
            }

            Loader {
                id: detailsLoader

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumWidth: 220
                sourceComponent: tabs.currentIndex === 1
                    ? chatComponent : summaryComponent
            }
        }

        Text {
            objectName: "arenaSelectReadyDisabledReason"
            Accessible.name: text
            Accessible.role: Accessible.StaticText
            Layout.fillWidth: true
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
                color: root.session && root.session.ready === true
                    ? "#b8f0c5" : "#d6deea"
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

            Button {
                objectName: "arenaSelectLeave"
                Accessible.name: qsTr("Leave Arena room")
                text: qsTr("Leave")
                onClicked: {
                    if (root.session)
                        root.session.leaveRoom();
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

        ArenaSelectionSummary {
            objectName: "arenaSelectSelection"
            compact: true
            session: root.session
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

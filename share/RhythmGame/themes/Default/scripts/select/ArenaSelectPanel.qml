pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml

FocusScope {
    id: root

    required property var session
    readonly property bool arenaNativeSelectPresentation: true
    property string detailMode: "details"

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

            Text {
                Layout.fillWidth: true
                color: "white"
                elide: Text.ElideRight
                font.bold: true
                font.pixelSize: 18
                text: root.session.roomName || qsTr("Arena room")
                textFormat: Text.PlainText
            }

            Button {
                id: detailsTab

                objectName: "arenaDefaultDetailsTab"
                Accessible.name: qsTr("Show Arena room details")
                checked: root.detailMode === "details"
                checkable: true
                text: qsTr("Details")
                onClicked: root.detailMode = "details"
            }

            Button {
                id: chatTab

                objectName: "arenaDefaultChatTab"
                Accessible.name: qsTr("Show Arena chat")
                checked: root.detailMode === "chat"
                checkable: true
                text: qsTr("Chat")
                onClicked: root.detailMode = "chat"
            }
        }

        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 8

            ArenaRosterView {
                objectName: "arenaDefaultRoster"
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumWidth: 270
                compact: true
                moderationEnabled: true
                session: root.session
                onKickRequested: memberId => root.session.kickMember(memberId)
            }

            Loader {
                id: detailsLoader

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumWidth: 220
                sourceComponent: root.detailMode === "chat" ? chatComponent : summaryComponent
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.fillWidth: true
                color: root.session.ready === true ? "#b8f0c5" : "#d6deea"
                text: root.session.ready === true ? qsTr("Ready") : qsTr("Not ready")
            }

            Button {
                id: readyButton

                objectName: "arenaDefaultReady"
                Accessible.description: root.summaryLoaderItem ? root.summaryLoaderItem.readyDisabledReason : ""
                Accessible.name: text
                enabled: root.session.roundsAvailable !== false && String(root.session.currentRoundId || "").length === 0 && (root.session.ready === true || root.session.canReady === true)
                text: root.session.ready === true ? qsTr("Unready") : qsTr("Ready")
                onClicked: root.session.setReady(root.session.ready !== true)
            }

            Button {
                objectName: "arenaDefaultLeave"
                Accessible.name: qsTr("Leave Arena room")
                text: qsTr("Leave")
                onClicked: root.session.leaveRoom()
            }
        }
    }

    readonly property var summaryLoaderItem: detailsLoader.status === Loader.Ready && root.detailMode === "details" ? detailsLoader.item : null

    Component {
        id: summaryComponent

        ArenaSelectionSummary {
            objectName: "arenaDefaultSelection"
            session: root.session
        }
    }

    Component {
        id: chatComponent

        ArenaChatView {
            objectName: "arenaDefaultChat"
            chatModel: root.session.chat
            inputEnabled: true
            session: root.session
        }
    }
}

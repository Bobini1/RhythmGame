pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root

    required property var session
    required property var presentationItem
    required property Item viewport
    property bool expanded: false
    property string detailMode: "players"
    readonly property bool nativePresentation: presentationItem !== null && presentationItem.arenaNativeSelectPresentation !== undefined && presentationItem.arenaNativeSelectPresentation === true
    readonly property real safeMarginX: Math.min(24, Math.max(0, (viewport.width - 1) / 2))
    readonly property real safeMarginY: Math.min(24, Math.max(0, (viewport.height - 1) / 2))
    readonly property real availableWidth: Math.max(0, viewport.width - 2 * safeMarginX)
    readonly property real availableHeight: Math.max(0, viewport.height - 2 * safeMarginY)

    objectName: "arenaLegacySelectOverlay"
    Accessible.name: qsTr("Arena room overlay")
    Accessible.role: Accessible.Pane
    height: Math.min(availableHeight, expanded ? 640 : compactHeader.implicitHeight + 20)
    visible: !nativePresentation
    width: Math.min(420, availableWidth)
    x: Math.max(safeMarginX, viewport.width - safeMarginX - width)
    y: safeMarginY

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
            spacing: 6

            ArenaSelectStrip {
                id: compactHeader

                objectName: "arenaLegacyCompactHeader"
                Layout.fillWidth: true
                actionsVisible: false
                compact: true
                session: root.session
            }

            Button {
                id: expandButton

                objectName: "arenaLegacyExpand"
                Accessible.name: root.expanded ? qsTr("Collapse Arena room overlay") : qsTr("Expand Arena room overlay")
                Layout.minimumHeight: 40
                Layout.minimumWidth: 40
                text: root.expanded ? "−" : "+"
                onClicked: root.expanded = !root.expanded
            }
        }

        Loader {
            id: expandedLoader

            Layout.fillHeight: true
            Layout.fillWidth: true
            active: root.expanded
            focus: active
            sourceComponent: expandedComponent
        }
    }

    Component {
        id: expandedComponent

        ColumnLayout {
            spacing: 6

            ArenaSelectionSummary {
                Layout.fillWidth: true
                compact: false
                session: root.session
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Button {
                    objectName: "arenaLegacyPlayersTab"
                    Accessible.name: qsTr("Show Arena players")
                    checked: root.detailMode === "players"
                    checkable: true
                    text: qsTr("Players")
                    onClicked: root.detailMode = "players"
                }

                Button {
                    objectName: "arenaLegacyChatTab"
                    Accessible.name: qsTr("Show Arena chat")
                    checked: root.detailMode === "chat"
                    checkable: true
                    text: qsTr("Chat")
                    onClicked: root.detailMode = "chat"
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            StackLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                currentIndex: root.detailMode === "chat" ? 1 : 0

                ArenaRosterView {
                    objectName: "arenaLegacyRoster"
                    compact: true
                    moderationEnabled: true
                    session: root.session
                    onKickRequested: memberId => root.session.kickMember(memberId)
                }

                ArenaChatView {
                    objectName: "arenaLegacyChat"
                    chatModel: root.session ? root.session.chat : null
                    inputEnabled: true
                    session: root.session
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    Layout.fillWidth: true
                    color: root.session && root.session.ready === true ? "#b8f0c5" : "#d6deea"
                    text: root.session ? (root.session.ready === true ? qsTr("Ready") : qsTr("Not ready")) : ""
                }

                Button {
                    objectName: "arenaLegacyReady"
                    Accessible.description: compactHeader.readyDisabledReason
                    Accessible.name: text
                    enabled: root.session && root.session.roundsAvailable !== false && String(root.session.currentRoundId || "").length === 0 && (root.session.ready === true || root.session.canReady === true)
                    text: root.session && root.session.ready === true ? qsTr("Unready") : qsTr("Ready")
                    onClicked: {
                        if (root.session) {
                            root.session.setReady(root.session.ready !== true);
                        }
                    }
                }

                Button {
                    objectName: "arenaLegacyLeave"
                    Accessible.name: qsTr("Leave Arena room")
                    text: qsTr("Leave")
                    onClicked: root.session.leaveRoom()
                }
            }
        }
    }
}

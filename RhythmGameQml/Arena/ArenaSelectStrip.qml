import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: root

    required property var session
    property bool actionsVisible: true
    property bool compact: false
    readonly property int connectedCount: rosterCounter.connectedCount
    readonly property int reservedCount: rosterCounter.reservedCount
    readonly property bool preparingRound: session ? String(session.currentRoundId || "").length > 0 : false
    readonly property bool updateRequired: session ? session.roundsAvailable === false : false
    readonly property bool busy: session ? session.availabilitySyncing === true || preparingRound : false
    readonly property bool ready: session ? session.ready === true : false
    readonly property bool readyForCurrentRound: ready || preparingRound
    signal leaveRequested

    function errorText(key): string {
        switch (key) {
        case "arena.error.notCommon":
            return qsTr("That chart is not available to every player.");
        case "arena.error.roomLibraryChanged":
            return qsTr("The room library changed. Please try again.");
        case "arena.error.unsupportedConfig":
            return qsTr("This chart configuration is not supported by Arena.");
        default:
            return "";
        }
    }

    function phaseText(): string {
        if (!root.session) {
            return "";
        }
        if (root.updateRequired) {
            return qsTr("Update required");
        }
        if (root.session.availabilitySyncing === true) {
            return qsTr("Comparing libraries…");
        }
        if (root.preparingRound) {
            return qsTr("Preparing synchronized start…");
        }
        return root.ready ? qsTr("Ready") : qsTr("Not ready");
    }

    Accessible.role: Accessible.Grouping
    implicitHeight: content.implicitHeight + topPadding + bottomPadding

    GridLayout {
        id: content

        anchors.fill: parent
        columns: root.actionsVisible && root.width >= 760 ? 2 : 1
        columnSpacing: 12
        rowSpacing: root.compact ? 2 : 5

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    objectName: "arenaStripRoom"
                    Layout.fillWidth: true
                    color: "white"
                    elide: Text.ElideRight
                    font.bold: true
                    text: root.session ? root.session.roomName || qsTr("Arena room") : ""
                    textFormat: Text.PlainText
                }

                Text {
                    color: "#d6deea"
                    text: qsTr("%1 connected · %2 reserved").arg(root.connectedCount).arg(root.reservedCount)
                }
            }

            Text {
                objectName: "arenaStripSelection"
                Layout.fillWidth: true
                color: "#d6deea"
                elide: Text.ElideRight
                text: root.session ? (String(root.session.selectedTitle || "").length > 0 ? qsTr("Selected: %1").arg(root.session.selectedTitle) : qsTr("Choose any chart available to everyone.")) : ""
                textFormat: Text.PlainText
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                BusyIndicator {
                    Accessible.ignored: true
                    Layout.preferredHeight: 24
                    Layout.preferredWidth: 24
                    running: visible && root.visible
                    visible: root.busy
                }

                Text {
                    objectName: "arenaStripReadyState"
                    Layout.fillWidth: true
                    color: root.readyForCurrentRound ? "#b8f0c5" : "#ffd38a"
                    elide: Text.ElideRight
                    text: root.phaseText()
                    textFormat: Text.PlainText
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            Layout.fillWidth: content.columns === 1
            spacing: 6
            visible: root.actionsVisible

            Button {
                objectName: "arenaStripReady"
                Accessible.name: text
                enabled: root.session && !root.updateRequired && !root.preparingRound && (root.ready || root.session.canReady === true)
                text: root.preparingRound
                    ? qsTr("Ready")
                    : (root.ready ? qsTr("Unready") : qsTr("Ready"))
                onClicked: {
                    if (root.session) {
                        root.session.setReady(!root.ready);
                    }
                }
            }

            Button {
                text: qsTr("Leave")
                onClicked: root.leaveRequested()
            }
        }

        Text {
            Accessible.name: text
            Accessible.role: Accessible.AlertMessage
            Layout.columnSpan: content.columns
            Layout.fillWidth: true
            color: "#ffb2a8"
            text: root.session ? root.errorText(root.session.errorMessageKey) : ""
            textFormat: Text.PlainText
            visible: text.length > 0
            wrapMode: Text.Wrap
        }
    }

    ArenaRosterView {
        id: rosterCounter

        height: 0
        moderationEnabled: false
        session: root.session
        visible: false
        width: 0
    }

}

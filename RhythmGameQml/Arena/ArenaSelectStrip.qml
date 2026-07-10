import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: root

    required property var session

    signal leaveRequested()

    readonly property bool preparingRound: String(session.currentRoundId || "").length > 0
    readonly property bool updateRequired: session.roundsAvailable === false
    readonly property bool busy: !!session.availabilitySyncing || preparingRound

    function errorText(key) : string {
        switch (key) {
        case "arena.error.notCommon":
            return qsTr("That chart is not available to every player.");
        case "arena.error.selectionStale":
        case "arena.error.availabilityStale":
        case "arena.error.inventoryStale":
            return qsTr("The room library changed. Please try again.");
        case "arena.error.roundLoading":
            return qsTr("The next round is already being prepared.");
        case "arena.error.unsupportedConfig":
            return qsTr("This chart configuration is not supported by Arena.");
        case "arena.error.missingFile":
            return qsTr("The selected chart is no longer available locally.");
        case "arena.error.hashMismatch":
            return qsTr("The local chart file no longer matches the room selection.");
        case "arena.error.parseFailed":
        case "arena.error.resourceFailed":
            return qsTr("The selected chart could not be prepared.");
        default:
            return "";
        }
    }

    Accessible.name: qsTr("Arena room controls")
    implicitHeight: content.implicitHeight + topPadding + bottomPadding

    GridLayout {
        id: content

        anchors.fill: parent
        columns: root.width >= 860 ? 2 : 1
        columnSpacing: 16
        rowSpacing: 8

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.bold: true
                text: root.session.roomName || qsTr("Arena room")
                textFormat: Text.PlainText
            }

            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: {
                    const title = String(root.session.selectedTitle || "");
                    if (title.length === 0) {
                        return qsTr("Choose any chart available to everyone.");
                    }
                    return qsTr("Selected: %1").arg(title);
                }
                textFormat: Text.PlainText
            }

            Label {
                Layout.fillWidth: true
                color: palette.mid
                elide: Text.ElideRight
                text: {
                    const memberId = String(root.session.selectedByMemberId || "");
                    return memberId.length > 0
                        ? qsTr("Selected by %1").arg(memberId)
                        : "";
                }
                textFormat: Text.PlainText
                visible: text.length > 0
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            Layout.fillWidth: root.width < 860
            spacing: 8

            BusyIndicator {
                Accessible.ignored: true
                Layout.preferredHeight: 32
                Layout.preferredWidth: 32
                running: visible && root.visible
                visible: root.busy
            }

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                text: {
                    if (root.updateRequired) {
                        return qsTr("Update required to play in this room");
                    }
                    if (root.session.availabilitySyncing) {
                        return qsTr("Comparing song libraries…");
                    }
                    if (root.preparingRound) {
                        return qsTr("Preparing the synchronized start…");
                    }
                    if (root.session.ready) {
                        return qsTr("Ready");
                    }
                    return qsTr("Not ready");
                }
                wrapMode: Text.Wrap
            }

            Button {
                id: readyButton

                Accessible.name: text
                enabled: !root.updateRequired
                    && !root.preparingRound
                    && (!!root.session.ready || !!root.session.canReady)
                text: root.session.ready ? qsTr("Unready") : qsTr("Ready")
                onClicked: root.session.setReady(!root.session.ready)
            }

            Button {
                Accessible.name: text
                text: qsTr("Leave room")
                onClicked: root.leaveRequested()
            }
        }

        Label {
            Layout.columnSpan: content.columns
            Layout.fillWidth: true
            color: palette.accent
            text: root.errorText(root.session.errorMessageKey)
            textFormat: Text.PlainText
            visible: text.length > 0
            wrapMode: Text.Wrap
        }
    }
}

import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var session
    property bool compact: false
    readonly property string readyDisabledReason: {
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
    readonly property string syncText: {
        if (root.session.availabilitySyncing === true) {
            return qsTr("Comparing song libraries…");
        }
        if (String(root.session.currentRoundId || "").length > 0) {
            return qsTr("Preparing the synchronized start…");
        }
        switch (String(root.session.errorMessageKey || "")) {
        case "arena.error.selectionStale":
        case "arena.error.availabilityStale":
        case "arena.error.inventoryStale":
            return qsTr("The room library changed. Select the chart again.");
        case "arena.error.parseFailed":
        case "arena.error.resourceFailed":
        case "arena.error.missingFile":
        case "arena.error.hashMismatch":
            return qsTr("Round preparation was cancelled because the chart could not be loaded.");
        default:
            return "";
        }
    }
    readonly property var result: root.session.lastResult
    readonly property string winnerText: {
        const value = root.result;
        if (!value || value.valid !== true || !value.winnerNames || value.winnerNames.length === 0) {
            return "";
        }
        return value.winnerNames.join(", ");
    }

    spacing: root.compact ? 2 : 6

    Text {
        objectName: "arenaSelectionTitle"
        Layout.fillWidth: true
        color: "white"
        elide: Text.ElideRight
        font.bold: true
        text: String(root.session.selectedTitle || "").length > 0 ? String(root.session.selectedTitle) : qsTr("No chart selected")
        textFormat: Text.PlainText
    }

    Text {
        objectName: "arenaSelectionSelector"
        Layout.fillWidth: true
        color: "#c9d2df"
        elide: Text.ElideRight
        text: String(root.session.selectedByMemberId || "").length > 0 ? qsTr("Selected by %1").arg(root.session.selectedByMemberId) : qsTr("Choose any chart available to everyone.")
        textFormat: Text.PlainText
    }

    Text {
        objectName: "arenaSelectionOptions"
        Layout.fillWidth: true
        color: "#d6deea"
        text: String(root.session.arenaOptionsSummary || "")
        textFormat: Text.PlainText
        visible: text.length > 0
        wrapMode: Text.Wrap
    }

    Text {
        objectName: "arenaSelectionSync"
        Accessible.name: text
        Accessible.role: Accessible.StaticText
        Layout.fillWidth: true
        color: "#ffd38a"
        text: root.syncText
        textFormat: Text.PlainText
        visible: text.length > 0
        wrapMode: Text.Wrap
    }

    Text {
        objectName: "arenaReadyDisabledReason"
        Layout.fillWidth: true
        color: "#ffb2a8"
        text: root.readyDisabledReason
        textFormat: Text.PlainText
        visible: !root.compact && text.length > 0
        wrapMode: Text.Wrap
    }

    Text {
        objectName: "arenaLastWinners"
        Layout.fillWidth: true
        color: "#ffe39b"
        text: root.winnerText.length > 0 ? qsTr("Last winner(s): %1").arg(root.winnerText) : ""
        textFormat: Text.PlainText
        visible: text.length > 0
        wrapMode: Text.Wrap
    }
}

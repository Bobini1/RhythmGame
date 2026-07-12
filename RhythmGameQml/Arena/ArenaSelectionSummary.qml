import QtQuick
import QtQuick.Layouts
import RhythmGameQml

ColumnLayout {
    id: root

    required property var session
    property var tables: Rg.tables
    property int tableRevision: 0
    property bool compact: false
    readonly property string selectedChartText: {
        if (!root.session) {
            return "";
        }
        const title = root.singleLine(root.session.selectedTitle);
        if (title.length === 0) {
            return "";
        }
        const subtitle = root.singleLine(root.session.selectedSubtitle);
        const metadata = subtitle.length > 0 ? title + " " + subtitle : title;
        const md5 = String(root.session.selectedMd5 || "");
        if (md5.length === 0) {
            return metadata;
        }
        const matches = root.searchLocalTables(md5, root.tableRevision);
        if (!matches || matches.length === 0) {
            return metadata;
        }
        const prefix = String(matches[0].symbol || "") + String(matches[0].levelName || "");
        return prefix !== "" ? prefix + " " + metadata : metadata;
    }
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
    readonly property string syncText: {
        if (!root.session) {
            return "";
        }
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
    readonly property var result: root.session ? root.session.lastResult : null
    readonly property string winnerText: {
        const value = root.result;
        if (!value || value.valid !== true || !value.winnerNames || value.winnerNames.length === 0) {
            return "";
        }
        return value.winnerNames.join(", ");
    }

    function singleLine(value): string {
        return String(value || "").replace(/\r\n|\n|\r/g, " ").trim();
    }

    function searchLocalTables(md5, revision): var {
        // revision is the cache key that re-runs the lookup after table reloads.
        if (revision < 0 || !root.tables || !root.tables.search) {
            return [];
        }
        return root.tables.search(md5);
    }

    spacing: root.compact ? 2 : 6

    Text {
        objectName: "arenaSelectionTitle"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        color: "white"
        font.bold: true
        text: root.session ? (root.selectedChartText.length > 0 ? root.selectedChartText : qsTr("No chart selected")) : ""
        textFormat: Text.PlainText
        wrapMode: Text.Wrap
    }

    Connections {
        target: root.tables
        ignoreUnknownSignals: true

        function onDataChanged(): void {
            ++root.tableRevision;
        }

        function onModelReset(): void {
            ++root.tableRevision;
        }

        function onRowsInserted(): void {
            ++root.tableRevision;
        }

        function onRowsMoved(): void {
            ++root.tableRevision;
        }

        function onRowsRemoved(): void {
            ++root.tableRevision;
        }
    }

    Text {
        objectName: "arenaSelectionSelector"
        Layout.fillWidth: true
        color: "#c9d2df"
        elide: Text.ElideRight
        text: {
            if (!root.session) {
                return "";
            }
            if (String(root.session.selectedByMemberId || "").length === 0) {
                return qsTr("Choose any chart available to everyone.");
            }
            const displayName = String(root.session.selectedByDisplayName || "");
            return displayName.length > 0
                ? qsTr("Selected by %1").arg(displayName)
                : qsTr("Selected by another player");
        }
        textFormat: Text.PlainText
    }

    Text {
        objectName: "arenaSelectionOptions"
        Layout.fillWidth: true
        color: "#d6deea"
        text: root.session ? String(root.session.arenaOptionsSummary || "") : ""
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

    Item {
        Layout.fillHeight: true
        Layout.minimumHeight: 0
    }
}

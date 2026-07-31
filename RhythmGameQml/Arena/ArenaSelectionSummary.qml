import QtQuick
import QtQuick.Layouts
import RhythmGameQml

ColumnLayout {
    id: root

    required property var session
    property var tables: Rg.tables
    property int tableRevision: 0
    property bool compact: false
    property bool fillRemainder: true
    property bool showTitle: true
    property bool titleOnly: false
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
        case "arena.error.roomLibraryChanged":
            return qsTr("The room library changed. Select the chart again.");
        default:
            return "";
        }
    }
    readonly property var result: root.session ? root.session.lastResult : null
    readonly property int winnerCount: {
        const value = root.result;
        return value && value.valid === true && Number(value.participantCount) >= 2
            && value.winnerNames
            ? value.winnerNames.length : 0;
    }
    readonly property string winnerText: {
        const value = root.result;
        if (!value || value.valid !== true || Number(value.participantCount) < 2
                || !value.winnerNames || value.winnerNames.length === 0) {
            return "";
        }
        return value.winnerNames.join(", ");
    }

    ArenaTypography {
        id: typography
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
        elide: root.titleOnly ? Text.ElideRight : Text.ElideNone
        font.bold: true
        font.pixelSize: typography.bodyPixelSize
        maximumLineCount: root.titleOnly ? 2 : 2147483647
        text: root.session ? (root.selectedChartText.length > 0 ? root.selectedChartText : qsTr("No chart selected")) : ""
        textFormat: Text.PlainText
        visible: root.showTitle
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
        font.pixelSize: typography.bodyPixelSize
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
        visible: !root.titleOnly
        wrapMode: Text.Wrap
    }

    Text {
        objectName: "arenaSelectionOptions"
        Layout.fillWidth: true
        color: "#d6deea"
        font.pixelSize: typography.bodyPixelSize
        text: root.session ? String(root.session.arenaOptionsSummary || "") : ""
        textFormat: Text.PlainText
        visible: !root.titleOnly && text.length > 0
        wrapMode: Text.Wrap
    }

    Text {
        objectName: "arenaSelectionSync"
        Accessible.name: text
        Accessible.role: Accessible.StaticText
        Layout.fillWidth: true
        color: "#ffd38a"
        font.pixelSize: typography.bodyPixelSize
        text: root.syncText
        textFormat: Text.PlainText
        visible: !root.titleOnly && text.length > 0
        wrapMode: Text.Wrap
    }

    Text {
        objectName: "arenaLastWinners"
        Layout.fillWidth: true
        color: "#ffe39b"
        font.pixelSize: typography.bodyPixelSize
        text: root.winnerCount > 0
            ? qsTr("Last winner: %1", "Arena last round winner count", root.winnerCount).arg(root.winnerText)
            : ""
        textFormat: Text.PlainText
        visible: !root.titleOnly && text.length > 0
        wrapMode: Text.Wrap
    }

    Item {
        Layout.fillHeight: root.fillRemainder
        Layout.minimumHeight: 0
        visible: root.fillRemainder
    }
}

import QtQuick

Item {
    id: root

    required property bool active
    required property string errorMessageKey
    required property bool reconnecting
    required property string roundLaunchCancellationStatusKey
    required property Item target

    property int announcementCount: 0
    property string lastAnnouncementKey: ""
    property string lastAnnouncementText: ""
    property string activeAnnouncementKey: ""
    readonly property string candidateKey: root.statusKey()
    readonly property string candidateText: root.statusText(root.candidateKey)

    function statusKey(): string {
        if (!root.active) {
            return "";
        }
        if (root.reconnecting) {
            return "arena.status.reconnecting";
        }
        if (root.roundLaunchCancellationStatusKey.length > 0) {
            return root.roundLaunchCancellationStatusKey;
        }
        switch (root.errorMessageKey) {
        case "arena.error.selectionStale":
        case "arena.error.availabilityStale":
        case "arena.error.inventoryStale":
            return "arena.status.selectionInvalidated";
        default:
            return "";
        }
    }

    function statusText(key): string {
        switch (key) {
        case "arena.status.reconnecting":
            return qsTr("Reconnecting to Arena. Your seat is reserved for up to 60 seconds.");
        case "arena.status.selectionInvalidated":
            return qsTr("The room library changed. Select the chart again.");
        case "arena.status.roundLaunchCancelled.missingFile":
            return qsTr("Round preparation was cancelled because the chart file is missing.");
        case "arena.status.roundLaunchCancelled.hashMismatch":
            return qsTr("Round preparation was cancelled because the chart file does not match the selected chart.");
        case "arena.status.roundLaunchCancelled.readFailed":
            return qsTr("Round preparation was cancelled because the chart file could not be read.");
        case "arena.status.roundLaunchCancelled.parseFailed":
            return qsTr("Round preparation was cancelled because the chart file could not be parsed.");
        case "arena.status.roundLaunchCancelled.unsupportedConfig":
            return qsTr("Round preparation was cancelled because the selected play options are not supported.");
        case "arena.status.roundLaunchCancelled.resourceFailed":
            return qsTr("Round preparation was cancelled because required chart resources could not be loaded.");
        case "arena.status.roundLaunchCancelled.probeTimeout":
            return qsTr("Round preparation was cancelled because checking players' chart files timed out.");
        case "arena.status.roundLaunchCancelled.loadTimeout":
            return qsTr("Round preparation was cancelled because loading the chart timed out.");
        case "arena.status.roundLaunchCancelled.participantLeft":
            return qsTr("Round preparation was cancelled because a player left the room.");
        case "arena.status.roundLaunchCancelled.participantKicked":
            return qsTr("Round preparation was cancelled because a player was removed from the room.");
        case "arena.status.roundLaunchCancelled.chartLengthMismatch":
            return qsTr("Round preparation was cancelled because players' chart lengths do not match.");
        case "arena.status.roundLaunchCancelled.serverShutdown":
            return qsTr("Round preparation was cancelled because the Arena server is shutting down.");
        case "arena.status.roundLaunchCancelled.cancelled":
            return qsTr("Round preparation was cancelled.");
        default:
            return "";
        }
    }

    function updateAnnouncement(): void {
        const key = root.candidateKey;
        const text = root.candidateText;
        if (key.length === 0 || text.length === 0) {
            root.activeAnnouncementKey = "";
            return;
        }
        if (key === root.activeAnnouncementKey) {
            return;
        }
        root.activeAnnouncementKey = key;
        root.lastAnnouncementKey = key;
        root.lastAnnouncementText = text;
        root.announcementCount += 1;
        root.target.Accessible.announce(text);
    }

    height: 0
    visible: false
    width: 0

    Component.onCompleted: root.updateAnnouncement()
    onCandidateKeyChanged: root.updateAnnouncement()
    onCandidateTextChanged: root.updateAnnouncement()
}

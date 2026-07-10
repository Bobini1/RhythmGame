import QtQuick

Item {
    id: root

    required property bool active
    required property string errorMessageKey
    required property bool reconnecting
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
        switch (root.errorMessageKey) {
        case "arena.error.selectionStale":
        case "arena.error.availabilityStale":
        case "arena.error.inventoryStale":
            return "arena.status.selectionInvalidated";
        case "arena.error.parseFailed":
        case "arena.error.resourceFailed":
        case "arena.error.missingFile":
        case "arena.error.hashMismatch":
            return "arena.status.roundLoadingCancelled";
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
        case "arena.status.roundLoadingCancelled":
            return qsTr("Round preparation was cancelled because the chart could not be loaded.");
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

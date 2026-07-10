pragma ComponentBehavior: Bound

import QtQuick
import QtQml.Models

Item {
    id: root

    required property bool active
    required property var result
    required property Item target
    required property string winnerSummary
    required property string localStanding

    property int announcementCount: 0
    property string lastAnnouncementKey: ""
    property string lastAnnouncementText: ""
    property var dnfRows: ({})

    readonly property bool rowsReady: root.active && root.result
        && Number(root.result.participantCount || 0) === rowInstantiator.count
    readonly property string dnfSummary: {
        const memberIds = Object.keys(root.dnfRows).sort();
        const summaries = [];
        for (const memberId of memberIds) {
            summaries.push(root.dnfRows[memberId].text);
        }
        return summaries.join(qsTr("; "));
    }
    readonly property string finalAnnouncementText: {
        if (!root.rowsReady) {
            return "";
        }
        if (root.dnfSummary.length > 0) {
            return qsTr("Arena result. %1. Your standing: %2. Did not finish: %3")
                .arg(root.winnerSummary)
                .arg(root.localStanding)
                .arg(root.dnfSummary);
        }
        return qsTr("Arena result. %1. Your standing: %2")
            .arg(root.winnerSummary)
            .arg(root.localStanding);
    }

    function updateDnf(memberId, displayName, competitionState, dnfReason,
                       token) {
        const key = String(memberId || "");
        if (key.length === 0) {
            return;
        }
        const next = Object.assign({}, root.dnfRows);
        if (competitionState === "dnf") {
            next[key] = {
                "text": qsTr("%1: %2").arg(displayName).arg(
                    competitionText.dnfReasonText(dnfReason)),
                "token": token
            };
        } else if (next[key] && next[key].token === token) {
            delete next[key];
        }
        root.dnfRows = next;
    }

    function removeDnf(memberId, token) {
        const key = String(memberId || "");
        if (!root.dnfRows[key] || root.dnfRows[key].token !== token) {
            return;
        }
        const next = Object.assign({}, root.dnfRows);
        delete next[key];
        root.dnfRows = next;
    }

    function announceFinalResult() {
        if (root.finalAnnouncementText.length === 0 || !root.result) {
            return;
        }
        const key = String(root.result.roundId || "") + "\n"
            + root.finalAnnouncementText;
        if (key === root.lastAnnouncementKey) {
            return;
        }
        root.lastAnnouncementKey = key;
        root.lastAnnouncementText = root.finalAnnouncementText;
        root.announcementCount += 1;
        root.target.Accessible.announce(root.lastAnnouncementText);
    }

    height: 0
    visible: false
    width: 0

    onActiveChanged: {
        if (!root.active) {
            root.dnfRows = ({});
        }
    }
    onFinalAnnouncementTextChanged: Qt.callLater(root.announceFinalResult)

    ArenaCompetitionText {
        id: competitionText
    }

    Instantiator {
        id: rowInstantiator

        active: root.active && root.result && root.result.standings
        model: active ? root.result.standings : null

        delegate: QtObject {
            id: rowObserver

            required property string memberId
            required property string displayName
            required property string competitionState
            required property string dnfReason
            readonly property var token: ({})

            function synchronize() {
                root.updateDnf(memberId, displayName, competitionState,
                               dnfReason, token);
            }

            Component.onCompleted: synchronize()
            Component.onDestruction: root.removeDnf(memberId, token)
            onCompetitionStateChanged: synchronize()
            onDisplayNameChanged: synchronize()
            onDnfReasonChanged: synchronize()
            onMemberIdChanged: synchronize()
        }
    }
}

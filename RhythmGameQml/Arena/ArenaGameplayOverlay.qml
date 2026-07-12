pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

Rectangle {
    id: root

    required property var session
    property bool expanded: false
    property int unreadCount: 0
    property int announcementCount: 0
    property string lastAnnouncementKey: ""
    property string lastAnnouncementText: ""
    property string observedRoundId: ""
    property string keyboardStandingMemberId: ""
    property var standingSnapshots: ({})
    readonly property alias dragHandle: gameplayHeader

    Accessible.role: Accessible.Grouping
    Accessible.name: qsTr("Arena live standings")
    Accessible.description: qsTr("Arena live standings")

    ArenaCompetitionText {
        id: competitionText
    }

    function focusStanding(index): void {
        if (standingsView.count <= 0) {
            return;
        }
        const targetIndex = Math.max(0, Math.min(standingsView.count - 1, index));
        standingsView.currentIndex = targetIndex;
        standingsView.positionViewAtIndex(targetIndex, ListView.Contain);
        const currentTarget = standingsView.itemAtIndex(targetIndex);
        if (currentTarget !== null && currentTarget !== undefined) {
            keyboardStandingMemberId = String(currentTarget.memberId || "");
            return;
        }
        Qt.callLater(function () {
            const target = standingsView.itemAtIndex(targetIndex);
            if (standingsView.currentIndex === targetIndex && target !== null && target !== undefined) {
                root.keyboardStandingMemberId = String(target.memberId || "");
            }
        });
    }

    function restoreKeyboardStanding(memberId, index): void {
        if (keyboardStandingMemberId.length === 0 || keyboardStandingMemberId !== String(memberId)) {
            return;
        }
        const targetMemberId = String(memberId);
        const targetIndex = Number(index);
        Qt.callLater(function () {
            if (root.keyboardStandingMemberId !== targetMemberId || targetIndex < 0 || targetIndex >= standingsView.count) {
                return;
            }
            standingsView.currentIndex = targetIndex;
            standingsView.positionViewAtIndex(targetIndex, ListView.Contain);
        });
    }

    function standingAccessibleName(standing): string {
        const markers = [];
        if (standing.localMember) {
            markers.push(qsTr("you"));
        }
        if (standing.opponentTarget) {
            markers.push(qsTr("rival"));
        }
        if (markers.length === 0) {
            return standing.displayName;
        }
        return qsTr("%1, %2").arg(standing.displayName).arg(markers.join(qsTr(", ")));
    }

    function standingAccessibleDescription(standing): string {
        const parts = [];
        parts.push(standing.rank > 0 ? qsTr("Rank %1").arg(standing.rank) : qsTr("Not ranked"));
        parts.push(qsTr("EX %1").arg(scoreText(standing.hasScore, standing.exScore)));
        parts.push(stateText(standing.connected, standing.competitionState));
        parts.push(qsTr("BP %1").arg(standing.badPoorCount));
        parts.push(qsTr("Combo %1").arg(standing.maxCombo));
        if (standing.hasScore) {
            parts.push(standing.currentClear);
        }
        const outcome = outcomeText(standing.clearType, standing.lobbyWinsAfter, standing.dnfReason);
        if (outcome.length > 0) {
            parts.push(outcome);
        }
        return parts.join(qsTr(", "));
    }

    function issueAnnouncement(key, text): void {
        if (text.length === 0 || key === lastAnnouncementKey) {
            return;
        }
        lastAnnouncementKey = key;
        lastAnnouncementText = text;
        announcementCount += 1;
        Accessible.announce(text, Accessible.Polite);
    }

    function observeStanding(memberId, displayName, connected, competitionState, rank, dnfReason): void {
        const roundId = session && session.liveStandings ? String(session.liveStandings.roundId || "") : "";
        if (roundId !== observedRoundId) {
            observedRoundId = roundId;
            standingSnapshots = ({});
            lastAnnouncementKey = "";
        }

        const snapshotKey = "member:" + String(memberId);
        const previous = standingSnapshots[snapshotKey];
        const current = {
            "connected": !!connected,
            "competitionState": String(competitionState || ""),
            "rank": Number(rank),
            "dnfReason": String(dnfReason || "")
        };
        standingSnapshots[snapshotKey] = current;
        if (previous === undefined) {
            return;
        }

        let eventKind = "";
        let message = "";
        if (current.competitionState === "dnf" && current.dnfReason.length > 0 && (previous.competitionState !== current.competitionState || previous.dnfReason !== current.dnfReason)) {
            eventKind = "dnf:" + current.dnfReason;
            message = qsTr("%1 did not finish: %2").arg(displayName).arg(dnfReasonText(current.dnfReason));
        } else if (current.competitionState === "finished" && current.rank === 1 && (previous.competitionState !== current.competitionState || previous.rank !== current.rank)) {
            eventKind = "winner";
            message = qsTr("%1 takes first place").arg(displayName);
        } else if (current.competitionState !== "finished" && current.competitionState !== "dnf" && previous.connected !== current.connected) {
            eventKind = current.connected ? "reconnected" : "disconnected";
            message = current.connected ? qsTr("%1 reconnected").arg(displayName) : qsTr("%1 disconnected").arg(displayName);
        }
        if (eventKind.length > 0) {
            issueAnnouncement(roundId + "|" + snapshotKey + "|" + eventKind, message);
        }
    }

    function stateText(connected, state): string {
        return competitionText.stateText(connected, state);
    }

    function rankText(rank): string {
        return rank > 0 ? String(rank) : "—";
    }

    function scoreText(hasScore, exScore): string {
        return hasScore ? String(exScore) : "—";
    }

    function dnfReasonText(value): string {
        return competitionText.dnfReasonText(value);
    }

    function outcomeText(clearType, lobbyWinsAfter, dnfReason): string {
        return competitionText.outcomeText(clearType, lobbyWinsAfter, dnfReason);
    }

    border.color: "#70ffffff"
    border.width: 1
    color: "#e6101218"
    implicitHeight: 360
    implicitWidth: 420
    radius: 6

    Instantiator {
        id: standingInstantiator

        active: root.session !== null && root.session !== undefined && root.session.liveStandings !== null && root.session.liveStandings !== undefined
        model: standingInstantiator.active ? root.session.liveStandings : null

        delegate: QtObject {
            required property int index
            required property string memberId
            required property string displayName
            required property bool connected
            required property string competitionState
            required property int rank
            required property string dnfReason
            property bool observationReady: false

            function observe(): void {
                if (!observationReady) {
                    return;
                }
                root.observeStanding(memberId, displayName, connected, competitionState, rank, dnfReason);
            }

            function restoreKeyboardFocus(): void {
                root.restoreKeyboardStanding(memberId, index);
            }

            onIndexChanged: restoreKeyboardFocus()
            onDisplayNameChanged: observe()
            onConnectedChanged: observe()
            onCompetitionStateChanged: observe()
            onRankChanged: observe()
            onDnfReasonChanged: observe()
            Component.onCompleted: {
                observationReady = true;
                observe();
                restoreKeyboardFocus();
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            id: gameplayHeader

            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                color: "white"
                font.bold: true
                font.pixelSize: 18
                text: qsTr("Arena")
            }

            Button {
                objectName: "arenaGameplayExpand"
                text: root.expanded ? qsTr("Compact") : qsTr("Expand")
                onClicked: root.expanded = !root.expanded
            }

            Button {
                text: root.session.gameplayChatOpen === true ? qsTr("Close chat") : (root.unreadCount > 0 ? qsTr("Chat (%1)").arg(root.unreadCount) : qsTr("Chat"))
                onClicked: root.session.toggleGameplayChat()
            }
        }

        ListView {
            id: standingsView

            objectName: "arenaGameplayStandings"

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 80
            activeFocusOnTab: true
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            model: root.session.liveStandings
            reuseItems: true
            spacing: 4

            Accessible.role: Accessible.List
            Accessible.name: qsTr("Arena standings")
            Accessible.description: qsTr("Use the arrow keys to review players")
            Accessible.focusable: true

            ScrollBar.vertical: ScrollBar {}

            onActiveFocusChanged: {
                if (activeFocus) {
                    root.focusStanding(Math.max(0, currentIndex));
                }
            }
            Keys.priority: Keys.BeforeItem
            Keys.onPressed: event => {
                if (event.key === Qt.Key_Up) {
                    root.focusStanding(currentIndex - 1);
                } else if (event.key === Qt.Key_Down) {
                    root.focusStanding(currentIndex + 1);
                } else if (event.key === Qt.Key_Home) {
                    root.focusStanding(0);
                } else if (event.key === Qt.Key_End) {
                    root.focusStanding(count - 1);
                } else {
                    return;
                }
                event.accepted = true;
            }

            delegate: Rectangle {
                id: standingDelegate

                required property int index
                required property string memberId
                required property string displayName
                required property bool connected
                required property string competitionState
                required property int rank
                required property bool hasScore
                required property var exScore
                required property int maxCombo
                required property int badPoorCount
                required property int perfect
                required property int great
                required property int good
                required property int bad
                required property int poor
                required property int emptyPoor
                required property string gaugeType
                required property int gaugeValueMilli
                required property string clearType
                required property int lobbyWinsAfter
                required property string dnfReason

                readonly property bool localMember: memberId === String(root.session.selfMemberId || "")
                readonly property bool opponentTarget: root.session.opponentTarget !== null && root.session.opponentTarget !== undefined && memberId === String(root.session.opponentTarget.memberId || "")
                readonly property bool focusIndicatorVisible: activeFocus || (ListView.isCurrentItem && standingsView.activeFocus)
                readonly property string currentClear: competitionText.currentClearText(maxCombo, perfect, great, good, bad, poor, emptyPoor, gaugeType, gaugeValueMilli)

                objectName: "arenaStandingRow" + index
                color: index % 2 === 0 ? "#241b2230" : "#141b2230"
                activeFocusOnTab: false
                border.color: "#8fdcff"
                border.width: focusIndicatorVisible ? 2 : 0
                height: standingContent.implicitHeight + 10
                radius: 3
                width: ListView.view.width

                Accessible.role: Accessible.ListItem
                Accessible.name: root.standingAccessibleName(standingDelegate)
                Accessible.description: root.standingAccessibleDescription(standingDelegate)
                Accessible.focusable: true

                onActiveFocusChanged: {
                    if (activeFocus) {
                        standingsView.currentIndex = index;
                        root.keyboardStandingMemberId = memberId;
                    }
                }

                ColumnLayout {
                    id: standingContent

                    spacing: 2
                    anchors {
                        fill: parent
                        leftMargin: 8
                        rightMargin: 8
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            objectName: "arenaStandingRank"
                            Layout.preferredWidth: 22
                            color: "#b9ffffff"
                            font.bold: true
                            horizontalAlignment: Text.AlignRight
                            text: root.rankText(standingDelegate.rank)
                            textFormat: Text.PlainText

                            Accessible.ignored: true
                        }

                        Text {
                            objectName: "arenaStandingLocalMark"
                            color: "#9ee6ff"
                            font.bold: true
                            text: qsTr("YOU")
                            textFormat: Text.PlainText
                            visible: standingDelegate.localMember

                            Accessible.ignored: true
                        }

                        Text {
                            objectName: "arenaStandingTargetMark"
                            color: "#cbb8ff"
                            font.bold: true
                            text: qsTr("RIVAL")
                            textFormat: Text.PlainText
                            visible: standingDelegate.opponentTarget

                            Accessible.ignored: true
                        }

                        Text {
                            objectName: "arenaStandingName"
                            Layout.fillWidth: true
                            color: "white"
                            elide: Text.ElideRight
                            font.bold: true
                            text: standingDelegate.displayName
                            textFormat: Text.PlainText

                            Accessible.ignored: true
                        }

                        Text {
                            objectName: "arenaStandingScore"
                            color: "#ffe38a"
                            font.bold: true
                            text: qsTr("EX %1").arg(root.scoreText(standingDelegate.hasScore, standingDelegate.exScore))
                            textFormat: Text.PlainText

                            Accessible.ignored: true
                        }

                        Text {
                            objectName: "arenaStandingCurrentClear"
                            Layout.preferredWidth: 104
                            color: "#d8ffffff"
                            horizontalAlignment: Text.AlignRight
                            text: standingDelegate.currentClear
                            textFormat: Text.PlainText

                            Accessible.ignored: true
                        }

                        Text {
                            objectName: "arenaStandingState"
                            Layout.preferredWidth: 82
                            color: standingDelegate.connected ? "#b9ffffff" : "#ff9b9b"
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignRight
                            text: root.stateText(standingDelegate.connected, standingDelegate.competitionState)
                            textFormat: Text.PlainText

                            Accessible.ignored: true
                        }
                    }

                    ColumnLayout {
                        objectName: "arenaStandingDetails"
                        Layout.fillWidth: true
                        spacing: 1
                        visible: root.expanded

                        Text {
                            Layout.fillWidth: true
                            color: "#d8ffffff"
                            text: qsTr("BP %1 · Combo %2").arg(standingDelegate.badPoorCount).arg(standingDelegate.maxCombo)
                            textFormat: Text.PlainText

                            Accessible.ignored: true
                        }

                        Text {
                            Layout.fillWidth: true
                            color: "#d8ffffff"
                            text: qsTr("PG %1 · GR %2 · GD %3").arg(standingDelegate.perfect).arg(standingDelegate.great).arg(standingDelegate.good)
                            textFormat: Text.PlainText

                            Accessible.ignored: true
                        }

                        Text {
                            Layout.fillWidth: true
                            color: "#d8ffffff"
                            text: qsTr("BD %1 · PR %2 · EP %3").arg(standingDelegate.bad).arg(standingDelegate.poor).arg(standingDelegate.emptyPoor)
                            textFormat: Text.PlainText

                            Accessible.ignored: true
                        }
                    }

                    Text {
                        objectName: "arenaStandingOutcome"
                        Layout.fillWidth: true
                        color: standingDelegate.dnfReason.length > 0 ? "#ffb0b0" : "#b9ffffff"
                        elide: Text.ElideRight
                        text: root.outcomeText(standingDelegate.clearType, standingDelegate.lobbyWinsAfter, standingDelegate.dnfReason)
                        textFormat: Text.PlainText
                        visible: text.length > 0

                        Accessible.ignored: true
                    }
                }
            }
        }
    }
}

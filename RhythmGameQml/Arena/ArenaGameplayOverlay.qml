pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

Rectangle {
    id: root

    required property var session
    property bool expanded: false
    property int announcementCount: 0
    property string lastAnnouncementKey: ""
    property string lastAnnouncementText: ""
    property string observedRoundId: ""
    property string keyboardStandingMemberId: ""
    property var activeStandingsView: null
    property var standingSnapshots: ({})
    readonly property alias dragHandle: gameplayHeader
    readonly property bool chatOpen: root.session && root.session.chatOpen === true
    readonly property bool narrowHeader: root.width < 620
    readonly property string roomName: root.session
        ? String(root.session.roomName || qsTr("Arena")) : qsTr("Arena")
    readonly property int totalNotes: {
        const runner = root.session ? root.session.arenaRunner : null;
        const player = runner ? runner.player1 : null;
        const score = player ? player.score : null;
        return score ? Math.max(0, Number(score.maxHits || 0)) : 0;
    }

    Accessible.role: Accessible.Grouping
    Accessible.name: root.roomName
    Accessible.description: root.chatOpen ? qsTr("Arena chat") : qsTr("Arena live standings")

    ArenaCompetitionText {
        id: competitionText
    }

    ArenaTypography {
        id: typography
    }

    function focusStanding(index): void {
        const view = root.activeStandingsView;
        if (!view || view.count <= 0) {
            return;
        }
        const targetIndex = Math.max(0, Math.min(view.count - 1, index));
        view.currentIndex = targetIndex;
        view.positionViewAtIndex(targetIndex, ListView.Contain);
        const currentTarget = view.itemAtIndex(targetIndex);
        if (currentTarget !== null && currentTarget !== undefined) {
            keyboardStandingMemberId = String(currentTarget.memberId || "");
            return;
        }
        Qt.callLater(function () {
            if (root.activeStandingsView !== view) {
                return;
            }
            const target = view.itemAtIndex(targetIndex);
            if (view.currentIndex === targetIndex && target !== null && target !== undefined) {
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
            const view = root.activeStandingsView;
            if (!view || root.keyboardStandingMemberId !== targetMemberId
                    || targetIndex < 0 || targetIndex >= view.count) {
                return;
            }
            view.currentIndex = targetIndex;
            view.positionViewAtIndex(targetIndex, ListView.Contain);
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
        parts.push(qsTr("Combo %1").arg(standing.maxCombo));
        parts.push(qsTr("PG %1, GR %2, GD %3, BD %4, PR %5, EP %6")
            .arg(standing.perfect).arg(standing.great).arg(standing.good)
            .arg(standing.bad).arg(standing.poor).arg(standing.emptyPoor));
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

    function visibleStateText(connected, state): string {
        if (connected && state === "playing") {
            return "";
        }
        return stateText(connected, state);
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

    border.color: "#74859a"
    border.width: 1
    clip: true
    color: "#ed111821"
    implicitHeight: 360
    implicitWidth: 420
    radius: 5

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
        anchors.margins: 10
        spacing: 8

        GridLayout {
            id: gameplayHeader

            columns: 3
            columnSpacing: 4
            Layout.fillWidth: true
            Layout.fillHeight: false
            Layout.minimumHeight: implicitHeight
            Layout.preferredHeight: implicitHeight
            rowSpacing: root.narrowHeader ? 4 : 0

            Item {
                Layout.column: 0
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: Math.max(24, gameplayTitle.implicitHeight)
                Layout.preferredWidth: 24
                Layout.row: 0

                Accessible.ignored: true

                Column {
                    anchors.centerIn: parent
                    spacing: 4

                    Repeater {
                        model: 3

                        Rectangle {
                            color: "#8b96a6"
                            height: 2
                            width: 18
                        }
                    }
                }
            }

            Label {
                id: gameplayTitle

                Layout.column: 1
                Layout.columnSpan: root.narrowHeader ? 2 : 1
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.row: 0
                Layout.alignment: Qt.AlignVCenter
                color: "white"
                elide: Text.ElideRight
                font.bold: true
                font.pixelSize: typography.scaled(18)
                text: root.roomName
                verticalAlignment: Text.AlignVCenter
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                Layout.column: root.narrowHeader ? 0 : 2
                Layout.columnSpan: root.narrowHeader ? 3 : 1
                Layout.fillWidth: root.narrowHeader
                Layout.minimumWidth: 0
                Layout.row: root.narrowHeader ? 1 : 0
                spacing: 0

                Item {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    visible: root.narrowHeader
                }

                Button {
                    id: expandButton

                    objectName: "arenaGameplayExpand"
                    enabled: !root.chatOpen
                    font.pixelSize: typography.bodyPixelSize
                    Layout.preferredHeight: implicitHeight
                    text: root.expanded ? qsTr("Compact") : qsTr("Expand")
                    visible: !root.chatOpen
                    onClicked: root.expanded = !root.expanded
                }

                ArenaPanelTabs {
                    id: gameplayTabs

                    Layout.preferredHeight: implicitHeight
                    chatAccessibleName: qsTr("Show Arena chat")
                    detailsAccessibleName: qsTr("Show Arena standings")
                    session: root.session
                }
            }

            HoverHandler {
                enabled: !expandButton.hovered && !gameplayTabs.hovered
                cursorShape: Qt.SizeAllCursor
            }
        }

        Loader {
            id: contentLoader

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 0
            Layout.minimumWidth: 0
            sourceComponent: root.chatOpen ? chatComponent : standingsComponent
        }

        ArenaChatActivityRail {
            Layout.fillWidth: true
            session: root.session
        }
    }

    Component {
        id: chatComponent

        ArenaChatView {
            objectName: "arenaGameplayChat"
            chatModel: root.session ? root.session.chat : null
            focusFallback: root
            inputEnabled: true
            session: root.session
            unreadCount: root.session ? Number(root.session.unreadChatCount || 0) : 0
        }
    }

    Component {
        id: standingsComponent

        ListView {
            id: standingsView

            objectName: "arenaGameplayStandings"
            activeFocusOnTab: true
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            model: root.session ? root.session.liveStandings : null
            reuseItems: true
            spacing: 4

            Accessible.role: Accessible.List
            Accessible.name: qsTr("Arena standings")
            Accessible.description: qsTr("Use the arrow keys to review players")
            Accessible.focusable: true

            ScrollBar.vertical: ScrollBar {}

            Component.onCompleted: root.activeStandingsView = standingsView
            Component.onDestruction: {
                if (root.activeStandingsView === standingsView) {
                    root.activeStandingsView = null;
                }
            }

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
                readonly property string currentClearLabel: competitionText.liveClearLabel(maxCombo, perfect, great, good, bad, poor, emptyPoor, gaugeType, gaugeValueMilli)
                readonly property bool currentClearShowsGaugeValue: competitionText.liveClearShowsGaugeValue(maxCombo, perfect, great, good, bad, poor, emptyPoor, gaugeType, gaugeValueMilli)
                readonly property string currentClear: competitionText.liveClearText(maxCombo, perfect, great, good, bad, poor, emptyPoor, gaugeType, gaugeValueMilli)
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
                        Layout.minimumWidth: 0
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            spacing: 5

                            Text {
                                objectName: "arenaStandingRank"
                                Layout.preferredWidth: 22
                                color: "#b9ffffff"
                                font.bold: true
                                font.pixelSize: typography.bodyPixelSize
                                horizontalAlignment: Text.AlignRight
                                text: root.rankText(standingDelegate.rank)
                                textFormat: Text.PlainText

                                Accessible.ignored: true
                            }

                            Text {
                                objectName: "arenaStandingLocalMark"
                                color: "#9ee6ff"
                                font.bold: true
                                font.pixelSize: typography.bodyPixelSize
                                text: qsTr("YOU")
                                textFormat: Text.PlainText
                                visible: standingDelegate.localMember

                                Accessible.ignored: true
                            }

                            Text {
                                objectName: "arenaStandingTargetMark"
                                color: "#cbb8ff"
                                font.bold: true
                                font.pixelSize: typography.bodyPixelSize
                                text: qsTr("RIVAL")
                                textFormat: Text.PlainText
                                visible: standingDelegate.opponentTarget

                                Accessible.ignored: true
                            }

                            Text {
                                objectName: "arenaStandingName"
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                color: "white"
                                elide: Text.ElideRight
                                font.bold: true
                                font.pixelSize: typography.bodyPixelSize
                                text: standingDelegate.displayName
                                textFormat: Text.PlainText

                                Accessible.ignored: true
                            }
                        }

                        Text {
                            objectName: "arenaStandingState"
                            Layout.maximumWidth: standingContent.width * 0.4
                            Layout.minimumWidth: 0
                            color: standingDelegate.connected ? "#b9ffffff" : "#ff9b9b"
                            elide: Text.ElideRight
                            font.pixelSize: typography.bodyPixelSize
                            horizontalAlignment: Text.AlignRight
                            text: root.visibleStateText(standingDelegate.connected, standingDelegate.competitionState)
                            textFormat: Text.PlainText
                            visible: text.length > 0

                            Accessible.ignored: true
                        }
                    }

                    GridLayout {
                        id: scoreMetrics

                        readonly property bool wide: standingContent.width
                            >= typography.scaled(250)

                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        columnSpacing: typography.scaled(12)
                        columns: wide ? 3 : 2
                        rowSpacing: 2

                        RowLayout {
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            Layout.column: 0
                            Layout.minimumWidth: 0
                            Layout.row: 0
                            spacing: 5

                            Text {
                                color: "#d9bf72"
                                font.pixelSize: typography.supportingPixelSize
                                text: qsTr("EX")
                                textFormat: Text.PlainText

                                Accessible.ignored: true
                            }

                            Text {
                                objectName: "arenaStandingScore"
                                color: "#ffe38a"
                                font.bold: true
                                font.pixelSize: typography.bodyPixelSize
                                text: root.scoreText(standingDelegate.hasScore,
                                                     standingDelegate.exScore)
                                textFormat: Text.PlainText

                                Accessible.ignored: true
                            }
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            Layout.column: scoreMetrics.wide ? 1 : 0
                            Layout.columnSpan: scoreMetrics.wide ? 1 : 2
                            Layout.minimumWidth: 0
                            Layout.row: scoreMetrics.wide ? 0 : 1
                            spacing: 5
                            visible: root.expanded

                            Text {
                                color: "#9da9b8"
                                font.pixelSize: typography.supportingPixelSize
                                text: qsTr("Combo")
                                textFormat: Text.PlainText

                                Accessible.ignored: true
                            }

                            Text {
                                color: "white"
                                font.bold: true
                                font.pixelSize: typography.bodyPixelSize
                                text: String(standingDelegate.maxCombo)
                                textFormat: Text.PlainText

                                Accessible.ignored: true
                            }
                        }

                        RowLayout {
                            Layout.column: scoreMetrics.wide ? 2 : 1
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.row: 0
                            spacing: 5

                            Text {
                                objectName: "arenaStandingCurrentClear"
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                color: "#d8ffffff"
                                elide: Text.ElideRight
                                font.pixelSize: typography.bodyPixelSize
                                horizontalAlignment: Text.AlignRight
                                text: standingDelegate.currentClearLabel
                                textFormat: Text.PlainText

                                Accessible.ignored: true
                            }

                            Text {
                                objectName: "arenaStandingGaugeValue"
                                Layout.preferredWidth: typography.scaled(48)
                                color: "#d8ffffff"
                                font.pixelSize: typography.bodyPixelSize
                                horizontalAlignment: Text.AlignRight
                                text: competitionText.gaugeValueText(standingDelegate.gaugeValueMilli)
                                textFormat: Text.PlainText
                                visible: standingDelegate.currentClearShowsGaugeValue

                                Accessible.ignored: true
                            }
                        }
                    }

                    ArenaJudgementBreakdown {
                        objectName: "arenaStandingJudgements"
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        bad: standingDelegate.bad
                        emptyPoor: standingDelegate.emptyPoor
                        expanded: root.expanded
                        good: standingDelegate.good
                        great: standingDelegate.great
                        perfect: standingDelegate.perfect
                        poor: standingDelegate.poor
                        totalNotes: root.totalNotes
                    }

                    Text {
                        objectName: "arenaStandingOutcome"
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        color: standingDelegate.dnfReason.length > 0 ? "#ffb0b0" : "#b9ffffff"
                        elide: Text.ElideRight
                        font.pixelSize: typography.bodyPixelSize
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

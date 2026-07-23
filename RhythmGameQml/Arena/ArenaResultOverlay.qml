pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: root

    required property var session
    required property string placementKind
    required property string resolvedSkinId
    required property string layoutVariant
    property bool expanded: true
    readonly property alias dragHandle: resultHeader
    readonly property alias announcementCount: resultAnnouncer.announcementCount
    readonly property alias lastAnnouncementKey: resultAnnouncer.lastAnnouncementKey
    readonly property alias lastAnnouncementText: resultAnnouncer.lastAnnouncementText
    readonly property alias finalAnnouncementText: resultAnnouncer.finalAnnouncementText
    readonly property bool chatOpen: root.session && root.session.chatOpen === true
    readonly property string roomName: root.session
        ? String(root.session.roomName || qsTr("Arena")) : qsTr("Arena")

    readonly property var result: root.session ? root.session.presentedResult : null
    readonly property bool resultAvailable: root.result && root.result.valid === true
    readonly property string statusText: {
        if (!root.resultAvailable) {
            return qsTr("Arena result unavailable");
        }
        return root.result.finalized ? "" : qsTr("Waiting for players…");
    }
    readonly property string winnerSummaryText: {
        if (!root.resultAvailable || !root.result.finalized) {
            return qsTr("Waiting for final standings");
        }
        const names = root.result.winnerNames || [];
        return competitionText.winnersText(names);
    }
    readonly property string localStandingText: {
        const count = root.resultAvailable ? Number(root.result.participantCount || 0) : 0;
        if (root.resultAvailable && root.result.localDnf) {
            return qsTr("DNF / %1").arg(count);
        }
        const rank = root.resultAvailable && root.result.finalized ? Number(root.result.localRank || 0) : 0;
        return rank > 0 ? qsTr("#%1 / %2").arg(rank).arg(count) : qsTr("— / %1").arg(count);
    }
    function rankLabel(rank, state): string {
        if (state === "dnf") {
            return qsTr("DNF");
        }
        return rank > 0 ? qsTr("#%1").arg(rank) : "—";
    }

    function winsLabel(wins): string {
        return competitionText.winsText(wins);
    }

    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Arena result")
    Accessible.description: root.finalAnnouncementText.length > 0 ? root.finalAnnouncementText : root.statusText

    border.color: "#74859a"
    border.width: 1
    color: "#ed111821"
    clip: true
    implicitHeight: 460
    implicitWidth: 520
    radius: 5

    ArenaCompetitionText {
        id: competitionText
    }

    ArenaTypography {
        id: typography
    }

    ArenaResultAnnouncer {
        id: resultAnnouncer

        active: root.resultAvailable && root.result.finalized
        localStanding: root.localStandingText
        result: root.result
        target: root
        winnerSummary: root.winnerSummaryText
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            id: resultHeader

            readonly property real contentHeight: Math.max(
                40, resultTitle.implicitHeight + 12,
                resultStatus.implicitHeight + 12,
                resultExpand.implicitHeight, resultTabs.implicitHeight)

            Layout.fillWidth: true
            Layout.fillHeight: false
            Layout.minimumHeight: contentHeight
            Layout.preferredHeight: contentHeight
            spacing: 4

            Item {
                Layout.fillHeight: true
                Layout.preferredWidth: 24

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
                id: resultTitle

                Layout.fillWidth: true
                Layout.minimumWidth: 0
                color: "white"
                elide: Text.ElideRight
                font.bold: true
                font.pixelSize: typography.scaled(18)
                text: root.roomName
            }

            Label {
                id: resultStatus

                objectName: "arenaResultStatus"
                color: root.resultAvailable && root.result.finalized ? "#a9f5bb" : "#ffe38a"
                font.bold: true
                font.pixelSize: typography.bodyPixelSize
                text: root.statusText
                visible: text.length > 0
            }

            Button {
                id: resultExpand

                objectName: "arenaResultExpand"
                enabled: !root.chatOpen
                font.pixelSize: typography.bodyPixelSize
                text: root.expanded ? qsTr("Compact") : qsTr("Expand")
                visible: !root.chatOpen

                Accessible.name: text
                Accessible.description: root.expanded ? qsTr("Hide detailed Arena standings") : qsTr("Show detailed Arena standings")
                onClicked: root.expanded = !root.expanded
            }

            ArenaPanelTabs {
                id: resultTabs

                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: false
                Layout.preferredHeight: implicitHeight
                chatAccessibleName: qsTr("Show Arena chat")
                detailsAccessibleName: qsTr("Show Arena result")
                session: root.session
            }

            HoverHandler {
                enabled: !resultTabs.hovered
                cursorShape: Qt.SizeAllCursor
            }
        }

        ArenaChatView {
            id: resultChat

            objectName: "arenaResultChat"
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 0
            chatModel: root.session ? root.session.chat : null
            focusFallback: root
            inputEnabled: true
            session: root.session
            visible: root.chatOpen
        }

        Text {
            objectName: "arenaResultWinners"
            Layout.fillWidth: true
            color: "white"
            font.bold: true
            font.pixelSize: typography.scaled(17)
            text: root.winnerSummaryText
            textFormat: Text.PlainText
            visible: !root.chatOpen
            wrapMode: Text.Wrap
        }

        Text {
            objectName: "arenaResultLocalStanding"
            Layout.fillWidth: true
            color: "#ffe38a"
            font.bold: true
            font.pixelSize: typography.scaled(22)
            text: root.localStandingText
            textFormat: Text.PlainText
            visible: !root.chatOpen
        }

        Text {
            Layout.fillWidth: true
            color: "#c9ffffff"
            font.pixelSize: typography.bodyPixelSize
            text: root.resultAvailable ? String(root.result.selectionTitle || "") : ""
            textFormat: Text.PlainText
            elide: Text.ElideRight
            visible: !root.chatOpen
        }

        ListView {
            id: standingsView

            objectName: "arenaResultStandings"
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 0
            activeFocusOnTab: visible && count > 0
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            keyNavigationEnabled: true
            model: root.resultAvailable ? root.result.standings : null
            reuseItems: true
            spacing: 4
            visible: !root.chatOpen

            Accessible.role: Accessible.List
            Accessible.name: qsTr("Arena final standings")

            function ensureCurrentItem() {
                if (count === 0) {
                    currentIndex = -1;
                } else if (currentIndex < 0 || currentIndex >= count) {
                    currentIndex = 0;
                }
            }

            Component.onCompleted: ensureCurrentItem()
            Keys.onPressed: event => {
                if (event.key !== Qt.Key_Home && event.key !== Qt.Key_End) {
                    return;
                }
                currentIndex = event.key === Qt.Key_Home ? 0 : count - 1;
                positionViewAtIndex(currentIndex, ListView.Contain);
                event.accepted = true;
            }
            onCountChanged: ensureCurrentItem()

            ScrollBar.vertical: ScrollBar {}

            delegate: Rectangle {
                id: standingDelegate

                required property int index
                required property string memberId
                required property string displayName
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
                required property int gaugeValueMilli
                required property string clearType
                required property int lobbyWinsAfter
                required property string dnfReason

                readonly property bool localMarkerVisible: memberId === String(root.session.selfMemberId || "")
                readonly property string rankLabel: root.rankLabel(rank, competitionState)
                readonly property string winsLabel: root.winsLabel(lobbyWinsAfter)
                readonly property string detailsLabel: competitionText.resultDetailsText(competitionState, dnfReason, maxCombo, clearType)
                readonly property string finalClearLabel: competitionText.clearTypeText(clearType)
                readonly property string gaugeLabel: competitionText.gaugeValueText(gaugeValueMilli)
                readonly property int totalNotes: Math.max(0, perfect) + Math.max(0, great)
                    + Math.max(0, good) + Math.max(0, bad) + Math.max(0, poor)
                readonly property string accessibleSummary: qsTr("%1, rank %2, score %3, %4").arg(localMarkerVisible ? qsTr("You · %1").arg(displayName) : displayName).arg(rankLabel).arg(hasScore ? String(exScore) : qsTr("No score")).arg(detailsLabel)
                readonly property bool focusIndicatorVisible: ListView.isCurrentItem && standingsView.activeFocus

                objectName: "arenaResultRow-" + memberId
                activeFocusOnTab: false
                color: index % 2 === 0 ? "#2a1b2230" : "#181b2230"
                height: standingContent.implicitHeight + 12
                radius: 3
                width: ListView.view.width

                Accessible.role: Accessible.ListItem
                Accessible.name: accessibleSummary
                Accessible.description: hasScore ? gaugeLabel + qsTr(" · ") + winsLabel : winsLabel
                Accessible.focusable: true
                Accessible.focused: focusIndicatorVisible
                Accessible.selected: ListView.isCurrentItem

                border.color: focusIndicatorVisible ? "#ffe38a" : "transparent"
                border.width: focusIndicatorVisible ? 2 : 0

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
                            Layout.preferredWidth: Math.max(42, implicitWidth)
                            color: standingDelegate.competitionState === "dnf" ? "#ff9b9b" : "#b9ffffff"
                            font.bold: true
                            font.pixelSize: typography.bodyPixelSize
                            horizontalAlignment: Text.AlignRight
                            text: standingDelegate.rankLabel
                            textFormat: Text.PlainText
                        }

                        Text {
                            Layout.fillWidth: true
                            color: "white"
                            elide: Text.ElideRight
                            font.bold: true
                            font.pixelSize: typography.bodyPixelSize
                            text: standingDelegate.localMarkerVisible ? qsTr("You · %1").arg(standingDelegate.displayName) : standingDelegate.displayName
                            textFormat: Text.PlainText
                        }

                        Text {
                            Layout.preferredWidth: Math.max(72, implicitWidth)
                            color: "#c9ffffff"
                            font.pixelSize: typography.bodyPixelSize
                            horizontalAlignment: Text.AlignRight
                            text: standingDelegate.winsLabel
                            textFormat: Text.PlainText
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: 8
                        visible: standingDelegate.hasScore

                        RowLayout {
                            spacing: 5

                            Text {
                                color: "#d9bf72"
                                font.pixelSize: typography.supportingPixelSize
                                text: qsTr("EX")
                                textFormat: Text.PlainText

                                Accessible.ignored: true
                            }

                            Text {
                                color: "#ffe38a"
                                font.bold: true
                                font.pixelSize: typography.bodyPixelSize
                                text: String(standingDelegate.exScore)
                                textFormat: Text.PlainText

                                Accessible.ignored: true
                            }
                        }

                        RowLayout {
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

                        Text {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            color: "#d8ffffff"
                            elide: Text.ElideRight
                            font.pixelSize: typography.bodyPixelSize
                            horizontalAlignment: Text.AlignRight
                            text: standingDelegate.finalClearLabel
                            textFormat: Text.PlainText

                            Accessible.ignored: true
                        }
                    }

                    ArenaJudgementBreakdown {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        bad: standingDelegate.bad
                        emptyPoor: standingDelegate.emptyPoor
                        expanded: root.expanded
                        good: standingDelegate.good
                        great: standingDelegate.great
                        perfect: standingDelegate.perfect
                        poor: standingDelegate.poor
                        totalNotes: standingDelegate.totalNotes
                        visible: standingDelegate.hasScore
                    }

                    Text {
                        Layout.fillWidth: true
                        color: "#ffb0b0"
                        elide: Text.ElideRight
                        font.pixelSize: typography.bodyPixelSize
                        text: standingDelegate.detailsLabel
                        textFormat: Text.PlainText
                        visible: root.expanded && !standingDelegate.hasScore
                    }
                }
            }
        }

        ArenaChatActivityRail {
            Layout.fillWidth: true
            session: root.session
        }
    }
}

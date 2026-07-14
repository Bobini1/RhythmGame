pragma ComponentBehavior: Bound

import RhythmGameQml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: panel

    required property var result
    required property var session
    required property string localMemberId
    required property string statsFontFamily
    required property string textFontFamily
    property bool expanded: true
    readonly property alias dragHandle: resultHeader
    readonly property alias announcementCount: resultAnnouncer.announcementCount
    readonly property alias lastAnnouncementKey: resultAnnouncer.lastAnnouncementKey
    readonly property alias lastAnnouncementText: resultAnnouncer.lastAnnouncementText
    readonly property alias finalAnnouncementText: resultAnnouncer.finalAnnouncementText
    readonly property bool chatOpen: panel.session && panel.session.chatOpen === true
    readonly property string roomName: panel.session
        ? String(panel.session.roomName || qsTr("Arena")) : qsTr("Arena")

    readonly property string winnerSummary: {
        if (!panel.result || !panel.result.valid || !panel.result.finalized) {
            return qsTr("Waiting for final standings");
        }
        const names = panel.result.winnerNames || [];
        return competitionText.winnersText(names);
    }
    readonly property string localStanding: {
        const count = panel.result && panel.result.valid ? Number(panel.result.participantCount || 0) : 0;
        if (panel.result && panel.result.localDnf) {
            return qsTr("DNF / %1").arg(count);
        }
        const rank = panel.result && panel.result.finalized ? Number(panel.result.localRank || 0) : 0;
        return rank > 0 ? qsTr("#%1 / %2").arg(rank).arg(count) : qsTr("— / %1").arg(count);
    }
    function rankText(rank, state): string {
        if (state === "dnf") {
            return qsTr("DNF");
        }
        return rank > 0 ? qsTr("#%1").arg(rank) : "—";
    }

    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Arena result")
    Accessible.description: panel.finalAnnouncementText.length > 0 ? panel.finalAnnouncementText : qsTr("Waiting for final standings")

    color: "#ed111821"
    border.color: "#74859a"
    border.width: 1
    clip: true
    radius: 5

    ArenaCompetitionText {
        id: competitionText
    }

    ArenaTypography {
        id: typography
    }

    ArenaResultAnnouncer {
        id: resultAnnouncer

        active: panel.result && panel.result.valid && panel.result.finalized
        localStanding: panel.localStanding
        result: panel.result
        target: panel
        winnerSummary: panel.winnerSummary
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
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

            Text {
                id: resultTitle

                Layout.fillWidth: true
                Layout.minimumWidth: 0
                color: "white"
                elide: Text.ElideRight
                font.bold: true
                font.contextFontMerging: true
                font.family: panel.textFontFamily
                font.pixelSize: typography.scaled(18)
                text: panel.roomName
                textFormat: Text.PlainText
            }

            Text {
                id: resultStatus

                color: panel.result && panel.result.finalized ? "#a9f5bb" : "#ffe38a"
                font.bold: true
                font.contextFontMerging: true
                font.family: panel.textFontFamily
                font.pixelSize: typography.scaled(14)
                text: qsTr("Waiting for players…")
                textFormat: Text.PlainText
                visible: !panel.result || !panel.result.finalized
            }

            Button {
                id: resultExpand

                objectName: "arenaNativeResultExpand"
                enabled: !panel.chatOpen
                font.pixelSize: typography.bodyPixelSize
                text: panel.expanded ? qsTr("Compact") : qsTr("Expand")
                visible: !panel.chatOpen

                Accessible.name: text
                Accessible.description: panel.expanded ? qsTr("Hide detailed Arena standings") : qsTr("Show detailed Arena standings")
                onClicked: panel.expanded = !panel.expanded
            }

            ArenaPanelTabs {
                id: resultTabs

                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: false
                Layout.preferredHeight: implicitHeight
                chatAccessibleName: qsTr("Show Arena chat")
                detailsAccessibleName: qsTr("Show Arena result")
                session: panel.session
            }

            HoverHandler {
                enabled: !resultTabs.hovered
                cursorShape: Qt.SizeAllCursor
            }
        }

        ArenaChatView {
            objectName: "arenaNativeResultChat"
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 0
            chatModel: panel.session ? panel.session.chat : null
            inputEnabled: true
            session: panel.session
            visible: panel.chatOpen
        }

        Text {
            Layout.fillWidth: true
            color: "white"
            font.bold: true
            font.contextFontMerging: true
            font.family: panel.textFontFamily
            font.pixelSize: typography.scaled(24)
            text: panel.winnerSummary
            textFormat: Text.PlainText
            visible: !panel.chatOpen
            wrapMode: Text.Wrap
        }

        Text {
            Layout.fillWidth: true
            color: "#ffe38a"
            font.bold: true
            font.contextFontMerging: true
            font.family: panel.statsFontFamily
            font.pixelSize: typography.scaled(30)
            text: panel.localStanding
            textFormat: Text.PlainText
            visible: !panel.chatOpen
        }

        ListView {
            id: standingsView

            objectName: "arenaNativeResultStandings"
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 0
            activeFocusOnTab: visible && count > 0
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            keyNavigationEnabled: true
            model: panel.result && panel.result.valid ? panel.result.standings : null
            reuseItems: true
            spacing: 5
            visible: !panel.chatOpen

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
                id: row

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

                readonly property bool local: memberId === panel.localMemberId
                readonly property string rankLabel: panel.rankText(rank, competitionState)
                readonly property string winsLabel: competitionText.winsText(lobbyWinsAfter)
                readonly property string detailsLabel: competitionText.nativeResultDetailsText(competitionState, dnfReason, maxCombo, clearType)
                readonly property string finalClearLabel: competitionText.clearTypeText(clearType)
                readonly property int totalNotes: Math.max(0, perfect) + Math.max(0, great)
                    + Math.max(0, good) + Math.max(0, bad) + Math.max(0, poor)
                readonly property string accessibleSummary: qsTr("%1, rank %2, score %3, %4").arg(local ? qsTr("You · %1").arg(displayName) : displayName).arg(rankLabel).arg(hasScore ? String(exScore) : qsTr("No score")).arg(detailsLabel)
                readonly property bool focusIndicatorVisible: ListView.isCurrentItem && standingsView.activeFocus

                objectName: "arenaNativeResultRow-" + memberId
                activeFocusOnTab: false
                color: index % 2 === 0 ? "#262a38" : "#1c202c"
                height: rowContent.implicitHeight + 14
                radius: 5
                width: ListView.view.width

                Accessible.role: Accessible.ListItem
                Accessible.name: accessibleSummary
                Accessible.description: winsLabel
                Accessible.focusable: true
                Accessible.focused: focusIndicatorVisible
                Accessible.selected: ListView.isCurrentItem

                border.color: focusIndicatorVisible ? "#ffe38a" : "transparent"
                border.width: focusIndicatorVisible ? 2 : 0

                ColumnLayout {
                    id: rowContent

                    spacing: 3
                    anchors {
                        fill: parent
                        leftMargin: 10
                        rightMargin: 10
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            Layout.preferredWidth: Math.max(52, implicitWidth)
                            color: row.competitionState === "dnf" ? "#ff9b9b" : "#d8ffffff"
                            font.bold: true
                            font.contextFontMerging: true
                            font.family: panel.statsFontFamily
                            font.pixelSize: typography.scaled(17)
                            horizontalAlignment: Text.AlignRight
                            text: row.rankLabel
                            textFormat: Text.PlainText
                        }

                        Text {
                            Layout.fillWidth: true
                            color: "white"
                            elide: Text.ElideRight
                            font.bold: true
                            font.contextFontMerging: true
                            font.family: panel.textFontFamily
                            font.pixelSize: typography.scaled(18)
                            text: row.local ? qsTr("You · %1").arg(row.displayName) : row.displayName
                            textFormat: Text.PlainText
                        }

                        Text {
                            Layout.preferredWidth: Math.max(82, implicitWidth)
                            color: "#c9ffffff"
                            font.contextFontMerging: true
                            font.family: panel.textFontFamily
                            font.pixelSize: typography.scaled(16)
                            horizontalAlignment: Text.AlignRight
                            text: row.winsLabel
                            textFormat: Text.PlainText
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: 8
                        visible: row.hasScore

                        RowLayout {
                            spacing: 5

                            Text {
                                color: "#d9bf72"
                                font.contextFontMerging: true
                                font.family: panel.textFontFamily
                                font.pixelSize: typography.supportingPixelSize
                                text: qsTr("EX")
                                textFormat: Text.PlainText

                                Accessible.ignored: true
                            }

                            Text {
                                color: "#ffe38a"
                                font.bold: true
                                font.contextFontMerging: true
                                font.family: panel.statsFontFamily
                                font.pixelSize: typography.bodyPixelSize
                                text: String(row.exScore)
                                textFormat: Text.PlainText

                                Accessible.ignored: true
                            }
                        }

                        RowLayout {
                            spacing: 5
                            visible: panel.expanded

                            Text {
                                color: "#9da9b8"
                                font.contextFontMerging: true
                                font.family: panel.textFontFamily
                                font.pixelSize: typography.supportingPixelSize
                                text: qsTr("Combo")
                                textFormat: Text.PlainText

                                Accessible.ignored: true
                            }

                            Text {
                                color: "white"
                                font.bold: true
                                font.contextFontMerging: true
                                font.family: panel.statsFontFamily
                                font.pixelSize: typography.bodyPixelSize
                                text: String(row.maxCombo)
                                textFormat: Text.PlainText

                                Accessible.ignored: true
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            color: "#d8ffffff"
                            elide: Text.ElideRight
                            font.contextFontMerging: true
                            font.family: panel.textFontFamily
                            font.pixelSize: typography.bodyPixelSize
                            horizontalAlignment: Text.AlignRight
                            text: row.finalClearLabel
                            textFormat: Text.PlainText

                            Accessible.ignored: true
                        }
                    }

                    ArenaJudgementBreakdown {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        bad: row.bad
                        emptyPoor: row.emptyPoor
                        expanded: panel.expanded
                        good: row.good
                        great: row.great
                        perfect: row.perfect
                        poor: row.poor
                        totalNotes: row.totalNotes
                        visible: row.hasScore
                    }

                    Text {
                        Layout.fillWidth: true
                        color: "#ffb0b0"
                        elide: Text.ElideRight
                        font.contextFontMerging: true
                        font.family: panel.textFontFamily
                        font.pixelSize: typography.bodyPixelSize
                        text: row.detailsLabel
                        textFormat: Text.PlainText
                        visible: panel.expanded && !row.hasScore
                    }
                }
            }
        }

        ArenaChatActivityRail {
            Layout.fillWidth: true
            session: panel.session
        }
    }
}

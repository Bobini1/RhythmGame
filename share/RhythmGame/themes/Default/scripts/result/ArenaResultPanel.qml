pragma ComponentBehavior: Bound

import RhythmGameQml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: panel

    required property var result
    required property string localMemberId
    required property string statsFontFamily
    required property string textFontFamily
    property bool expanded: true
    readonly property alias announcementCount: resultAnnouncer.announcementCount
    readonly property alias lastAnnouncementKey: resultAnnouncer.lastAnnouncementKey
    readonly property alias lastAnnouncementText: resultAnnouncer.lastAnnouncementText
    readonly property alias finalAnnouncementText: resultAnnouncer.finalAnnouncementText

    readonly property string winnerSummary: {
        if (!panel.result || !panel.result.valid || !panel.result.finalized) {
            return qsTr("Waiting for final standings");
        }
        const names = panel.result.winnerNames || [];
        return names.length > 0 ? qsTr("Winners: %1").arg(names.join(", ")) : qsTr("No winner");
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

    color: "#e8202430"
    border.color: "#80ffffff"
    border.width: 2
    radius: 10

    ArenaCompetitionText {
        id: competitionText
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
        anchors.margins: 18
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                Layout.fillWidth: true
                color: "white"
                font.bold: true
                font.contextFontMerging: true
                font.family: panel.textFontFamily
                font.pixelSize: 28
                text: qsTr("Arena result")
                textFormat: Text.PlainText
            }

            Text {
                color: panel.result && panel.result.finalized ? "#a9f5bb" : "#ffe38a"
                font.bold: true
                font.contextFontMerging: true
                font.family: panel.textFontFamily
                font.pixelSize: 20
                text: panel.result && panel.result.finalized ? qsTr("Final") : qsTr("Waiting for players…")
                textFormat: Text.PlainText
            }

            Button {
                objectName: "arenaNativeResultExpand"
                text: panel.expanded ? qsTr("Compact") : qsTr("Details")

                Accessible.name: text
                Accessible.description: panel.expanded ? qsTr("Hide detailed Arena standings") : qsTr("Show detailed Arena standings")
                onClicked: panel.expanded = !panel.expanded
            }
        }

        Text {
            Layout.fillWidth: true
            color: "white"
            font.bold: true
            font.contextFontMerging: true
            font.family: panel.textFontFamily
            font.pixelSize: 24
            text: panel.winnerSummary
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
        }

        Text {
            Layout.fillWidth: true
            color: "#ffe38a"
            font.bold: true
            font.contextFontMerging: true
            font.family: panel.statsFontFamily
            font.pixelSize: 30
            text: panel.localStanding
            textFormat: Text.PlainText
        }

        Text {
            Layout.fillWidth: true
            color: "#d8ffffff"
            font.contextFontMerging: true
            font.family: panel.textFontFamily
            font.pixelSize: 18
            text: panel.result ? String(panel.result.selectionOptionsSummary || "") : ""
            textFormat: Text.PlainText
            visible: panel.expanded && text.length > 0
            wrapMode: Text.Wrap
        }

        ListView {
            id: standingsView

            objectName: "arenaNativeResultStandings"
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: panel.expanded ? 120 : 0
            activeFocusOnTab: visible && count > 0
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            keyNavigationEnabled: true
            model: panel.result && panel.result.valid ? panel.result.standings : null
            reuseItems: true
            spacing: 5
            visible: panel.expanded

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

                readonly property bool local: memberId === panel.localMemberId
                readonly property string rankLabel: panel.rankText(rank, competitionState)
                readonly property string winsLabel: competitionText.winsText(lobbyWinsAfter)
                readonly property string detailsLabel: competitionText.nativeResultDetailsText(competitionState, dnfReason, badPoorCount, maxCombo, clearType, gaugeType, gaugeValueMilli)
                readonly property string accessibleSummary: qsTr("%1, rank %2, score %3, %4").arg(local ? qsTr("You · %1").arg(displayName) : displayName).arg(rankLabel).arg(hasScore ? String(exScore) : qsTr("No score")).arg(detailsLabel)
                readonly property bool focusIndicatorVisible:
                    ListView.isCurrentItem && standingsView.activeFocus

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
                            Layout.preferredWidth: 52
                            color: row.competitionState === "dnf" ? "#ff9b9b" : "#d8ffffff"
                            font.bold: true
                            font.contextFontMerging: true
                            font.family: panel.statsFontFamily
                            font.pixelSize: 17
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
                            font.pixelSize: 18
                            text: row.local ? qsTr("You · %1").arg(row.displayName) : row.displayName
                            textFormat: Text.PlainText
                        }

                        Text {
                            color: "#ffe38a"
                            font.bold: true
                            font.contextFontMerging: true
                            font.family: panel.statsFontFamily
                            font.pixelSize: 17
                            text: row.hasScore ? qsTr("EX %1").arg(row.exScore) : "—"
                            textFormat: Text.PlainText
                        }

                        Text {
                            Layout.preferredWidth: 82
                            color: "#c9ffffff"
                            font.contextFontMerging: true
                            font.family: panel.textFontFamily
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignRight
                            text: row.winsLabel
                            textFormat: Text.PlainText
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        color: "#c0ffffff"
                        elide: Text.ElideRight
                        font.contextFontMerging: true
                        font.family: panel.textFontFamily
                        font.pixelSize: 15
                        text: row.detailsLabel
                        textFormat: Text.PlainText
                    }

                    Text {
                        Layout.fillWidth: true
                        color: "#a9ffffff"
                        elide: Text.ElideRight
                        font.contextFontMerging: true
                        font.family: panel.statsFontFamily
                        font.pixelSize: 14
                        text: qsTr("PG %1 · GR %2 · GD %3 · BD %4 · PR %5 · EP %6").arg(row.perfect).arg(row.great).arg(row.good).arg(row.bad).arg(row.poor).arg(row.emptyPoor)
                        textFormat: Text.PlainText
                        visible: row.hasScore
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: !panel.expanded
            Layout.minimumHeight: panel.expanded ? 0 : 1
        }
    }
}

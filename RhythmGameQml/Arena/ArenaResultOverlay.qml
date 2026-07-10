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
    property bool expanded: false
    readonly property alias announcementCount: resultAnnouncer.announcementCount
    readonly property alias lastAnnouncementKey: resultAnnouncer.lastAnnouncementKey
    readonly property alias lastAnnouncementText: resultAnnouncer.lastAnnouncementText
    readonly property alias finalAnnouncementText: resultAnnouncer.finalAnnouncementText

    readonly property var result: root.session ? root.session.presentedResult : null
    readonly property bool resultAvailable: root.result && root.result.valid === true
    readonly property string statusText: {
        if (!root.resultAvailable) {
            return qsTr("Arena result unavailable");
        }
        return root.result.finalized ? qsTr("Final") : qsTr("Waiting for players…");
    }
    readonly property string winnerSummaryText: {
        if (!root.resultAvailable || !root.result.finalized) {
            return qsTr("Waiting for final standings");
        }
        const names = root.result.winnerNames || [];
        return names.length > 0 ? qsTr("Winners: %1").arg(names.join(", ")) : qsTr("No winner");
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

    border.color: "#70ffffff"
    border.width: 1
    color: "#ee101218"
    implicitHeight: 460
    implicitWidth: 520
    radius: 6

    ArenaCompetitionText {
        id: competitionText
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
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                color: "white"
                font.bold: true
                font.pixelSize: 18
                text: qsTr("Arena result")
            }

            Label {
                objectName: "arenaResultStatus"
                color: root.resultAvailable && root.result.finalized ? "#a9f5bb" : "#ffe38a"
                font.bold: true
                text: root.statusText
            }

            Button {
                objectName: "arenaResultExpand"
                text: root.expanded ? qsTr("Compact") : qsTr("Details")

                Accessible.name: text
                Accessible.description: root.expanded ? qsTr("Hide detailed Arena standings") : qsTr("Show detailed Arena standings")
                onClicked: root.expanded = !root.expanded
            }
        }

        Text {
            objectName: "arenaResultWinners"
            Layout.fillWidth: true
            color: "white"
            font.bold: true
            font.pixelSize: 17
            text: root.winnerSummaryText
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
        }

        Text {
            objectName: "arenaResultLocalStanding"
            Layout.fillWidth: true
            color: "#ffe38a"
            font.bold: true
            font.pixelSize: 22
            text: root.localStandingText
            textFormat: Text.PlainText
        }

        Text {
            Layout.fillWidth: true
            color: "#c9ffffff"
            text: root.resultAvailable ? String(root.result.selectionTitle || "") : ""
            textFormat: Text.PlainText
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            color: "#b9ffffff"
            text: root.resultAvailable ? String(root.result.selectionOptionsSummary || "") : ""
            textFormat: Text.PlainText
            visible: root.expanded && text.length > 0
            wrapMode: Text.Wrap
        }

        ListView {
            id: standingsView

            objectName: "arenaResultStandings"
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: root.expanded ? 96 : 0
            activeFocusOnTab: visible && count > 0
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            keyNavigationEnabled: true
            model: root.resultAvailable ? root.result.standings : null
            reuseItems: true
            spacing: 4
            visible: root.expanded

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

                readonly property bool localMarkerVisible: memberId === String(root.session.selfMemberId || "")
                readonly property string rankLabel: root.rankLabel(rank, competitionState)
                readonly property string winsLabel: root.winsLabel(lobbyWinsAfter)
                readonly property string detailsLabel: competitionText.resultDetailsText(competitionState, dnfReason, badPoorCount, maxCombo, clearType)
                readonly property string gaugeLabel: competitionText.gaugeText(gaugeType, gaugeValueMilli)
                readonly property string accessibleSummary: qsTr("%1, rank %2, score %3, %4").arg(localMarkerVisible ? qsTr("You · %1").arg(displayName) : displayName).arg(rankLabel).arg(hasScore ? String(exScore) : qsTr("No score")).arg(detailsLabel)
                readonly property bool focusIndicatorVisible:
                    ListView.isCurrentItem && standingsView.activeFocus

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
                            Layout.preferredWidth: 42
                            color: standingDelegate.competitionState === "dnf" ? "#ff9b9b" : "#b9ffffff"
                            font.bold: true
                            horizontalAlignment: Text.AlignRight
                            text: standingDelegate.rankLabel
                            textFormat: Text.PlainText
                        }

                        Text {
                            Layout.fillWidth: true
                            color: "white"
                            elide: Text.ElideRight
                            font.bold: true
                            text: standingDelegate.localMarkerVisible ? qsTr("You · %1").arg(standingDelegate.displayName) : standingDelegate.displayName
                            textFormat: Text.PlainText
                        }

                        Text {
                            color: "#ffe38a"
                            font.bold: true
                            text: standingDelegate.hasScore ? qsTr("EX %1").arg(standingDelegate.exScore) : "—"
                            textFormat: Text.PlainText
                        }

                        Text {
                            Layout.preferredWidth: 72
                            color: "#c9ffffff"
                            horizontalAlignment: Text.AlignRight
                            text: standingDelegate.winsLabel
                            textFormat: Text.PlainText
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        color: "#b9ffffff"
                        text: standingDelegate.detailsLabel
                        textFormat: Text.PlainText
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        color: "#a9ffffff"
                        text: qsTr("PG %1 · GR %2 · GD %3 · BD %4 · PR %5 · EP %6").arg(standingDelegate.perfect).arg(standingDelegate.great).arg(standingDelegate.good).arg(standingDelegate.bad).arg(standingDelegate.poor).arg(standingDelegate.emptyPoor)
                        textFormat: Text.PlainText
                        elide: Text.ElideRight
                        visible: standingDelegate.hasScore
                    }

                    Text {
                        Layout.fillWidth: true
                        color: "#a9ffffff"
                        text: standingDelegate.gaugeLabel
                        textFormat: Text.PlainText
                        visible: standingDelegate.hasScore
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: !root.expanded
            Layout.minimumHeight: root.expanded ? 0 : 1
        }
    }
}

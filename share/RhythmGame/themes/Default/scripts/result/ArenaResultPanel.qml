pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: panel

    required property var result
    required property string localMemberId
    required property string fontFamily
    property bool expanded: true

    readonly property string winnerSummary: {
        if (!panel.result || !panel.result.valid || !panel.result.finalized) {
            return qsTr("Waiting for final standings");
        }
        const names = panel.result.winnerNames || [];
        return names.length > 0
            ? qsTr("Winners: %1").arg(names.join(", "))
            : qsTr("No winner");
    }
    readonly property string localStanding: {
        const count = panel.result && panel.result.valid
            ? Number(panel.result.participantCount || 0) : 0;
        if (panel.result && panel.result.localDnf) {
            return qsTr("DNF / %1").arg(count);
        }
        const rank = panel.result && panel.result.finalized
            ? Number(panel.result.localRank || 0) : 0;
        return rank > 0
            ? qsTr("#%1 / %2").arg(rank).arg(count)
            : qsTr("— / %1").arg(count);
    }

    function rankText(rank, state) : string {
        if (state === "dnf") {
            return qsTr("DNF");
        }
        return rank > 0 ? qsTr("#%1").arg(rank) : "—";
    }

    color: "#e8202430"
    border.color: "#80ffffff"
    border.width: 2
    radius: 10

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
                font.family: panel.fontFamily
                font.pixelSize: 28
                text: qsTr("Arena result")
                textFormat: Text.PlainText
            }

            Text {
                color: panel.result && panel.result.finalized
                    ? "#a9f5bb" : "#ffe38a"
                font.bold: true
                font.family: panel.fontFamily
                font.pixelSize: 20
                text: panel.result && panel.result.finalized
                    ? qsTr("Final") : qsTr("Waiting for players…")
                textFormat: Text.PlainText
            }

            Button {
                text: panel.expanded ? qsTr("Compact") : qsTr("Details")
                onClicked: panel.expanded = !panel.expanded
            }
        }

        Text {
            Layout.fillWidth: true
            color: "white"
            font.bold: true
            font.family: panel.fontFamily
            font.pixelSize: 24
            text: panel.winnerSummary
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
        }

        Text {
            Layout.fillWidth: true
            color: "#ffe38a"
            font.bold: true
            font.family: panel.fontFamily
            font.pixelSize: 30
            text: panel.localStanding
            textFormat: Text.PlainText
        }

        Text {
            Layout.fillWidth: true
            color: "#d8ffffff"
            font.family: panel.fontFamily
            font.pixelSize: 18
            text: panel.result ? String(panel.result.selectionOptionsSummary
                                        || "") : ""
            textFormat: Text.PlainText
            visible: panel.expanded && text.length > 0
            wrapMode: Text.Wrap
        }

        ListView {
            id: standingsView

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: panel.expanded ? 120 : 0
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            model: panel.result && panel.result.valid
                ? panel.result.standings : null
            reuseItems: true
            spacing: 5
            visible: panel.expanded

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

                color: index % 2 === 0 ? "#262a38" : "#1c202c"
                height: rowContent.implicitHeight + 14
                radius: 5
                width: ListView.view.width

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
                            color: row.competitionState === "dnf"
                                ? "#ff9b9b" : "#d8ffffff"
                            font.bold: true
                            font.family: panel.fontFamily
                            font.pixelSize: 17
                            horizontalAlignment: Text.AlignRight
                            text: panel.rankText(row.rank,
                                                 row.competitionState)
                            textFormat: Text.PlainText
                        }

                        Text {
                            Layout.fillWidth: true
                            color: "white"
                            elide: Text.ElideRight
                            font.bold: true
                            font.family: panel.fontFamily
                            font.pixelSize: 18
                            text: row.local
                                ? qsTr("You · %1").arg(row.displayName)
                                : row.displayName
                            textFormat: Text.PlainText
                        }

                        Text {
                            color: "#ffe38a"
                            font.bold: true
                            font.family: panel.fontFamily
                            font.pixelSize: 17
                            text: row.hasScore
                                ? qsTr("EX %1").arg(row.exScore) : "—"
                            textFormat: Text.PlainText
                        }

                        Text {
                            Layout.preferredWidth: 82
                            color: "#c9ffffff"
                            font.family: panel.fontFamily
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignRight
                            text: row.lobbyWinsAfter >= 0
                                ? qsTr("Wins %1").arg(row.lobbyWinsAfter)
                                : qsTr("Wins —")
                            textFormat: Text.PlainText
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        color: "#c0ffffff"
                        elide: Text.ElideRight
                        font.family: panel.fontFamily
                        font.pixelSize: 15
                        text: row.competitionState === "dnf"
                            ? qsTr("Did not finish · %1").arg(row.dnfReason)
                            : qsTr("BP %1 · Combo %2 · %3 · %4 %5%")
                                  .arg(row.badPoorCount)
                                  .arg(row.maxCombo)
                                  .arg(row.clearType)
                                  .arg(row.gaugeType)
                                  .arg((row.gaugeValueMilli / 1000).toFixed(1))
                        textFormat: Text.PlainText
                    }

                    Text {
                        Layout.fillWidth: true
                        color: "#a9ffffff"
                        elide: Text.ElideRight
                        font.family: panel.fontFamily
                        font.pixelSize: 14
                        text: qsTr("PG %1 · GR %2 · GD %3 · BD %4 · PR %5 · EP %6")
                            .arg(row.perfect)
                            .arg(row.great)
                            .arg(row.good)
                            .arg(row.bad)
                            .arg(row.poor)
                            .arg(row.emptyPoor)
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

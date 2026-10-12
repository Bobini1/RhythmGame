import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import RhythmGameQml

GridLayout {
    id: stats
    required property var score
    columns: width < 720 ? 1 : 3
    columnSpacing: 24
    rowSpacing: 24

    // [judgement-counts]
    ColumnLayout {
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        Layout.alignment: Qt.AlignTop

        Label { text: qsTr("Judgements"); font.bold: true }
        Repeater {
            model: [
                { judgement: Judgement.Perfect, text: qsTr("PGREAT"), color: "#9de6ff" },
                { judgement: Judgement.Great, text: qsTr("GREAT"), color: "#ffe28a" },
                { judgement: Judgement.Good, text: qsTr("GOOD"), color: "#a8dcc9" },
                { judgement: Judgement.Bad, text: qsTr("BAD"), color: "#ef9ad0" },
                { judgement: Judgement.Poor, text: qsTr("POOR"), color: "#ef7777" },
                { judgement: Judgement.EmptyPoor, text: qsTr("EMPTY POOR"), color: "#ef7777" },
                { judgement: Judgement.MineHit, text: qsTr("MINE"), color: "#ef7777" }
            ]

            RowLayout {
                id: judgementRow
                required property var modelData
                Layout.fillWidth: true

                Label {
                    Layout.fillWidth: true
                    text: judgementRow.modelData.text
                    color: judgementRow.modelData.color
                }
                Label {
                    text: stats.score.result.judgementCounts[judgementRow.modelData.judgement] ?? 0
                }
            }
        }
    }
    // [judgement-counts]

    HitDistribution {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 1
        Layout.alignment: Qt.AlignTop
        replayData: stats.score.replayData
    }

    LifeGraph {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 1
        Layout.alignment: Qt.AlignTop
        score: stats.score
    }
}

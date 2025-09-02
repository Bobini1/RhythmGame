import QtQml
import QtQuick
import QtQuick.Layouts
import RhythmGameQml
import "../common/helpers.js" as Helpers

Item {
    id: judgementCountsContainer

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.6
    }
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: "grey"
        border.width: 2
    }

    property var score

    Repeater {
        id: judgementCountsModel

        model: judgementCountsContainer.score.judgementCounts
        delegate: Item {
            required property var judgement
            required property int count
        }
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        clip: true
        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            horizontalAlignment: Text.AlignRight
            color: "white"
            fontSizeMode: Text.VerticalFit
            textFormat: Text.PlainText
            minimumPixelSize: 3
            font.pixelSize: 60
            text: Helpers.getGrade(judgementCountsContainer.score.points, judgementCountsContainer.score.maxPointsNow)
            font.capitalization: Font.AllUppercase
        }
        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            horizontalAlignment: Text.AlignRight
            color: "white"
            fontSizeMode: Text.VerticalFit
            textFormat: Text.PlainText
            minimumPixelSize: 3
            font.pixelSize: 60
            text: {
                let points = judgementCountsContainer.score.points;
                let maxPoints = judgementCountsContainer.score.maxPointsNow;
                let ratio = width / height;
                let digits = 0;
                if (ratio > 2.5) {
                    digits = 1;
                }
                if (ratio > 2.9) {
                    digits = 2;
                }
                if (maxPoints > 0) {
                    return (points * 100 / maxPoints).toFixed(digits) + "%";
                } else {
                    return (100).toFixed(digits) + "%";
                }
            }
            font.capitalization: Font.AllUppercase
        }
        Repeater {
            id: judgementCounts

            model: 6
            readonly property var modelWide: ["PERFECT", "GREAT",  "GOOD",  "BAD", "E. POOR", "POOR"]
            readonly property var modelNarrow: ["PG", "GR",  "GD",  "BD", "EP", "PR"]

            delegate: RowLayout {
                id: judgementRow
                required property int index
                required property string modelData
                Layout.fillWidth: true
                Layout.fillHeight: true

                Text {
                    color: "white"
                    fontSizeMode: Text.VerticalFit
                    textFormat: Text.PlainText
                    minimumPixelSize: 3
                    font.pixelSize: 60
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 2
                    text: {
                        if (width / height < 3) {
                            return judgementCounts.modelNarrow[index];
                        } else {
                            return judgementCounts.modelWide[index];
                        }
                    }
                }

                Text {
                    color: "white"
                    fontSizeMode: Text.VerticalFit
                    textFormat: Text.PlainText
                    minimumPixelSize: 3
                    font.pixelSize: 60
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    horizontalAlignment: Text.AlignRight
                    Layout.preferredWidth: 1
                    text: {
                        let row = (judgementCountsModel.count, judgementCountsModel.itemAt(judgementCounts.modelWide.length - judgementRow.index - 1));
                        if (!row) {
                            return "";
                        }
                        return row.count;
                    }
                }
            }
        }
    }
}
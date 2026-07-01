import QtQuick
import QtQuick.Layouts
import RhythmGameQml
import "../common"

RowLayout {
    id: statsDelegate
    anchors.fill: parent
    anchors.margins: 28
    anchors.topMargin: 54
    spacing: 10
    property var clearCounts
    property int playerCount

    ThemeFont {
        id: rankingStatsFont
        fileName: root.themeVars.rankingFont
        fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
    }

    readonly property real percentLaneWidth: 56
    readonly property real reservedPercentWidth: Math.min(percentLaneWidth, percentWidthMetrics.width)

    TextMetrics {
        id: percentWidthMetrics
        font.family: rankingStatsFont.fontFamily
        font.weight: rankingStatsFont.fontWeight
        font.variableAxes: rankingStatsFont.variableAxes
        font.italic: rankingStatsFont.italic
        font.pixelSize: 12
        text: "100.0%"
    }

    component StatLineItem: Item {
        id: statLineItem
        property alias labelSource: label.source
        property alias text1: textOne.text
        property alias text2: textTwo.text
        readonly property real percentLaneWidth: statsDelegate.percentLaneWidth
        readonly property real countPercentGap: 5
        readonly property real reservedPercentWidth: statsDelegate.reservedPercentWidth
        height: label.height
        width: parent ? parent.width : 188
        Image {
            id: label
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            id: textOne
            anchors.left: label.right
            anchors.leftMargin: 5
            anchors.right: parent.right
            anchors.rightMargin: statLineItem.reservedPercentWidth + statLineItem.countPercentGap
            anchors.verticalCenter: parent.verticalCenter
            height: parent.height
            font.family: rankingStatsFont.fontFamily
            font.weight: rankingStatsFont.boldFontWeight
            font.variableAxes: rankingStatsFont.boldVariableAxes
            font.italic: rankingStatsFont.italic
            font.pixelSize: 21
            fontSizeMode: Text.HorizontalFit
            horizontalAlignment: Text.AlignRight
            minimumPixelSize: 8
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            id: textTwo
            font.pixelSize: 12
            width: statLineItem.percentLaneWidth
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: parent.height
            font.family: rankingStatsFont.fontFamily
            font.weight: rankingStatsFont.fontWeight
            font.variableAxes: rankingStatsFont.variableAxes
            font.italic: rankingStatsFont.italic
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 7
        }
    }
    Column {
        spacing: 7
        Layout.fillWidth: true
        Layout.preferredWidth: 1

        Repeater {
            model: ["MAX", "PERFECT", "FC", "EXHARD", "HARD", "NORMAL", "EASY"]
            delegate: StatLineItem {
                labelSource: root.iniImagesUrl + "parts.png/ranking_" + modelData
                text1: statsDelegate.clearCounts[modelData] ?? 0
                text2: statsDelegate.playerCount > 0 ? ((statsDelegate.clearCounts[modelData] ?? 0) / statsDelegate.playerCount * 100).toFixed(1) + "%" : "0.0%"
            }
        }
    }
    Column {
        spacing: 7
        Layout.fillWidth: true
        Layout.preferredWidth: 1

        StatLineItem {
            id: playerCountItem
            labelSource: root.iniImagesUrl + "parts.png/ranking_PLAYER"
            text1: statsDelegate.playerCount
        }
        StatLineItem {
            labelSource: root.iniImagesUrl + "parts.png/ranking_FULLCOMBO"
            readonly property int count: (statsDelegate.clearCounts["FC"] ?? 0) + (statsDelegate.clearCounts["PERFECT"] ?? 0) + (statsDelegate.clearCounts["MAX"] ?? 0)
            text1: count
            text2: statsDelegate.playerCount > 0 ? (count / statsDelegate.playerCount * 100).toFixed(1) + "%" : "0.0%"
        }
        StatLineItem {
            labelSource: root.iniImagesUrl + "parts.png/ranking_CLEAR"
            readonly property int count: {
                let total = 0;
                for (let key in statsDelegate.clearCounts) {
                    if (key !== "NOPLAY" && key !== "FAILED") {
                        total += statsDelegate.clearCounts[key] ?? 0;
                    }
                }
                return total;
            }
            text1: count
            text2: statsDelegate.playerCount > 0 ? (count / statsDelegate.playerCount * 100).toFixed(1) + "%" : "0.0%"
        }
        StatLineItem {
            height: playerCountItem.height
        }
        Repeater {
            model: ["AEASY", "FAILED", "NOPLAY"]
            delegate: StatLineItem {
                labelSource: root.iniImagesUrl + "parts.png/ranking_" + modelData
                text1: statsDelegate.clearCounts[modelData] ?? 0
                text2: statsDelegate.playerCount > 0 ? ((statsDelegate.clearCounts[modelData] ?? 0) / statsDelegate.playerCount * 100).toFixed(1) + "%" : "0.0%"
            }
        }
    }
}

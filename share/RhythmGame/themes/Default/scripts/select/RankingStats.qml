import QtQuick
import QtQuick.Layouts
import RhythmGameQml

RowLayout {
    id: statsDelegate
    anchors.fill: parent
    anchors.margins: 28
    anchors.topMargin: 54
    spacing: 10
    property var clearCounts
    property int playerCount

    component StatLineItem: Item {
        id: statLineItem
        property alias labelSource: label.source
        property alias text1: textOne.text
        property alias text2: textTwo.text
        height: label.height
        width: 188
        Image {
            id: label
        }
        Text {
            id: textOne
            anchors.right: textTwo.left
            anchors.rightMargin: 5
            anchors.baseline: label.bottom
            font.pixelSize: 21
            anchors.baselineOffset: -1
        }
        Text {
            id: textTwo
            font.pixelSize: 12
            width: 37
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignBottom
            anchors.right: parent.right
            anchors.baseline: label.bottom
            font.bold: true
            anchors.baselineOffset: -1
        }
    }
    Column {
        spacing: 7

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
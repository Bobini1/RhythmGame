import RhythmGameQml
import QtQuick
import "../common"

Image {
    id: selector

    property var currentItem: null
    property bool scrollingText: false

    ThemeFont {
        id: selectorFont
        fileName: root.themeVars.songInfoFont
    }

    source: root.iniImagesUrl + "folders.png/frame"

    Loader {
        id: loader

        active: currentItem instanceof ChartData
        anchors.fill: parent

        sourceComponent: Component {
            Item {
                readonly property int infoRowHeight: 36
                readonly property int infoFontSize: 34
                readonly property int infoValueWidth: 96
                readonly property int artistWidth: 280

                anchors.bottomMargin: 10
                anchors.fill: parent
                anchors.leftMargin: 50

                Item {
                    id: infoRows

                    anchors.left: parent.left
                    anchors.right: rankImage.left
                    anchors.rightMargin: 20
                    anchors.top: artistInfo.top
                    anchors.topMargin: (artist.height - parent.infoRowHeight) / 2
                    height: parent.infoRowHeight * 2

                    readonly property real valueWidth: Math.max(28, Math.min(parent.infoValueWidth, Math.max(bpmValueMetrics.width, keysValueMetrics.width) + 4))
                    readonly property real labelWidth: Math.max(0, width - valueWidth - 4)

                    TextMetrics {
                        id: bpmValueMetrics

                        font: bpmValue.font
                        text: bpmValue.text
                    }
                    TextMetrics {
                        id: keysValueMetrics

                        font: keysValue.font
                        text: keysValue.text
                    }

                    Item {
                        id: bpmRow

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        height: infoRows.parent.infoRowHeight

                        Text {
                            id: bpmText

                            clip: true
                            font.pixelSize: infoRows.parent.infoFontSize
                            font.family: selectorFont.fontFamily
                            font.weight: selectorFont.fontWeight
                            font.italic: selectorFont.italic
                            fontSizeMode: Text.Fit
                            height: parent.height
                            horizontalAlignment: Text.AlignLeft
                            maximumLineCount: 1
                            minimumPixelSize: 5
                            text: qsTr("BPM")
                            verticalAlignment: Text.AlignVCenter
                            width: infoRows.labelWidth
                            wrapMode: Text.NoWrap
                        }
                        Text {
                            id: bpmValue

                            anchors.left: bpmText.right
                            anchors.leftMargin: 4
                            clip: true
                            font.pixelSize: infoRows.parent.infoFontSize
                            font.family: selectorFont.fontFamily
                            font.weight: selectorFont.fontWeight
                            font.italic: selectorFont.italic
                            fontSizeMode: Text.Fit
                            height: parent.height
                            horizontalAlignment: Text.AlignHCenter
                            maximumLineCount: 1
                            minimumPixelSize: 5
                            text: currentItem.initialBpm
                            verticalAlignment: Text.AlignVCenter
                            width: infoRows.valueWidth
                            wrapMode: Text.NoWrap
                        }
                    }
                    Item {
                        id: keysRow

                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: infoRows.parent.infoRowHeight
                        y: rankImage.y + rankImage.height / 2 - infoRows.y - height / 2

                        Text {
                            id: keysText

                            clip: true
                            font.pixelSize: infoRows.parent.infoFontSize
                            font.family: selectorFont.fontFamily
                            font.weight: selectorFont.fontWeight
                            font.italic: selectorFont.italic
                            fontSizeMode: Text.Fit
                            height: parent.height
                            horizontalAlignment: Text.AlignLeft
                            maximumLineCount: 1
                            minimumPixelSize: 5
                            text: qsTr("KEYS")
                            verticalAlignment: Text.AlignVCenter
                            width: infoRows.labelWidth
                            wrapMode: Text.NoWrap
                        }
                        Text {
                            id: keysValue

                            anchors.left: keysText.right
                            anchors.leftMargin: 4
                            clip: true
                            font.pixelSize: infoRows.parent.infoFontSize
                            font.family: selectorFont.fontFamily
                            font.weight: selectorFont.fontWeight
                            font.italic: selectorFont.italic
                            fontSizeMode: Text.Fit
                            height: parent.height
                            horizontalAlignment: Text.AlignHCenter
                            maximumLineCount: 1
                            minimumPixelSize: 5
                            text: currentItem.keymode
                            verticalAlignment: Text.AlignVCenter
                            width: infoRows.valueWidth
                            wrapMode: Text.NoWrap
                        }
                    }
                }
                Image {
                    id: rankImage

                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 3
                    anchors.right: artistInfo.left
                    anchors.rightMargin: 8
                    source: {
                        let rank = currentItem.rank;
                        if (rank < 25) {
                            return root.iniImagesUrl + "parts.png/r_over_hard";
                        } else if (rank === 25) {
                            return root.iniImagesUrl + "parts.png/r_very_hard";
                        } else if (rank < 50) {
                            return root.iniImagesUrl + "parts.png/r_more_hard";
                        } else if (rank === 50) {
                            return root.iniImagesUrl + "parts.png/r_hard";
                        } else if (rank < 75) {
                            return root.iniImagesUrl + "parts.png/r_little_hard";
                        } else if (rank === 75) {
                            return root.iniImagesUrl + "parts.png/r_normal";
                        } else if (rank < 100) {
                            return root.iniImagesUrl + "parts.png/r_little_easy";
                        } else if (rank === 100) {
                            return root.iniImagesUrl + "parts.png/r_easy";
                        } else {
                            return root.iniImagesUrl + "parts.png/r_over_easy";
                        }
                    }
                }
                Column {
                    id: artistInfo

                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 5
                    anchors.right: parent.right
                    anchors.rightMargin: 60
                    spacing: 5

                    NameLabel {
                        id: artist

                        anchors.right: parent.right
                        fontFile: root.themeVars.songInfoFont
                        font.pixelSize: 20
                        height: metrics.height
                        scrolling: selector.scrollingText
                        text: currentItem.artist
                        width: artistInfo.parent.artistWidth
                    }
                    NameLabel {
                        id: subartist

                        anchors.right: parent.right
                        fontFile: root.themeVars.songInfoFont
                        font.pixelSize: 15
                        height: metricsSmall.height
                        scrolling: selector.scrollingText
                        text: currentItem.subartist
                        width: artistInfo.parent.artistWidth
                    }
                    TextMetrics {
                        id: metrics

                        font: artist.font
                        text: "z"
                    }
                    TextMetrics {
                        id: metricsSmall

                        font: subartist.font
                        text: "z"
                    }
                }
            }
        }
    }
}

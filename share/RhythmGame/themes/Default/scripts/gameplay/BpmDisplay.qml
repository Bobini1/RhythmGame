import QtQuick
import QtQuick.Layouts
import "../common"

Item {
    id: bpmDisplay

    property real currentBpm: 0
    property real minBpm: 0
    property real maxBpm: 0
    property string fontFile: "file:NotoSansJP-VariableFont_wght.ttf"
    property bool contentVisible: true

    ThemeFont {
        id: bpmDisplayFont
        fileName: bpmDisplay.fontFile
    }

    Column {
        anchors.fill: parent
        spacing: 1
        visible: bpmDisplay.contentVisible

        // Labels row: MIN | BPM | MAX
        Row {
            id: labelRow
            width: parent.width
            height: parent.height * 0.38

            // Reference measurements at a known size (bold = worst-case width)
            FontMetrics {
                id: labelFm
                font.pixelSize: 100
                font.family: bpmDisplayFont.fontFamily
                font.weight: bpmDisplayFont.boldFontWeight
                font.italic: bpmDisplayFont.italic
            }
            // Height-limited: 65 % of row height
            readonly property real heightFs: height * 0.65
            // Width-limited: scale so "BPM" (bold, widest) fits inside one third
            readonly property real widthFs: (width / 3) / labelFm.advanceWidth("BPM") * 100
            readonly property real fs: Math.max(6, Math.min(heightFs, widthFs))

            Text {
                width: labelRow.width / 3; height: labelRow.height
                text: qsTr("MIN")
                font.pixelSize: labelRow.fs
                font.family: bpmDisplayFont.fontFamily
                font.weight: bpmDisplayFont.fontWeight
                font.italic: bpmDisplayFont.italic
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
            Text {
                width: labelRow.width / 3; height: labelRow.height
                text: qsTr("BPM")
                font.pixelSize: labelRow.fs
                font.family: bpmDisplayFont.fontFamily
                font.weight: bpmDisplayFont.boldFontWeight
                font.italic: bpmDisplayFont.italic
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
            Text {
                width: labelRow.width / 3; height: labelRow.height
                text: qsTr("MAX")
                font.pixelSize: labelRow.fs
                font.family: bpmDisplayFont.fontFamily
                font.weight: bpmDisplayFont.fontWeight
                font.italic: bpmDisplayFont.italic
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
        }

        // Numbers row: minBpm | currentBpm | maxBpm
        Row {
            id: numberRow
            width: parent.width
            height: parent.height * 0.55
            readonly property real smallFs: Math.max(6, height * 0.75)
            readonly property real bigFs:   Math.max(6, height * 0.9)

            Text {
                width: numberRow.width / 3; height: numberRow.height
                text: Math.round(bpmDisplay.minBpm)
                font.pixelSize: numberRow.smallFs
                font.family: bpmDisplayFont.fontFamily
                font.weight: bpmDisplayFont.fontWeight
                font.italic: bpmDisplayFont.italic
                fontSizeMode: Text.Fit; minimumPixelSize: 6
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
            Text {
                width: numberRow.width / 3; height: numberRow.height
                text: Math.round(bpmDisplay.currentBpm)
                font.pixelSize: numberRow.bigFs
                font.family: bpmDisplayFont.fontFamily
                font.weight: bpmDisplayFont.boldFontWeight
                font.italic: bpmDisplayFont.italic
                fontSizeMode: Text.Fit; minimumPixelSize: 6
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
            Text {
                width: numberRow.width / 3; height: numberRow.height
                text: Math.round(bpmDisplay.maxBpm)
                font.pixelSize: numberRow.smallFs
                font.family: bpmDisplayFont.fontFamily
                font.weight: bpmDisplayFont.fontWeight
                font.italic: bpmDisplayFont.italic
                fontSizeMode: Text.Fit; minimumPixelSize: 6
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
        }
    }
}

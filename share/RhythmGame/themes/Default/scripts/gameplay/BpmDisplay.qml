import QtQuick
import QtQuick.Layouts
import "../common"

Item {
    id: bpmDisplay

    property real currentBpm: 0
    property real minBpm: 0
    property real maxBpm: 0
    property string fontFile: "file:NotoSans-VariableFont_wdth,wght.ttf"
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
                font: bpmDisplayFont.uiFont({
                    pixelSize: 100,
                    weight: bpmDisplayFont.boldFontWeight,
                    variableAxes: bpmDisplayFont.boldVariableAxes,
                    italic: bpmDisplayFont.italic
                })
            }
            // Height-limited: 65 % of row height
            readonly property real heightFs: height * 0.65
            // Width-limited: scale so "BPM" (bold, widest) fits inside one third
            readonly property real widthFs: (width / 3) / labelFm.advanceWidth("BPM") * 100
            readonly property real fs: Math.max(6, Math.min(heightFs, widthFs))

            Text {
                width: labelRow.width / 3; height: labelRow.height
                text: qsTr("MIN")
                font: bpmDisplayFont.uiFont({
                    pixelSize: labelRow.fs,
                    weight: bpmDisplayFont.fontWeight,
                    variableAxes: bpmDisplayFont.variableAxes,
                    italic: bpmDisplayFont.italic
                })
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
            Text {
                width: labelRow.width / 3; height: labelRow.height
                text: qsTr("BPM")
                font: bpmDisplayFont.uiFont({
                    pixelSize: labelRow.fs,
                    weight: bpmDisplayFont.boldFontWeight,
                    variableAxes: bpmDisplayFont.boldVariableAxes,
                    italic: bpmDisplayFont.italic
                })
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
            Text {
                width: labelRow.width / 3; height: labelRow.height
                text: qsTr("MAX")
                font: bpmDisplayFont.uiFont({
                    pixelSize: labelRow.fs,
                    weight: bpmDisplayFont.fontWeight,
                    variableAxes: bpmDisplayFont.variableAxes,
                    italic: bpmDisplayFont.italic
                })
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
                font: bpmDisplayFont.uiFont({
                    pixelSize: numberRow.smallFs,
                    weight: bpmDisplayFont.fontWeight,
                    variableAxes: bpmDisplayFont.variableAxes,
                    italic: bpmDisplayFont.italic
                })
                fontSizeMode: Text.Fit; minimumPixelSize: 6
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
            Text {
                width: numberRow.width / 3; height: numberRow.height
                text: Math.round(bpmDisplay.currentBpm)
                font: bpmDisplayFont.uiFont({
                    pixelSize: numberRow.bigFs,
                    weight: bpmDisplayFont.boldFontWeight,
                    variableAxes: bpmDisplayFont.boldVariableAxes,
                    italic: bpmDisplayFont.italic
                })
                fontSizeMode: Text.Fit; minimumPixelSize: 6
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
            Text {
                width: numberRow.width / 3; height: numberRow.height
                text: Math.round(bpmDisplay.maxBpm)
                font: bpmDisplayFont.uiFont({
                    pixelSize: numberRow.smallFs,
                    weight: bpmDisplayFont.fontWeight,
                    variableAxes: bpmDisplayFont.variableAxes,
                    italic: bpmDisplayFont.italic
                })
                fontSizeMode: Text.Fit; minimumPixelSize: 6
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
            }
        }
    }
}

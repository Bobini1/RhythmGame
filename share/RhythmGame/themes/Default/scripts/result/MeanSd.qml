import QtQuick
import QtQuick.Layouts
import "../common/helpers.js" as Helpers
import "../common"

WindowBg {
    id: meanSd

    required property real stddev
    required property real mean

    ThemeFont {
        id: meanSdFont
        fileName: root.themeVars.resultStatsFont
        fallbackFileName: "file:NotoSans-VariableFont_wdth,wght.ttf"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        anchors.leftMargin: 36
        anchors.rightMargin: 36
        RowLayout {
            spacing: 13
            Rectangle {
                width: 120
                height: 24
                radius: 12
                color: "#4B4B4B"
                Text {
                    anchors.centerIn: parent
                    text: qsTr("MEAN")
                    font: meanSdFont.uiFont({
                        weight: meanSdFont.boldFontWeight,
                        variableAxes: meanSdFont.boldVariableAxes,
                        italic: meanSdFont.italic,
                        pixelSize: 16
                    })
                    color: "white"
                }
                Layout.alignment: Qt.AlignVCenter
            }
            ResultNumberText {
                text: {
                    let num = (meanSd.mean / 1000000).toFixed(1) + " ms"
                    let sign = meanSd.mean > 0 ? "+" : "";
                    return sign + num;
                }
                font: meanSdFont.uiFont({
                    weight: meanSdFont.fontWeight,
                    variableAxes: meanSdFont.variableAxes,
                    italic: meanSdFont.italic,
                    pixelSize: 24
                })
                Layout.fillHeight: true
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
            }
        }
        RowLayout {
            spacing: 13
            Rectangle {
                width: 120
                height: 24
                radius: 12
                color: "#4B4B4B"
                Text {
                    anchors.centerIn: parent
                    text: qsTr("SD")
                    font: meanSdFont.uiFont({
                        weight: meanSdFont.boldFontWeight,
                        variableAxes: meanSdFont.boldVariableAxes,
                        italic: meanSdFont.italic,
                        pixelSize: 16
                    })
                    color: "white"
                }
                Layout.alignment: Qt.AlignVCenter
            }
            ResultNumberText {
                text: (meanSd.stddev / 1000000).toFixed(1) + " ms"
                font: meanSdFont.uiFont({
                    weight: meanSdFont.fontWeight,
                    variableAxes: meanSdFont.variableAxes,
                    italic: meanSdFont.italic,
                    pixelSize: 24
                })
                Layout.fillHeight: true
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}


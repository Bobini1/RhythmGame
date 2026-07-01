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
        fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
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
                    font.family: meanSdFont.fontFamily
                    font.weight: meanSdFont.boldFontWeight
                    font.italic: meanSdFont.italic
                    color: "white"
                    font.pixelSize: 16
                }
                Layout.alignment: Qt.AlignVCenter
            }
            ResultNumberText {
                text: {
                    let num = (meanSd.mean / 1000000).toFixed(1) + " ms"
                    let sign = meanSd.mean > 0 ? "+" : "";
                    return sign + num;
                }
                font.family: meanSdFont.fontFamily
                font.weight: meanSdFont.fontWeight
                font.italic: meanSdFont.italic
                font.pixelSize: 24
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
                    font.family: meanSdFont.fontFamily
                    font.weight: meanSdFont.boldFontWeight
                    font.italic: meanSdFont.italic
                    color: "white"
                    font.pixelSize: 16
                }
                Layout.alignment: Qt.AlignVCenter
            }
            ResultNumberText {
                text: (meanSd.stddev / 1000000).toFixed(1) + " ms"
                font.family: meanSdFont.fontFamily
                font.weight: meanSdFont.fontWeight
                font.italic: meanSdFont.italic
                font.pixelSize: 24
                Layout.fillHeight: true
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}


import QtQuick
import QtQuick.Layouts
import "../common/helpers.js" as Helpers

WindowBg {
    id: meanSd

    required property real stddev
    required property real mean

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
                    font.bold: true
                    color: "white"
                    font.pixelSize: 16
                }
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: {
                    let num = (meanSd.mean / 1000000).toFixed(1) + " ms"
                    let sign = meanSd.mean > 0 ? "+" : "";
                    return sign + num;
                }
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
                    font.bold: true
                    color: "white"
                    font.pixelSize: 16
                }
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: (meanSd.stddev / 1000000).toFixed(1) + " ms"
                font.pixelSize: 24
                Layout.fillHeight: true
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}


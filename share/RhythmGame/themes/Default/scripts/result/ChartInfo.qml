import QtQuick
import QtQuick.Layouts
import RhythmGameQml

WindowBg {
    id: chartInfo
    required property var difficulty
    required property var total
    required property int noteCount
    Image {
        anchors.left: parent.left
        anchors.top: parent.top
        source: {
            switch (chartInfo.difficulty) {
            case 1:
                return root.iniImagesUrl + "parts.png/beginner_diff";
            case 2:
                return root.iniImagesUrl + "parts.png/normal_diff";
            case 3:
                return root.iniImagesUrl + "parts.png/hyper_diff";
            case 4:
                return root.iniImagesUrl + "parts.png/another_diff";
            case 5:
                return root.iniImagesUrl + "parts.png/insane_diff";
            default:
                return root.iniImagesUrl + "parts.png/unknown_diff";
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.bottomMargin: 24
        anchors.topMargin: 40
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
                    text: qsTr("NOTES")
                    font.bold: true
                    color: "white"
                    font.pixelSize: 16
                }
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: chartInfo.noteCount
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
                    text: qsTr("TOTAL")
                    font.bold: true
                    color: "white"
                    font.pixelSize: 16
                }
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: chartInfo.total || "-"
                font.pixelSize: 24
                Layout.fillHeight: true
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}

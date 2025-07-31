import QtQuick
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
                return root.iniImagesUrl + "parts.png/beginner";
            case 2:
                return root.iniImagesUrl + "parts.png/normal";
            case 3:
                return root.iniImagesUrl + "parts.png/hyper";
            case 4:
                return root.iniImagesUrl + "parts.png/another";
            case 5:
                return root.iniImagesUrl + "parts.png/insane";
            default:
                return root.iniImagesUrl + "parts.png/unknown";
            }
        }
    }
    Text {
        id: totalLabel

        anchors.baseline: parent.top
        anchors.baselineOffset: 60
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -60
        color: "white"
        font.pixelSize: 20
        text: qsTr("TOTAL")

        Rectangle {
            anchors.centerIn: parent
            color: "#4b4b4b"
            height: 20
            radius: 10
            width: 90
            z: -1
        }
    }
    Text {
        anchors.baseline: totalLabel.baseline
        anchors.right: parent.right
        anchors.rightMargin: 36
        font.pixelSize: 20
        text: chartInfo.total || "-"
    }
    Text {
        id: noteCountLabel

        anchors.baseline: parent.top
        anchors.baselineOffset: 90
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -60
        color: "white"
        font.pixelSize: 20
        text: qsTr("NOTES")

        Rectangle {
            anchors.centerIn: parent
            color: "#4b4b4b"
            height: 20
            radius: 10
            width: 90
            z: -1
        }
    }
    Text {
        anchors.baseline: noteCountLabel.baseline
        anchors.right: parent.right
        anchors.rightMargin: 36
        font.pixelSize: 20
        text: chartInfo.noteCount
    }
}

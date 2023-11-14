import QtQuick
import RhythmGameQml

Item {
    id: statLine

    property alias img: comboImg.source
    property bool invertDeltaColor: false
    property int newVal: 0
    property int oldVal: 0

    height: comboImg.sourceSize.height

    Image {
        id: comboImg

        anchors.bottom: parent.bottom
        anchors.left: parent.left
    }
    Text {
        id: oldText

        anchors.baseline: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 120
        color: "lightgray"
        font.pixelSize: 25
        horizontalAlignment: Text.AlignRight
        text: "0000".slice(0, Math.max(0, 4 - statLine.oldVal.toString().length)) + "<font color='black'>" + statLine.oldVal + "</font>"
    }
    Image {
        id: arrow

        anchors.left: oldText.right
        anchors.leftMargin: 30
        anchors.verticalCenter: oldText.verticalCenter
        source: root.iniImagesUrl + "parts.png/arrow"
    }
    Text {
        id: newText

        anchors.baseline: parent.bottom
        anchors.left: arrow.right
        anchors.leftMargin: 30
        color: "lightgray"
        font.pixelSize: 34
        horizontalAlignment: Text.AlignLeft
        text: "0000".slice(0, Math.max(0, 4 - statLine.newVal.toString().length)) + "<font color='black'>" + statLine.newVal + "</font>"
    }
    Text {
        id: deltaText

        anchors.baseline: parent.bottom
        anchors.left: newText.right
        anchors.leftMargin: 20
        color: (statLine.newVal > statLine.oldVal) ^ !invertDeltaColor ? "FireBrick" : "darkgreen"
        font.pixelSize: 25
        text: {
            var delta = statLine.newVal - statLine.oldVal;
            if (delta > 0) {
                "+" + delta;
            } else if (delta < 0) {
                "–" + (-delta);
            } else {
                "";
            }
        }
    }
}

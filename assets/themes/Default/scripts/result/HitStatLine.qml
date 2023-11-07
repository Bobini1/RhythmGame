import QtQuick
import RhythmGameQml

Item {
    id: hitStatLine

    property int earlyCount: 0
    property alias img: judgementImage.source
    property int judgementCount: 0
    property int lateCount: 0

    height: judgementImage.sourceSize.height
    width: parent.width

    Image {
        id: judgementImage

        anchors.left: parent.left
        anchors.leftMargin: 36
    }
    Text {
        id: count

        anchors.baseline: parent.bottom
        anchors.left: judgementImage.right
        anchors.leftMargin: 40
        color: "lightgray"
        font.pixelSize: 34
        horizontalAlignment: Text.AlignRight
        text: "0000".slice(0, 4 - hitStatLine.judgementCount.toString().length) + "<font color='black'>" + hitStatLine.judgementCount + "</font>"
    }
    Text {
        id: earlyLate

        anchors.baseline: parent.bottom
        anchors.left: count.right
        anchors.leftMargin: 68
        color: "lightgray"
        font.pixelSize: 25
        horizontalAlignment: Text.AlignLeft
        text: {
            let earlyLate = root.earlyLate;
            let early = "0000".slice(0, 4 - hitStatLine.earlyCount.toString().length) + "<font color='black'>" + hitStatLine.earlyCount + "/</font>";
            let late = "0000".slice(0, 4 - hitStatLine.lateCount.toString().length) + "<font color='black'>" + hitStatLine.lateCount + "</font>";
            return early + late;
        }
    }
}

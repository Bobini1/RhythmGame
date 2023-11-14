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
        anchors.right: judgementImage.right
        anchors.rightMargin: -120
        color: "lightgray"
        font.pixelSize: 34
        horizontalAlignment: Text.AlignRight
        text: "0000".slice(0, Math.max(0, 4 - hitStatLine.judgementCount.toString().length)) + "<font color='black'>" + hitStatLine.judgementCount + "</font>"
    }
    Text {
        id: earlyLate

        anchors.baseline: parent.bottom
        anchors.horizontalCenter: judgementImage.right
        anchors.horizontalCenterOffset: 250
        color: "lightgray"
        font.pixelSize: 25
        horizontalAlignment: Text.AlignHCenter
        text: {
            let len = Math.max(hitStatLine.earlyCount.toString().length, hitStatLine.lateCount.toString().length, 4);
            let zeroes = "";
            for (let i = 0; i < len; i++) {
                zeroes += "0";
            }
            let early = zeroes.slice(0, Math.max(0, len - hitStatLine.earlyCount.toString().length)) + "<font color='black'>" + hitStatLine.earlyCount + "/</font>";
            let late = zeroes.slice(0, Math.max(0, len - hitStatLine.lateCount.toString().length)) + "<font color='black'>" + hitStatLine.lateCount + "</font>";
            return early + late;
        }
    }
}

import QtQuick
import RhythmGameQml
import "../common"

Item {
    id: hitStatLine

    property int earlyCount: 0
    property alias img: judgementImage.source
    property int judgementCount: 0
    property int lateCount: 0

    ThemeFont {
        id: hitStatLineFont
        fileName: root.themeVars.resultStatsFont
        fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
    }

    height: judgementImage.sourceSize.height
    width: parent.width

    Image {
        id: judgementImage

        anchors.left: parent.left
        anchors.leftMargin: 36
    }
    ResultNumberText {
        id: count

        anchors.baseline: parent.bottom
        anchors.right: judgementImage.right
        anchors.rightMargin: -120
        color: "lightgray"
        font.family: hitStatLineFont.fontFamily
        font.weight: hitStatLineFont.fontWeight
        font.variableAxes: hitStatLineFont.variableAxes
        font.italic: hitStatLineFont.italic
        font.pixelSize: 34
        horizontalAlignment: Text.AlignRight
        text: "0000".slice(0, Math.max(0, 4 - hitStatLine.judgementCount.toString().length)) + "<font color='black'>" + hitStatLine.judgementCount + "</font>"
    }
    ResultNumberText {
        id: earlyLate

        anchors.baseline: parent.bottom
        anchors.horizontalCenter: judgementImage.right
        anchors.horizontalCenterOffset: 250
        color: "lightgray"
        font.family: hitStatLineFont.fontFamily
        font.weight: hitStatLineFont.fontWeight
        font.variableAxes: hitStatLineFont.variableAxes
        font.italic: hitStatLineFont.italic
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

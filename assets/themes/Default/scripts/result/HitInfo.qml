import QtQuick
import RhythmGameQml

WindowBg {
    id: hitInfo

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 36
        anchors.verticalCenter: parent.verticalCenter

        Column {
            spacing: 20
            width: 460

            StatLine {
                img: root.iniImagesUrl + "parts.png/miss_count"
                invertDeltaColor: true
                newVal: result.result.judgementCounts[Judgement.Poor] + result.result.judgementCounts[Judgement.EmptyPoor] + result.result.judgementCounts[Judgement.Bad]
                oldVal: root.oldBestStats ? root.oldBestStats.missCount : 0
                width: parent.width
            }
            StatLine {
                img: root.iniImagesUrl + "parts.png/combo"
                newVal: result.result.maxCombo
                oldVal: root.oldBestStats ? root.oldBestStats.maxCombo : 0
                width: parent.width
            }
            StatLine {
                img: root.iniImagesUrl + "parts.png/combo_break"
                invertDeltaColor: true
                newVal: result.result.judgementCounts[Judgement.Poor] + result.result.judgementCounts[Judgement.Bad]
                oldVal: root.oldBestStats ? root.oldBestStats.comboBreak : 0
                width: parent.width
            }
        }
    }
}

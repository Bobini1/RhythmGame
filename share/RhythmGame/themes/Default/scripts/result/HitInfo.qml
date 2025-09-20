import QtQuick
import RhythmGameQml

WindowBg {
    id: hitInfo
    
    required property var judgementCounts
    required property var oldBestStats
    required property int maxCombo


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
                newVal: hitInfo.judgementCounts[Judgement.Poor] + hitInfo.judgementCounts[Judgement.EmptyPoor] + hitInfo.judgementCounts[Judgement.Bad]
                oldVal: hitInfo.oldBestStats ? hitInfo.oldBestStats.missCount : 0
                width: parent.width
            }
            StatLine {
                img: root.iniImagesUrl + "parts.png/combo"
                newVal: hitInfo.maxCombo
                oldVal: hitInfo.oldBestStats ? hitInfo.oldBestStats.maxCombo : 0
                width: parent.width
            }
            StatLine {
                img: root.iniImagesUrl + "parts.png/combo_break"
                invertDeltaColor: true
                newVal: hitInfo.judgementCounts[Judgement.Poor] + hitInfo.judgementCounts[Judgement.Bad]
                oldVal: hitInfo.oldBestStats ? hitInfo.oldBestStats.comboBreak : 0
                width: parent.width
            }
        }
    }
}

import QtQuick
import RhythmGameQml

WindowBg {
    id: hitStats

    required property var judgementCounts
    required property var earlyLate

    Item {
        id: hitStatHeader

        anchors.left: parent.left
        anchors.leftMargin: 450
        anchors.top: parent.top
        anchors.topMargin: 36
        height: childrenRect.height

        Image {
            id: early

            anchors.left: parent.left
            source: root.iniImagesUrl + "parts.png/early"
        }
        Image {
            id: late

            anchors.left: early.right
            anchors.leftMargin: -155
            source: root.iniImagesUrl + "parts.png/late"
        }
    }
    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: hitStatHeader.bottom
        spacing: 18

        Repeater {
            model: [Judgement.Perfect, Judgement.Great, Judgement.Good, Judgement.Bad, Judgement.Poor, Judgement.EmptyPoor]

            HitStatLine {
                earlyCount: hitStats.earlyLate.early[modelData]
                img: {
                    switch (modelData) {
                    case Judgement.Perfect:
                        return root.iniImagesUrl + "parts.png/j_perfect";
                    case Judgement.Great:
                        return root.iniImagesUrl + "parts.png/j_great";
                    case Judgement.Good:
                        return root.iniImagesUrl + "parts.png/j_good";
                    case Judgement.Bad:
                        return root.iniImagesUrl + "parts.png/j_bad";
                    case Judgement.Poor:
                        return root.iniImagesUrl + "parts.png/j_poor";
                    case Judgement.EmptyPoor:
                        return root.iniImagesUrl + "parts.png/j_empty_poor";
                    }
                }
                judgementCount: hitStats.judgementCounts[modelData]
                lateCount: hitStats.earlyLate.late[modelData]
            }
        }
    }
}

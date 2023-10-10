import QtQuick
import RhythmGameQml

Image {
    id: gradeImage

    function getGrade(points, maxPoints) {
        if (points === maxPoints) {
            return "max";
        }
        let percent = points / maxPoints;
        if (percent >= 0.88) {
            return "aaa";
        } else if (percent >= 0.77) {
            return "aa";
        } else if (percent >= 0.66) {
            return "a";
        } else if (percent >= 0.55) {
            return "b";
        } else if (percent >= 0.44) {
            return "c";
        } else if (percent >= 0.33) {
            return "d";
        } else if (percent >= 0.22) {
            return "e";
        } else {
            return "f";
        }
    }

    anchors.centerIn: parent
    anchors.verticalCenterOffset: 90
    asynchronous: true
    source: root.iniImagesUrl + "parts.png/" + getGrade(root.scoreWithBestPoints.points, root.scoreWithBestPoints.maxPoints)
}

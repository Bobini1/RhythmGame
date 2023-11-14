import QtQuick
import RhythmGameQml
import "../common/helpers.js" as Helpers

Image {
    id: gradeImage

    anchors.centerIn: parent
    anchors.verticalCenterOffset: 90
    asynchronous: true
    source: root.iniImagesUrl + "parts.png/" + Helpers.getGrade(root.scoreWithBestPoints.points, root.scoreWithBestPoints.maxPoints)
}

import QtQuick
import RhythmGameQml
import "../common/helpers.js" as Helpers

Image {
    id: gradeImage

    required property BmsScore scoreWithBestPoints

    anchors.centerIn: parent
    anchors.verticalCenterOffset: 90
    asynchronous: true
    source: gradeImage.scoreWithBestPoints ? root.iniImagesUrl + "parts.png/" + Helpers.getGrade(gradeImage.scoreWithBestPoints.points, gradeImage.scoreWithBestPoints.maxPoints) : ""
}

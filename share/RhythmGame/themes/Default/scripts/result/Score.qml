
import QtQuick
import "../common/helpers.js" as Helpers
import "../common"

WindowBg {
    id: score

    required property real points
    required property real maxPoints
    required property real oldBestPoints
    property string importedSource: ""

    ThemeFont {
        id: scoreFont
        fileName: root.themeVars.resultStatsFont
        fallbackFileName: "file:NotoSans-VariableFont_wdth,wght.ttf"
    }

    Text {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 36
        anchors.rightMargin: 36
        anchors.topMargin: 20
        horizontalAlignment: Text.AlignRight
        color: "#20242c"
        font: scoreFont.uiFont({ pixelSize: 14, weight: Font.DemiBold })
        text: qsTr("IMPORTED · %1").arg(score.importedSource)
        textFormat: Text.PlainText
        elide: Text.ElideRight
        visible: score.importedSource.length > 0
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 36
        anchors.verticalCenter: parent.verticalCenter

        Column {
            spacing: 20
            width: 460

            Item {
                height: exScoreImg.sourceSize.height
                width: parent.width

                Image {
                    id: exScoreImg

                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    source: root.iniImagesUrl + "parts.png/ex_score"
                }
                ResultNumberText {
                    id: exScoreText

                    anchors.baseline: parent.bottom
                    anchors.right: parent.right
                    anchors.rightMargin: 120
                    color: "lightgray"
                    font: scoreFont.uiFont({
                        weight: scoreFont.fontWeight,
                        variableAxes: scoreFont.variableAxes,
                        italic: scoreFont.italic,
                        pixelSize: 41
                    })
                    horizontalAlignment: Text.AlignRight
                    text: {
                        return "00000".slice(0, Math.max(0, 5 - score.points.toString().length)) + "<font color='DeepPink'>" + points + "</font>";
                    }
                }
                ResultNumberText {
                    id: scoreRate

                    anchors.baseline: parent.bottom
                    anchors.leftMargin: 30
                    anchors.right: parent.right
                    font: scoreFont.uiFont({
                        weight: scoreFont.fontWeight,
                        variableAxes: scoreFont.variableAxes,
                        italic: scoreFont.italic,
                        pixelSize: 25
                    })
                    horizontalAlignment: Text.AlignRight
                    text: (score.maxPoints ? (score.points / score.maxPoints * 100).toFixed(2) : "0.00") + "%"
                }
            }
            Item {
                height: hiScoreImg.sourceSize.height
                width: parent.width

                Image {
                    id: hiScoreImg

                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    source: root.iniImagesUrl + "parts.png/hi_score"
                }
                ResultNumberText {
                    id: hiScoreText

                    anchors.baseline: parent.bottom
                    anchors.right: parent.right
                    anchors.rightMargin: 120
                    color: "lightgray"
                    font: scoreFont.uiFont({
                        weight: scoreFont.fontWeight,
                        variableAxes: scoreFont.variableAxes,
                        italic: scoreFont.italic,
                        pixelSize: 34
                    })
                    horizontalAlignment: Text.AlignRight
                    text: {
                        let points = score.oldBestPoints;
                        return "00000".slice(0, Math.max(0, 5 - points.toString().length)) + "<font color='DeepPink'>" + points + "</font>";
                    }
                }
                ResultNumberText {
                    id: hiScoreDelta

                    anchors.baseline: parent.bottom
                    anchors.left: hiScoreText.right
                    anchors.leftMargin: 41
                    color: {
                        let delta = score.points - score.oldBestPoints;
                        return delta > 0 ? "darkgreen" : (delta < 0 ? "FireBrick" : "black");
                    }
                    font: scoreFont.uiFont({
                        weight: scoreFont.fontWeight,
                        variableAxes: scoreFont.variableAxes,
                        italic: scoreFont.italic,
                        pixelSize: 25
                    })
                    horizontalAlignment: Text.AlignLeft
                    text: {
                        let delta = score.points - score.oldBestPoints;
                        if (delta > 0) {
                            return "+" + delta;
                        } else if (delta < 0) {
                            return "–" + (-delta);
                        } else {
                            return "";
                        }
                    }
                }
            }
        }
        Image {
            id: gradeImage

            anchors.verticalCenter: parent.verticalCenter
            source: root.iniImagesUrl + "parts.png/" + Helpers.getGrade(score.points, score.maxPoints)
        }
    }
}

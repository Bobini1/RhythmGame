import RhythmGameQml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtMultimedia
import QtQml
import "../common/helpers.js" as Helpers

FocusScope {
    focus: StackView.status === StackView.Active

    Image {
        id: root

        readonly property bool active: parent.focus
        readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
        readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
        readonly property var oldBestClearScore: {
            let scores = ScoreDb.getScoresForChart(chartData.sha256);
            // remove the score with the same id
            scores = scores.filter(function (score) {
                    return score.id !== result.id;
                });
            return Helpers.getClearType(scores);
        }
        readonly property var oldBestPointsScore: {
            let scores = ScoreDb.getScoresForChart(chartData.sha256);
            // remove the score with the same id
            scores = scores.filter(function (score) {
                    return score.id !== result.id;
                });
            return Helpers.getScoreWithBestPoints(scores);
        }
        property string rootUrl: globalRoot.urlToPath(Qt.resolvedUrl(".").toString())

        fillMode: Image.PreserveAspectCrop
        height: parent.height
        source: root.imagesUrl + (result.clearType === "Failed" ? "failed.png" : "clear.png")
        width: parent.width

        Shortcut {
            enabled: resultFocusScope.active
            sequence: "Esc"

            onActivated: {
                sceneStack.pop();
                sceneStack.pop();
            }
        }
        Item {
            id: scaledRoot

            height: 1080
            scale: Math.min(parent.width / width, parent.height / height)
            width: 1920

            Column {
                width: parent.width

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter

                    Image {
                        id: stageFile

                        asynchronous: true
                        height: sourceSize.height
                        source: {
                            let stageFile = "file://" + chartData.directory + chartData.stageFile;
                            return FileValidator.exists(stageFile) ? stageFile : "";
                        }
                        sourceSize.height: 192
                        sourceSize.width: 256
                        width: sourceSize.width

                        Rectangle {
                            anchors.fill: parent
                            color: "black"
                            opacity: 0.5
                            z: -1
                        }
                    }
                    WindowBg {
                        id: title

                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 24
                        height: 124
                        width: 1214

                        Text {
                            id: titleText

                            anchors.bottom: artistText.top
                            anchors.horizontalCenter: parent.horizontalCenter
                            font.pixelSize: 40
                            text: chartData.title + (chartData.subtitle ? (" " + chartData.subtitle) : "")
                        }
                        Text {
                            id: artistText

                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 18
                            anchors.horizontalCenter: parent.horizontalCenter
                            font.pixelSize: 30
                            text: chartData.artist + (chartData.subartist ? (" " + chartData.subartist) : "")
                        }
                    }
                }
                Column {
                    id: scoreColumn

                    WindowBg {
                        id: score

                        height: 160
                        width: 650

                        Row {
                            anchors.left: parent.left
                            anchors.leftMargin: 24
                            anchors.top: parent.top
                            anchors.topMargin: 24

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
                                    Text {
                                        id: exScoreText

                                        anchors.bottom: parent.bottom
                                        anchors.bottomMargin: -12
                                        anchors.right: parent.right
                                        anchors.rightMargin: 120
                                        color: "lightgray"
                                        font.pixelSize: 41
                                        horizontalAlignment: Text.AlignRight
                                        text: {
                                            let points = result.result.points;
                                            // pad with gray zeros, make the number pink
                                            return "00000".slice(0, 5 - points.toString().length) + "<font color='DeepPink'>" + points + "</font>";
                                        }
                                    }
                                    Text {
                                        id: scoreRate

                                        anchors.bottom: parent.bottom
                                        anchors.bottomMargin: -8
                                        anchors.leftMargin: 30
                                        anchors.right: parent.right
                                        font.pixelSize: 25
                                        horizontalAlignment: Text.AlignRight
                                        text: (result.result.maxPoints ? (result.result.points / result.result.maxPoints * 100).toFixed(2) : "0.00") + "%"
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
                                    Text {
                                        id: hiScoreText

                                        anchors.bottom: parent.bottom
                                        anchors.bottomMargin: -9
                                        anchors.right: parent.right
                                        anchors.rightMargin: 120
                                        color: "lightgray"
                                        font.pixelSize: 34
                                        horizontalAlignment: Text.AlignRight
                                        text: {
                                            let points = root.oldBestPointsScore ? root.oldBestPointsScore.points : 0;
                                            // pad with gray zeros, make the number pink
                                            return "00000".slice(0, 5 - points.toString().length) + "<font color='DeepPink'>" + points + "</font>";
                                        }
                                    }
                                    Text {
                                        id: hiScoreDelta

                                        anchors.bottom: parent.bottom
                                        anchors.bottomMargin: -7
                                        anchors.left: hiScoreText.right
                                        anchors.leftMargin: 41
                                        color: {
                                            let delta = result.result.points - (root.oldBestPointsScore ? root.oldBestPointsScore.points : 0);
                                            return delta > 0 ? "darkgreen" : (delta < 0 ? "FireBrick" : "black");
                                        }
                                        font.pixelSize: 25
                                        horizontalAlignment: Text.AlignLeft
                                        text: {
                                            let delta = result.result.points - (root.oldBestPointsScore ? root.oldBestPointsScore.points : 0);
                                            return (delta > 0 ? "+" : "") + delta;
                                        }
                                    }
                                }
                            }
                            Image {
                                id: gradeImage

                                anchors.verticalCenter: parent.verticalCenter
                                source: root.iniImagesUrl + "parts.png/" + Helpers.getGrade(result.result.points, result.result.maxPoints)
                            }
                        }
                    }
                }
            }
        }
    }
}
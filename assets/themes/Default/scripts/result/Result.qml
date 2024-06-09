import RhythmGameQml
import QtQuick
import QtQuick.Shapes
import RhythmGameQml
import QtQml
import QtQuick.Controls.Basic
import "../common/helpers.js" as Helpers

FocusScope {
    focus: StackView.status === StackView.Active

    Image {
        id: root

        readonly property bool active: parent.focus
        readonly property var earlyLate: Helpers.getEarlyLate(result.replayData, result.result.judgementCounts[Judgement.Poor])
        readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
        readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
        readonly property var oldBestClear: Helpers.getClearType(scores)
        readonly property var oldBestPointsScore: Helpers.getScoreWithBestPoints(scores)
        readonly property var oldBestStats: Helpers.getBestStats(scores)
        readonly property string rootUrl: globalRoot.urlToPath(Qt.resolvedUrl(".").toString())
        readonly property var scores: ScoreDb.getScoresForChart(chartData.sha256).filter(function (score) {
            return score.id !== result.result.id;
        })

        fillMode: Image.PreserveAspectCrop
        height: parent.height
        source: root.imagesUrl + (result.result.clearType === "FAILED" ? "failed.png" : "clear.png")
        width: parent.width

        Shortcut {
            enabled: resultFocusScope.active
            sequence: "Esc"

            onActivated: {
                sceneStack.pop();
                sceneStack.pop();
            }
        }
        Shortcut {
            enabled: resultFocusScope.active
            sequence: "Down"

            onActivated: {
                lifeGraph.decrementIndex();
            }
        }
        Shortcut {
            enabled: resultFocusScope.active
            sequence: "Up"

            onActivated: {
                lifeGraph.incrementIndex();
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

                    StageFile {
                    }
                    TitleArtist {
                        id: titleArtist

                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 24
                        height: 124
                        width: 1214
                    }
                    ChartInfo {
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 24
                        height: titleArtist.height
                        width: 280
                    }
                }
                Item {
                    height: childrenRect.height
                    width: parent.width

                    LifeGraph {
                        id: lifeGraph

                        anchors.right: parent.right
                        anchors.rightMargin: 90
                        anchors.top: scoreColumn.top
                    }
                    LampDiff {
                        anchors.left: scoreColumn.right
                        height: 104
                        width: 350
                    }
                    ScoreColumn {
                        id: scoreColumn

                    }
                }
            }
        }
    }
}
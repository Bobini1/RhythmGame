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
        readonly property BmsScoreAftermath result: resultFocusScope.result[0]
        readonly property var scores: ProfileList.mainProfile.scoreDb.getScoresForChart(chartData.sha256).filter(function (score) {
            return score.id !== result.result.id;
        })
        readonly property ChartData _chartData: chartData

        fillMode: Image.PreserveAspectCrop
        height: parent.height
        source: root.imagesUrl + (result.result.clearType === "FAILED" ? "failed.png" : "clear.png")
        width: parent.width

        Shortcut {
            enabled: root.enabled
            sequence: "Esc"

            onActivated: {
                sceneStack.pop();
                sceneStack.pop();
            }
        }
        Shortcut {
            enabled: root.enabled
            sequence: "Down"

            onActivated: {
                lifeGraph.decrementIndex();
            }
        }
        Shortcut {
            enabled: root.enabled
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
            anchors.centerIn: parent

            Column {
                width: parent.width

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter

                    StageFile {
                        chartDirectory: chartData.chartDirectory
                        stageFileName: chartData.stageFile
                    }
                    TitleArtist {
                        id: titleArtist

                        title: chartData.title
                        artist: chartData.artist
                        subtitle: chartData.subtitle
                        subartist: chartData.subartist

                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 24
                        height: 124
                        width: 1214
                    }
                    ChartInfo {
                        judgementCounts: root.result.result.judgementCounts
                        chartData: root._chartData

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

                        clearType: root.result.result.clearType
                        gaugeHistory: root.result.gaugeHistory.gaugeHistory
                        gaugeInfo: root.result.gaugeHistory.gaugeInfo
                        chartData: root._chartData

                        anchors.right: parent.right
                        anchors.rightMargin: 90
                        anchors.top: scoreColumn.top
                    }
                    LampDiff {
                        anchors.left: scoreColumn.right
                        height: 104
                        width: 350

                        clearType: root.result.result.clearType
                        oldBestClear: root.oldBestClear
                    }
                    ScoreColumn {
                        id: scoreColumn

                        points: root.result.result.points
                        maxPoints: root.result.result.maxPoints
                        oldBestPoints: root.oldBestPointsScore?.points || 0
                        oldBestStats: root.oldBestStats
                        earlyLate: root.earlyLate
                        judgementCounts: root.result.result.judgementCounts
                        maxCombo: root.result.result.maxCombo
                    }
                }
            }
        }
    }
}
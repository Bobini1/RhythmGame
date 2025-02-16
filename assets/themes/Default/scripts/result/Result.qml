import RhythmGameQml
import QtQuick
import QtQuick.Shapes
import RhythmGameQml
import QtQml
import QtQuick.Controls.Basic
import "../common/helpers.js" as Helpers

FocusScope {
    focus: enabled

    Image {
        id: root

        readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
        readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
        readonly property string rootUrl: globalRoot.urlToPath(Qt.resolvedUrl(".").toString())
        readonly property BmsScoreAftermath result1: resultFocusScope.result[0]
        readonly property BmsScoreAftermath result2: resultFocusScope.result[1] || null
        readonly property bool isBattle: result1 && result2
        readonly property ChartData _chartData: chartData

        fillMode: Image.PreserveAspectCrop
        height: parent.height
        source: root.imagesUrl + (result1.result.clearType === "FAILED" ? "failed.png" : "clear.png")
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


            Row {
                id: chartInfoRow
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
                    chartData: root._chartData
                    noteCount: root.result1.result.normalNoteCount + root.result1.result.lnCount

                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 24
                    height: titleArtist.height
                    width: 280
                }
            }

            Side {
                chartData: root._chartData
                result: root.result1
                isBattle: root.isBattle
                width: parent.width
                anchors.top: chartInfoRow.bottom
            }

            Loader {
                active: root.isBattle
                width: parent.width
                anchors.top: chartInfoRow.bottom
                sourceComponent: Side {
                    chartData: root._chartData
                    result: root.result2
                    isBattle: root.isBattle
                    mirrored: true
                }
            }

            component Side: Column {
                id: side
                required property ChartData chartData
                required property BmsScoreAftermath result
                required property bool isBattle
                property bool mirrored: false
                readonly property var earlyLate: Helpers.getEarlyLate(result.replayData, result.result.judgementCounts[Judgement.Poor])
                readonly property var oldBestClear: Helpers.getClearType(scores)
                readonly property var oldBestPointsScore: Helpers.getScoreWithBestPoints(scores)
                readonly property var oldBestStats: Helpers.getBestStats(scores)
                readonly property var scores: result.profile.scoreDb.getScoresForChart(chartData.sha256).filter(function (score) {
                    return score.id !== result.result.id;
                })
                transform: Scale {
                    xScale: side.mirrored ? -1 : 1
                    origin.x: side.mirrored ? side.width / 2 : 0
                }
                Item {
                    height: childrenRect.height
                    width: parent.width

                    LampDiff {
                        id: lampDiff
                        anchors.left: scoreColumn.right
                        anchors.top: parent.top
                        anchors.topMargin: side.mirrored ? 330 : 0
                        height: 104
                        width: 350

                        clearType: root.result1.result.clearType
                        oldBestClear: side.oldBestClear
                        transform: Scale {
                            xScale: side.mirrored ? -1 : 1
                            origin.x: side.mirrored ? lampDiff.width / 2 : 0
                        }
                    }
                    LifeGraph {
                        id: lifeGraph

                        clearType: root.result1.result.clearType
                        gaugeHistory: root.result1.gaugeHistory.gaugeHistory
                        gaugeInfo: root.result1.gaugeHistory.gaugeInfo
                        chartData: root._chartData

                        anchors.right: side.isBattle ? undefined : parent.right
                        anchors.left: side.isBattle ? scoreColumn.right : undefined
                        scale: side.isBattle ? 350 / implicitWidth : 1
                        transformOrigin: Item.TopLeft
                        anchors.rightMargin: 90
                        anchors.top: side.isBattle ? lampDiff.bottom : scoreColumn.top
                        transform: Scale {
                            xScale: side.mirrored ? -1 : 1
                            origin.x: side.mirrored ? lifeGraph.scale * lifeGraph.width / 2 : 0
                        }
                    }
                    ScoreColumn {
                        id: scoreColumn

                        points: side.result.result.points
                        maxPoints: side.result.result.maxPoints
                        oldBestPoints: side.oldBestPointsScore?.points || 0
                        oldBestStats: side.oldBestStats
                        earlyLate: side.earlyLate
                        judgementCounts: side.result.result.judgementCounts
                        maxCombo: side.result.result.maxCombo
                        transform: Scale {
                            xScale: side.mirrored ? -1 : 1
                            origin.x: side.mirrored ? scoreColumn.width / 2 : 0
                        }
                    }
                }
            }
        }
    }
}
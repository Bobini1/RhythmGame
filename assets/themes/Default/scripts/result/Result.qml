import RhythmGameQml
import QtQuick
import QtQuick.Shapes
import RhythmGameQml
import QtQml
import QtQuick.Controls.Basic
import "../common/helpers.js" as Helpers

Item {
    id: root

    // course
    property var course
    property var chartDatas
    // single chart
    property ChartData chartData

    required property var scores
    required property list<Profile> profiles

    readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    readonly property string rootUrl: QmlUtils.fileName.slice(0, QmlUtils.fileName.lastIndexOf("/") + 1)
    readonly property var score1: scores[0]
    readonly property var score2: scores[1] || null
    readonly property Profile profile1: profiles[0]
    readonly property Profile profile2: profiles[1] || null
    readonly property bool isBattle: score1 && score2

    Image {
        fillMode: Image.PreserveAspectCrop
        height: parent.height
        source: root.imagesUrl + (root.score1.result.clearType === "FAILED" ? "failed.png" : "clear.png")
        width: parent.width

        Shortcut {
            enabled: root.enabled
            sequence: "Esc"

            onActivated: {
                sceneStack.pop();
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
                    chartDirectory: chartData?.chartDirectory || ""
                    stageFileName: chartData?.stageFile || ""
                }
                TitleArtist {
                    id: titleArtist

                    title: chartData?.title || course?.name || ""
                    artist: root.chartData?.artist || ""
                    subtitle: root.chartData?.subtitle || ""
                    subartist: root.chartData?.subartist || ""

                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 24
                    height: 124
                    width: 1214
                }
                ChartInfo {
                    difficulty: root.chartData?.difficulty
                    total: root.chartData?.total
                    noteCount: root.score1.result.normalNoteCount + root.score1.result.lnCount

                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 24
                    height: titleArtist.height
                    width: 280
                }
            }

            Side {
                score: root.score1
                isBattle: root.isBattle
                profile: root.profile1
                width: parent.width
                anchors.top: chartInfoRow.bottom
            }

            Loader {
                active: root.isBattle
                width: parent.width
                anchors.top: chartInfoRow.bottom
                sourceComponent: Side {
                    score: root.score2
                    isBattle: root.isBattle
                    profile: root.profile2
                    mirrored: true
                }
            }

            component Side: Column {
                id: side
                required property var score
                required property Profile profile
                required property bool isBattle
                property bool mirrored: false
                readonly property var earlyLate: Helpers.getEarlyLate(score.replayData)
                readonly property string oldBestClear: Helpers.getClearType(scores)
                readonly property var oldBestPointsScore: Helpers.getScoreWithBestPoints(scores)
                readonly property var oldBestStats: Helpers.getBestStats(scores)
                readonly property var scores: []
                Component.onCompleted: {
                    if (root.course) {
                        profile.scoreDb.getScoresForCourseId([root.course.identifier]).then((dbScores) => {
                            scores = dbScores[0].filter((oldScore) => oldScore.result.guid !== score.result.guid)
                        });
                    } else {
                        profile.scoreDb.getScoresForMd5([root.chartData.md5]).then((dbScores) => {
                            scores = dbScores[0].filter((oldScore) => oldScore.result.guid !== score.result.guid)
                        });
                    }
                }
                transform: Scale {
                    xScale: side.mirrored ? -1 : 1
                    origin.x: side.mirrored ? side.width / 2 : 0
                }
                Input.onButtonPressed: (key) => {
                    if (key === BmsKey[`Col${mirrored ? 2 : 1}1`]) {
                        lifeGraph.decrementIndex();
                    } else if (key === BmsKey[`Col${mirrored ? 2 : 1}2`]) {
                        lifeGraph.incrementIndex();
                    }
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

                        clearType: side.score.result.clearType
                        oldBestClear: side.oldBestClear
                        transform: Scale {
                            xScale: side.mirrored ? -1 : 1
                            origin.x: side.mirrored ? lampDiff.width / 2 : 0
                        }
                    }
                    LifeGraph {
                        id: lifeGraph

                        clearType: side.score.result.clearType
                        gaugeInfo: side.score.gaugeHistory.gaugeInfo
                        length: side.score.result.length
                        lengths: side.score instanceof BmsScoreCourse ? side.score.scores.map((s) => s.result.length) : [side.score.result.length]

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

                        points: side.score.result.points
                        maxPoints: side.score.result.maxPoints
                        oldBestPoints: side.oldBestPointsScore?.result.points || 0
                        oldBestStats: side.oldBestStats
                        earlyLate: side.earlyLate
                        judgementCounts: side.score.result.judgementCounts
                        maxCombo: side.score.result.maxCombo
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
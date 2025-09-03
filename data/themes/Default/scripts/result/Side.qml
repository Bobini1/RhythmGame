import RhythmGameQml
import QtQuick
import QtQml
import "../common/helpers.js" as Helpers

Column {
    id: side
    required property var score
    required property Profile profile
    required property bool isBattle
    property bool mirrored: false
    readonly property var earlyLate: Helpers.getEarlyLate(score.replayData)
    readonly property string oldBestClear: Helpers.getClearType(scores)
    readonly property var oldBestPointsScore: Helpers.getScoreWithBestPoints(scores)
    readonly property var oldBestStats: Helpers.getBestStats(scores)
    property var scores: []
    Component.onCompleted: {
        if (root.course) {
            profile.scoreDb.getScoresForCourseId([root.course.identifier]).then((dbScores) => {
                if (dbScores.scores[root.course.identifier] === undefined) {
                    return;
                }
                scores =  dbScores.scores[root.course.identifier].filter((oldScore) => oldScore.result.guid !== score.result.guid);
            });
        } else {
            profile.scoreDb.getScoresForMd5([root.chartData.md5]).then((dbScores) => {
                if (dbScores.scores[root.chartData.md5] === undefined) {
                    return;
                }
                scores = dbScores.scores[root.chartData.md5].filter((oldScore) => oldScore.result.guid !== score.result.guid);
            });
        }
    }
    transform: Scale {
        xScale: side.mirrored ? -1 : 1
        origin.x: side.mirrored ? side.width / 2 : 0
    }
    Input.onButtonPressed: (key) => {
        if (key === BmsKey[`Select${mirrored ? 2 : 1}`]) {
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

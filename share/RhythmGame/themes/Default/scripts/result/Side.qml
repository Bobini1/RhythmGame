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
    readonly property var stddevAndMean: Helpers.getStddevAndMean(score.replayData)
    readonly property var stddev: stddevAndMean.stddev
    readonly property var mean: stddevAndMean.mean
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

        MeanSd {
            id: meanSd
            anchors.left: scoreColumn.right
            anchors.top: parent.top
            anchors.topMargin: side.mirrored ? 330 : 0
            height: 104
            width: 350

            mean: side.mean
            stddev: side.stddev
            transform: Scale {
                xScale: side.mirrored ? -1 : 1
                origin.x: side.mirrored ? meanSd.width / 2 : 0
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
            anchors.top: side.isBattle ? meanSd.bottom : scoreColumn.top
            transform: Scale {
                xScale: side.mirrored ? -1 : 1
                origin.x: side.mirrored ? lifeGraph.scale * lifeGraph.width / 2 : 0
            }
        }
        ScoreColumn {
            id: scoreColumn

            OnlineRankingModel {
                id: rankingModelNew
                md5: side.score.result.md5
                sortBy: OnlineRankingModel.ScorePct
                sortDir: OnlineRankingModel.Desc
                webApiUrl: side.profile.vars.generalVars.webApiUrl
                dateLte: side.score.result.unixTimestamp

                property int position: {
                    let entries = rankingEntries;
                    for (let i = 0; i < entries.length; i++) {
                        if (entries[i].owner === side.score.result.owner ||
                            (side.score.result.owner === "" && entries[i].userId === side.profile.onlineUserId)) {
                            return i + 1;
                        }
                    }
                    return 0;
                }
            }

            Connections {
                target: side.score

                function onSubmissionStateChanged() {
                    rankingModelNew.refresh();
                }
            }

            OnlineRankingModel {
                id: rankingModelOld
                md5: side.score.result.md5
                sortBy: OnlineRankingModel.ScorePct
                sortDir: OnlineRankingModel.Desc
                webApiUrl: side.profile.vars.generalVars.webApiUrl
                dateLte: side.score.result.unixTimestamp - 1

                property int position: {
                    let entries = rankingEntries;
                    for (let i = 0; i < entries.length; i++) {
                        if (entries[i].owner === side.score.result.owner ||
                            (side.score.result.owner === "" && entries[i].userId === side.profile.onlineUserId)) {
                            return i + 1;
                        }
                    }
                    return 0;
                }
            }

            points: side.score.result.points
            maxPoints: side.score.result.maxPoints
            oldBestPoints: side.oldBestPointsScore?.result.points || 0
            oldBestStats: side.oldBestStats
            earlyLate: side.earlyLate
            judgementCounts: side.score.result.judgementCounts
            maxCombo: side.score.result.maxCombo
            clearType: side.score.result.clearType
            oldBestClear: side.oldBestClear
            oldRankingPosition: rankingModelOld.position
            newRankingPosition: rankingModelNew.position
            totalEntries: rankingModelNew.rankingEntries.length
            loading: rankingModelNew.loading || rankingModelOld.loading || side.score.submissionState === BmsScore.Submitting
            scoreSubmissionFailed: side.score.submissionState === BmsScore.Failed || side.score.submissionState === BmsScore.NotSubmitting
            rankingUrl: totalEntries ? Rg.onlineLinks.chart(side.profile.vars.generalVars.websiteBaseUrl, side.score.result.md5) : ""
            transform: Scale {
                xScale: side.mirrored ? -1 : 1
                origin.x: side.mirrored ? scoreColumn.width / 2 : 0
            }
        }
    }
}

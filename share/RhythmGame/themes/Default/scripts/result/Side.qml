import RhythmGameQml
import QtQuick
import QtQml
import "../common/helpers.js" as Helpers

Column {
    id: side
    required property var score
    required property Profile profile
    required property bool isBattle
    required property var chartKeymode
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
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true

                onWheel: (event) => {
                    if (event.angleDelta.y > 0) {
                        lifeGraph.incrementIndex();
                    } else if (event.angleDelta.y < 0) {
                        lifeGraph.decrementIndex();
                    }
                }
            }
        }
        ScoreColumn {
            id: scoreColumn

            component Ranking: RankingQuery {
                md5: side.score.result.md5
                webApiUrl: side.profile.vars.generalVars.webApiUrl
                userId: provider === OnlineRankingModel.Tachi ? side.profile.tachiData?.userId : side.profile.onlineUserData?.userId
            }

            Ranking {
                id: tachi
                provider: OnlineRankingModel.Tachi
            }

            Ranking {
                id: lr2ir
                provider: OnlineRankingModel.LR2IR
            }

            Ranking {
                id: rhythmGame
                provider: OnlineRankingModel.RhythmGame
            }

            property var ranking: {
                switch (rankingProvider) {
                    case OnlineRankingModel.RhythmGame:
                        return rhythmGame;
                    case OnlineRankingModel.LR2IR:
                        return lr2ir;
                    case OnlineRankingModel.Tachi:
                        return tachi;
                }
            }

            property var rankingProvider: {
                let choice = profile.vars.themeVars.select[QmlUtils.themeName].rankingProvider
                switch (choice) {
                    case "rhythmgame":
                        return OnlineRankingModel.RhythmGame;
                    case "lr2ir":
                        return OnlineRankingModel.LR2IR;
                    case "bokutachi":
                        return OnlineRankingModel.Tachi;
                    default:
                        return OnlineRankingModel.RhythmGame;
                }
            }
            Binding {
                delayed: true
                scoreColumn.rankingProvider: {
                    switch (side.profile.vars.themeVars.select[QmlUtils.themeName].rankingProvider) {
                        case "rhythmgame":
                            return OnlineRankingModel.RhythmGame;
                        case "lr2ir":
                            return OnlineRankingModel.LR2IR;
                        case "bokutachi":
                            return OnlineRankingModel.Tachi;
                        default:
                            return OnlineRankingModel.RhythmGame;
                    }
                }
            }
            Binding {
                delayed: true
                target: side.profile.vars.themeVars.select[QmlUtils.themeName]
                property: "rankingProvider"
                value: {
                    switch (scoreColumn.rankingProvider) {
                        case OnlineRankingModel.RhythmGame:
                            return "rhythmgame";
                        case OnlineRankingModel.LR2IR:
                            return "lr2ir";
                        case OnlineRankingModel.Tachi:
                            return "bokutachi";
                    }
                }
            }

            onLeftClicked: {
                if (scoreColumn.rankingProvider === OnlineRankingModel.RhythmGame) {
                    scoreColumn.rankingProvider = OnlineRankingModel.Tachi;
                } else if (scoreColumn.rankingProvider === OnlineRankingModel.Tachi) {
                    scoreColumn.rankingProvider = OnlineRankingModel.LR2IR;
                } else if (scoreColumn.rankingProvider === OnlineRankingModel.LR2IR) {
                    scoreColumn.rankingProvider = OnlineRankingModel.RhythmGame;
                }
            }

            onRightClicked: {
                if (scoreColumn.rankingProvider === OnlineRankingModel.RhythmGame) {
                    scoreColumn.rankingProvider = OnlineRankingModel.LR2IR;
                } else if (scoreColumn.rankingProvider === OnlineRankingModel.LR2IR) {
                    scoreColumn.rankingProvider = OnlineRankingModel.Tachi;
                } else if (scoreColumn.rankingProvider === OnlineRankingModel.Tachi) {
                    scoreColumn.rankingProvider = OnlineRankingModel.RhythmGame;
                }
            }

            Connections {
                target: side.score
                ignoreUnknownSignals: true

                function onSubmissionStateChanged() {
                    rhythmGame.refresh();
                    tachi.refresh();
                }
            }

            readonly property string keymode: {
                switch (side.score.keymode || side.chartKeymode) {
                    case 5:
                    case 7:
                        return "7K";
                    case 10:
                    case 14:
                        return "14K";
                }
                return "";
            }

            points: side.score.result.points
            maxPoints: side.score.result.maxPoints
            oldBestPoints: side.oldBestPointsScore?.result?.points || 0
            oldBestStats: side.oldBestStats
            earlyLate: side.earlyLate
            judgementCounts: side.score.result.judgementCounts
            maxCombo: side.score.result.maxCombo
            clearType: side.score.result.clearType
            oldBestClear: side.oldBestClear
            // Course rankings are not implemented atm
            oldRankingPosition: root.course ? 0 : ranking.oldPosition
            newRankingPosition: root.course ? 0 : ranking.position
            websiteUrl: side.profile.vars.generalVars.websiteBaseUrl
            provider: ranking.provider
            totalEntries: root.course ? 0 : ranking.size
            loading: ranking.loading || ranking.positionLoading || side.score.submissionState === BmsScore.Submitting
            scoreSubmissionFailed: side.score.submissionState === BmsScore.Failed ||
                side.score.submissionState === BmsScore.NotSubmitting || root.course ||
                (ranking.provider === OnlineRankingModel.RhythmGame && !side.profile.onlineUserData) ||
                (ranking.provider === OnlineRankingModel.Tachi && !side.profile.tachiData)
            rankingUrl: {
                if (root.course || !totalEntries || (ranking.provider === OnlineRankingModel.LR2IR && totalEntries <= 1)) {
                    return "";
                }
                if (ranking.provider === OnlineRankingModel.LR2IR) {
                    return "http://www.dream-pro.info/~lavalse/LR2IR/search.cgi?mode=ranking&bmsmd5=" + side.score.result.md5;
                }
                if (ranking.provider === OnlineRankingModel.Tachi) {
                    return "https://boku.tachi.ac/games/bms/" + scoreColumn.keymode +
                        "/charts/" + ranking.chartId;
                }
                return side.profile.vars.generalVars.websiteBaseUrl + "/charts/" + side.score.result.md5
            }
            transform: Scale {
                xScale: side.mirrored ? -1 : 1
                origin.x: side.mirrored ? scoreColumn.width / 2 : 0
            }
        }
    }
}

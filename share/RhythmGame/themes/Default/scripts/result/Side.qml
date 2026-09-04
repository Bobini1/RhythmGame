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
    property string arenaRoundId: ""
    property bool arenaResultActive: false
    property bool mirrored: false
    readonly property var presentedResult: Rg.arenaSession.presentedResult
    readonly property bool arenaResultMatches: side.arenaResultActive
        && side.arenaRoundId.length > 0
        && Rg.arenaSession.resultPresentationActive === true
        && side.presentedResult && side.presentedResult.valid === true
        && side.arenaRoundId
            === String(side.presentedResult.roundId || "")
    readonly property var earlyLate: Helpers.getEarlyLate(score.replayData)
    readonly property var stddevAndMean: Helpers.getStddevAndMean(score.replayData)
    readonly property var stddev: stddevAndMean.stddev
    readonly property var mean: stddevAndMean.mean
    readonly property string oldBestClear: Helpers.getClearType(scores)
    readonly property var oldBestPointsScore: Helpers.getScoreWithBestPoints(scores)
    readonly property var oldBestStats: Helpers.getBestStats(scores)
    property var scores: []
    property var pendingScoreDbReply: null

    function trackScoreDbReply(reply: var) : var {
        if (!reply || reply.resultAvailable) {
            return reply;
        }
        pendingScoreDbReply = reply;
        let forget = function() {
            reply.finished.disconnect(forget);
            if (pendingScoreDbReply === reply) {
                pendingScoreDbReply = null;
            }
        };
        reply.finished.connect(forget);
        return reply;
    }

    function cancelScoreDbReply() {
        let reply = pendingScoreDbReply;
        pendingScoreDbReply = null;
        if (reply && !reply.resultAvailable) {
            reply.cancel();
        }
    }

    Component.onCompleted: {
        if (root.course) {
            trackScoreDbReply(profile.scoreDb.getScoresForCourseId([root.course.identifier])).then((dbScores) => {
                if (dbScores.scores[root.course.identifier] === undefined) {
                    return;
                }
                scores =  dbScores.scores[root.course.identifier].filter((oldScore) => oldScore.result.guid !== score.result.guid);
            });
        } else {
            trackScoreDbReply(profile.scoreDb.getScoresForMd5([root.chartData.md5])).then((dbScores) => {
                if (dbScores.scores[root.chartData.md5] === undefined) {
                    return;
                }
                scores = dbScores.scores[root.chartData.md5].filter((oldScore) => oldScore.result.guid !== score.result.guid);
            });
        }
    }
    Component.onDestruction: cancelScoreDbReply();

    function cycleGauge() {
        if (side.score.gaugeHistory === null) {
            return false;
        }
        lifeGraph.incrementIndex();
        return true;
    }

    transform: Scale {
        xScale: side.mirrored ? -1 : 1
        origin.x: side.mirrored ? side.width / 2 : 0
    }
    Input.onButtonPressed: (key) => {
        if (key === BmsKey[`Select${mirrored ? 2 : 1}`]) {
            side.cycleGauge();
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
            visible: side.score.replayData !== null
            transform: Scale {
                xScale: side.mirrored ? -1 : 1
                origin.x: side.mirrored ? meanSd.width / 2 : 0
            }
        }
        LifeGraph {
            id: lifeGraph

            clearType: side.score.result.clearType
            gaugeInfo: side.score.gaugeHistory?.gaugeInfo || []
            length: side.score.result.length
            lengths: side.score instanceof BmsScoreCourse ? side.score.scores.map((s) => s.result.length) : [side.score.result.length]

            anchors.right: side.isBattle ? undefined : parent.right
            anchors.left: side.isBattle ? scoreColumn.right : undefined
            scale: side.isBattle ? 350 / implicitWidth : 1
            transformOrigin: Item.TopLeft
            visible: !side.arenaResultMatches && side.score.gaugeHistory !== null
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
                switch (effectiveRankingProvider) {
                    case OnlineRankingModel.RhythmGame:
                        return rhythmGame;
                    case OnlineRankingModel.LR2IR:
                        return lr2ir;
                    case OnlineRankingModel.Tachi:
                        return tachi;
                }
                return rhythmGame;
            }

            readonly property var generalVars: side.profile.vars.generalVars
            readonly property bool arenaMode: side.arenaResultMatches
            readonly property bool arenaSourceSelected: arenaMode
                && effectiveResultSource === "arena"
            readonly property int effectiveRankingProvider:
                providerForSource(effectiveResultSource)
            property string effectiveResultSource: "rhythmGame"

            // Screen-local order: Arena -> RhythmGame -> Tachi -> LR2IR.
            function providerForSource(source) {
                switch (source) {
                case "tachi":
                    return OnlineRankingModel.Tachi;
                case "lr2ir":
                    return OnlineRankingModel.LR2IR;
                default:
                    return OnlineRankingModel.RhythmGame;
                }
            }

            function sourceForProvider(providerValue) {
                switch (providerValue) {
                case OnlineRankingModel.Tachi:
                    return "tachi";
                case OnlineRankingModel.LR2IR:
                    return "lr2ir";
                default:
                    return "rhythmGame";
                }
            }

            function syncRankingProviderFromGeneralVars() {
                if (scoreColumn.arenaMode) {
                    return;
                }
                scoreColumn.effectiveResultSource = sourceForProvider(
                    scoreColumn.generalVars
                        ? scoreColumn.generalVars.rankingProvider
                        : OnlineRankingModel.RhythmGame);
            }

            function setEffectiveResultSource(source) {
                const accepted = ["arena", "rhythmGame", "tachi", "lr2ir"];
                if (accepted.indexOf(source) < 0
                        || (!side.arenaResultMatches && source === "arena")) {
                    return;
                }
                scoreColumn.effectiveResultSource = source;
                if (!side.arenaResultMatches) {
                    if (scoreColumn.generalVars) {
                        scoreColumn.generalVars.rankingProvider =
                            providerForSource(source);
                    }
                }
            }

            function resetEffectiveResultSource() {
                if (scoreColumn.arenaMode) {
                    scoreColumn.effectiveResultSource = "arena";
                } else {
                    scoreColumn.syncRankingProviderFromGeneralVars();
                }
            }

            function cycleEffectiveResultSource(step) {
                const sources = scoreColumn.arenaMode
                    ? ["arena", "rhythmGame", "tachi", "lr2ir"]
                    : ["rhythmGame", "tachi", "lr2ir"];
                let index = sources.indexOf(
                    scoreColumn.effectiveResultSource);
                if (index < 0) {
                    index = 0;
                }
                const next = (index + step + sources.length)
                    % sources.length;
                scoreColumn.setEffectiveResultSource(sources[next]);
            }

            Component.onCompleted: resetEffectiveResultSource()

            onArenaModeChanged: resetEffectiveResultSource()

            Connections {
                target: scoreColumn.generalVars

                function onRankingProviderChanged() {
                    scoreColumn.syncRankingProviderFromGeneralVars();
                }
            }

            onLeftClicked: cycleEffectiveResultSource(1)

            onRightClicked: cycleEffectiveResultSource(-1)

            Connections {
                target: side.score
                ignoreUnknownSignals: true

                function onSubmissionStateChanged() {
                    rhythmGame.refresh();
                    tachi.refresh();
                }
            }

            readonly property string tachiGameId: {
                switch (side.score.keymode || side.chartKeymode) {
                    case 5:
                    case 7:
                        return "bms-7k";
                    case 10:
                    case 14:
                        return "bms-14k";
                }
                return "";
            }

            points: side.score.result.points
            importedSource: side.score.imported === true ? side.score.sourceName : ""
            maxPoints: side.score.result.maxPoints
            oldBestPoints: side.oldBestPointsScore?.result?.points || 0
            oldBestStats: side.oldBestStats
            earlyLate: side.earlyLate
            judgementCounts: side.score.result.judgementCounts
            maxCombo: side.score.result.maxCombo
            clearType: side.score.result.clearType
            oldBestClear: side.oldBestClear
            // Course rankings are not implemented atm
            oldRankingPosition: scoreColumn.arenaSourceSelected || root.course
                ? 0 : ranking.oldPosition
            newRankingPosition: scoreColumn.arenaSourceSelected
                ? (side.presentedResult.finalized
                    ? side.presentedResult.localRank : 0)
                : (root.course ? 0 : ranking.position)
            websiteUrl: side.profile.vars.generalVars.websiteBaseUrl
            provider: ranking.provider
            rankingSource: scoreColumn.effectiveResultSource
            arenaSelected: scoreColumn.arenaSourceSelected
            arenaFinalized: scoreColumn.arenaSourceSelected
                && side.presentedResult.finalized
            arenaLocalDnf: scoreColumn.arenaSourceSelected
                && side.presentedResult.localDnf
            arenaLocalRank: scoreColumn.arenaSourceSelected
                ? side.presentedResult.localRank : 0
            arenaParticipantCount: scoreColumn.arenaSourceSelected
                ? side.presentedResult.participantCount : 0
            totalEntries: scoreColumn.arenaSourceSelected
                ? side.presentedResult.participantCount
                : (root.course ? 0 : ranking.size)
            loading: scoreColumn.arenaSourceSelected
                ? !side.presentedResult.finalized
                : (ranking.loading || ranking.positionLoading
                   || side.score.submissionState === BmsScore.Submitting)
            scoreSubmissionFailed: !scoreColumn.arenaSourceSelected && (
                side.score.submissionState === BmsScore.Failed ||
                side.score.submissionState === BmsScore.NotSubmitting || root.course ||
                (ranking.provider === OnlineRankingModel.RhythmGame && !side.profile.onlineUserData) ||
                (ranking.provider === OnlineRankingModel.Tachi && !side.profile.tachiData))
            rankingUrl: {
                if (scoreColumn.arenaSourceSelected) {
                    return "";
                }
                if (root.course || !totalEntries || (ranking.provider === OnlineRankingModel.LR2IR && totalEntries <= 1)) {
                    return "";
                }
                if (ranking.provider === OnlineRankingModel.LR2IR) {
                    return "http://www.dream-pro.info/~lavalse/LR2IR/search.cgi?mode=ranking&bmsmd5=" + side.score.result.md5;
                }
                if (ranking.provider === OnlineRankingModel.Tachi) {
                    if (!ranking.chartId || !scoreColumn.tachiGameId) {
                        return "";
                    }
                    return "https://boku.tachi.ac/games/" + scoreColumn.tachiGameId +
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

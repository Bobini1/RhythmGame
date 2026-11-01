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
    property bool arenaResultActive: false
    property bool mirrored: false
    readonly property var presentedResult: Rg.arenaSession.presentedResult
    readonly property var earlyLate: Helpers.getEarlyLate(score.replayData)
    readonly property var stddevAndMean: Helpers.getStddevAndMean(score.replayData)
    readonly property var stddev: stddevAndMean.stddev
    readonly property var mean: stddevAndMean.mean
    readonly property string oldBestClear: Helpers.getClearType(results)
    readonly property var oldBestPointsScore: Helpers.getScoreWithBestPoints(results)
    readonly property var oldBestStats: Helpers.getBestStats(results)
    readonly property var results: {
        const scoreDb = side.profile.scoreDb;
        const key = root.course ? root.course.identifier : root.chartData.md5;
        const records = root.course ? scoreDb.courseResults : scoreDb.chartResults;
        return (records[key] || []).filter(result => result.guid !== side.score.result.guid);
    }

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
            visible: !side.arenaResultActive && side.score.gaugeHistory !== null
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

            StandardRanking {
                id: ranking
                enabled: side.enabled && !root.course && !scoreColumn.arenaSourceSelected
                chartData: root.course ? null : root.chartData
                profile: side.profile
                score: root.course ? null : side.score
                provider: scoreColumn.effectiveRankingProvider
            }
            readonly property var generalVars: side.profile.vars.generalVars
            readonly property bool arenaMode: side.arenaResultActive
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
                        || (!side.arenaResultActive && source === "arena")) {
                    return;
                }
                scoreColumn.effectiveResultSource = source;
                if (!side.arenaResultActive) {
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

            points: side.score.result.points
            importedSource: side.score.imported === true ? side.score.sourceName : ""
            maxPoints: side.score.result.maxPoints
            oldBestPoints: side.oldBestPointsScore?.points || 0
            oldBestStats: side.oldBestStats
            earlyLate: side.earlyLate
            judgementCounts: side.score.result.judgementCounts
            maxCombo: side.score.result.maxCombo
            clearType: side.score.result.clearType
            oldBestClear: side.oldBestClear
            // Course rankings are not implemented atm
            oldRankingPosition: scoreColumn.arenaSourceSelected || root.course
                ? 0 : ranking.previousPosition
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
                : (root.course ? 0 : ranking.playerCount)
            loading: scoreColumn.arenaSourceSelected
                ? !side.presentedResult.finalized
                : (ranking.loading
                   || side.score.submissionState === BmsScore.Submitting)
            scoreSubmissionFailed: !scoreColumn.arenaSourceSelected && (
                side.score.submissionState === BmsScore.Failed ||
                side.score.submissionState === BmsScore.NotSubmitting || root.course ||
                (ranking.provider === OnlineRankingModel.RhythmGame && !side.profile.onlineUserData) ||
                (ranking.provider === OnlineRankingModel.Tachi && !side.profile.tachiData))
            rankingAvailable: !scoreColumn.arenaSourceSelected && !root.course
                && ranking.available && !ranking.opening
            onRankingClicked: ranking.open()
            transform: Scale {
                xScale: side.mirrored ? -1 : 1
                origin.x: side.mirrored ? scoreColumn.width / 2 : 0
            }
        }
    }
}

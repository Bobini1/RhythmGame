import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardGameplayTarget
    \inqmlmodule RhythmGameQml
    \brief Supplies the score target and personal best for a gameplay display.

    Bind \l gameplay to the screen context and choose \l playerIndex for battle.
    The component follows the player's score-target settings. It loads saved
    scores for each course stage, chooses the best or latest run, and advances
    recorded targets as the player consumes notes. Scores without a recording
    use their final score rate multiplied by the points available so far.

    Battle uses the other player's current score. Arena uses the session's
    opponent target, and \l available is false until that target is available.
    Local play uses \l mode and \l fraction. Overriding either changes this
    target display without changing the player's saved settings.

    Bind a graph or label to \l points and \l finalPoints. The \l bestPoints,
    \l bestFinalPoints and \l bestMaxPoints properties describe the personal
    best independently of the chosen target. The component draws nothing.
    Pending queries are cancelled when the stage changes or it is disabled.
*/
Item {
    id: root

    /*! Supplies the context for a single chart or course. */
    property GameplayContext gameplay: null
    /*! Player whose profile and progress are used. Zero selects the first player. */
    property int playerIndex: 0
    /*! Target choice. Defaults to the player's saved ScoreTarget setting. */
    property int mode: state.player?.profile.vars.generalVars.scoreTarget ?? ScoreTarget.Fraction
    /*! Target fraction used by ScoreTarget.Fraction and when no saved target exists. */
    property real fraction: state.player?.profile.vars.generalVars.targetScoreFraction ?? 0
    /*! Whether the selected player's saved scores are being loaded. */
    readonly property bool loading: state.loading
    /*! Whether the current target can be displayed. */
    readonly property bool available: enabled && !!state.player
        && (!gameplay.isArena || (gameplay.arenaActive && !!state.arenaTarget?.memberId))
    /*! Current target points, at the player's progress through the chart. */
    readonly property real points: {
        if (!available) return 0;
        if (gameplay.isArena) return state.arenaTarget.exScore;
        if (state.opponent) return state.opponent.score.points;
        const saved = state.targetResult;
        if (saved) return (mode === ScoreTarget.LastScore ? state.lastScore : state.bestScore)?.replayData
            ? (mode === ScoreTarget.LastScore ? lastGhost.points : bestGhost.points)
            : state.pointsAtProgress(saved);
        return state.player.score.maxPointsNow * state.targetFraction;
    }
    /*! Final target points. Live battle opponents have no fixed final target. */
    readonly property real finalPoints: {
        if (!available) return 0;
        if (gameplay.isArena) return state.arenaTarget.exScore;
        if (state.opponent) return 0;
        return state.targetResult ? state.targetResult.points
            : state.player.score.maxPoints * state.targetFraction;
    }
    /*! Personal-best points at the current progress, using a recording when present. */
    readonly property real bestPoints: !enabled || !state.bestResult ? 0
        : state.bestScore?.replayData ? bestGhost.points : state.pointsAtProgress(state.bestResult)
    /*! Final points from the personal best, or zero before scores are loaded. */
    readonly property real bestFinalPoints: state.bestResult?.points || 0
    /*! Maximum points for the personal-best score. */
    readonly property real bestMaxPoints: state.bestResult?.maxPoints || 0

    PendingReplyGroup { id: replies }

    QtObject {
        id: state
        readonly property var player: root.gameplay?.players[root.playerIndex] || null
        readonly property var opponent: root.gameplay?.players[root.playerIndex === 0 ? 1 : 0] || null
        readonly property var arenaTarget: Rg.arenaSession.opponentTarget
        property var results: []
        property var bestScore: null
        property var lastScore: null
        property int remaining: 0
        property var pendingHits: []
        property int generation: 0
        property bool loading: false
        readonly property var bestResult: chooseResult(false)
        readonly property var lastResult: chooseResult(true)
        readonly property var targetResult: root.mode === ScoreTarget.BestScore ? bestResult
            : root.mode === ScoreTarget.LastScore ? lastResult : null
        readonly property real targetFraction: root.mode === ScoreTarget.NextRank && player
            ? nextRankPoints(root.bestFinalPoints, player.score.maxPoints)
                / Math.max(1, player.score.maxPoints) : root.fraction

        function chooseResult(newest) {
            let best = null;
            for (const score of results) {
                if (!score || score.maxPoints <= 0) continue;
                const value = newest ? score.unixTimestamp
                    : score.points / score.maxPoints;
                const previous = !best ? -Infinity : newest ? best.unixTimestamp
                    : best.points / best.maxPoints;
                if (value > previous || (value === previous
                        && score.unixTimestamp > best.unixTimestamp)) best = score;
            }
            return best;
        }

        function nextRankPoints(points, maximum) {
            const max = Math.max(0, Math.floor(maximum));
            for (let rank = 15; rank < 27; ++rank) {
                const target = Math.ceil(max * rank / 27);
                if (points < target) return target;
            }
            return max;
        }

        function pointsAtProgress(score) {
            return player ? player.score.maxPointsNow * score.points
                / Math.max(1, score.maxPoints) : 0;
        }

        function load() {
            const request = ++generation;
            replies.cancelAll();
            results = [];
            bestScore = null;
            lastScore = null;
            pendingHits = [];
            bestGhost.resetPoints();
            lastGhost.resetPoints();
            loading = root.enabled && !!player;
            if (!loading) return;
            const requestedPlayer = player;
            const stage = root.gameplay.stageIndex;
            const md5 = root.gameplay.chartData.md5;
            results = player.profile.scoreDb.chartResults[md5.toUpperCase()] || [];
            const chosen = [bestResult, lastResult];
            remaining = chosen.length;
            chosen.forEach((result, index) => {
                const done = function(score) {
                    if (generation !== request || !root.enabled || player !== requestedPlayer
                            || root.gameplay.stageIndex !== stage) return;
                    if (index === 0) bestScore = score;
                    else lastScore = score;
                    if (--remaining === 0) Qt.callLater(state.finishLoading, request);
                };
                if (!result?.replayAvailable) done(null);
                else replies.track(player.profile.scoreDb.getScore(result)).then(done, () => done(null));
            });
        }

        function finishLoading(request) {
            if (generation !== request || !root.enabled) return;
            for (const hit of pendingHits) {
                bestGhost.notifyHit(hit);
                lastGhost.notifyHit(hit);
            }
            pendingHits = [];
            loading = false;
        }

        onPlayerChanged: Qt.callLater(load)
    }

    ScoreReplayer {
        id: bestGhost
        hitEvents: state.bestScore?.replayData?.hitEvents || []
    }
    ScoreReplayer {
        id: lastGhost
        hitEvents: state.lastScore?.replayData?.hitEvents || []
    }
    Connections {
        target: root.enabled ? state.player?.score || null : null
        function onHit(hit) {
            if (state.loading) {
                if (hit.noteRemoved) state.pendingHits.push(hit);
            }
            else {
                bestGhost.notifyHit(hit);
                lastGhost.notifyHit(hit);
            }
        }
    }
    Connections {
        target: root.gameplay
        function onStageChanged() { Qt.callLater(state.load); }
    }
    onGameplayChanged: Qt.callLater(state.load)
    onEnabledChanged: Qt.callLater(state.load)
    Component.onCompleted: Qt.callLater(state.load)
}

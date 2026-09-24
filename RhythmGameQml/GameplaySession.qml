import QtQml
import RhythmGameQml

// Internal lifecycle shared by the standard gameplay presentation and its tests.
// Audio, input and visual timing live in StandardGameplayFlow.
QtObject {
    id: root

    required property var gameplay
    required property var navigation
    property var arenaSession: null
    property bool active: false
    property bool startReady: false
    property bool finishReady: true

    readonly property var chart: root.gameplay ? root.gameplay._runner : null
    readonly property bool ready: root.gameplay?.status === ChartRunner.Ready
    readonly property bool finished: root.gameplay?.status === ChartRunner.Finished
    readonly property bool arenaManaged: root.gameplay?.isArena ?? false
    readonly property bool isCourse: root.gameplay?.isCourse ?? false
    readonly property int stageIndex: root.gameplay?.stageIndex ?? 0
    readonly property var chartData: root.gameplay ? root.gameplay.chartData : null
    readonly property bool playing: state.phase === "playing"

    signal stageActivated()
    signal finishRequested()
    signal closing()
    signal presentationFailed()

    property QtObject state: QtObject {
        id: state

        property string phase: "playing"
        property int announcedStage: -1
        property bool started: false
        property bool busy: false
        property bool returned: false
        property bool waitForFinish: true
        property bool blocked: false
        property var stageResult: null
        property var courseResult: null
        property QtObject resultItem: null
        property var resultSession: null
        property string arenaRoundId: ""
    }

    function endArenaPresentation() {
        const session = state.resultSession;
        const roundId = state.arenaRoundId;
        state.arenaRoundId = "";
        if (session && roundId.length > 0) {
            session.endResultPresentation(roundId);
        }
    }

    function reset() {
        root.endArenaPresentation();
        state.resultItem = null;
        state.resultSession = null;
        state.phase = "playing";
        state.announcedStage = -1;
        state.started = false;
        state.returned = false;
        state.waitForFinish = true;
        state.blocked = false;
        state.stageResult = null;
        state.courseResult = null;
        Qt.callLater(root.synchronize);
    }

    function profiles() {
        return Array.from(root.gameplay.players, player => player.profile);
    }

    function leave() {
        if (!root.active || state.busy) {
            return false;
        }
        state.busy = true;
        root.closing();
        const removed = root.navigation.returnToPreviousScreen();
        state.busy = false;
        if (removed) {
            state.phase = "leaving";
        }
        return !!removed;
    }

    // An explicit exit skips the natural-finish animation gate. It can also
    // retry a failed presentation without finishing the runner a second time.
    function complete() {
        if (!root.active || !root.chart || state.busy
                || (state.phase !== "playing" && state.phase !== "finishing")) {
            return false;
        }
        state.phase = "finishing";
        state.waitForFinish = false;
        state.blocked = false;
        Qt.callLater(root.synchronize);
        return true;
    }

    function retryTransition() {
        if (!root.active || !state.blocked) {
            return false;
        }
        state.blocked = false;
        Qt.callLater(root.synchronize);
        return true;
    }

    function presentStageResult() {
        state.busy = true;
        root.closing();
        if (!state.stageResult) {
            // proceed() changes the index and players synchronously. Capture
            // the completed stage before it mutates any presentation bindings.
            const data = root.chartData;
            const players = root.profiles();
            const finalStage = root.stageIndex >= root.gameplay.stageCount - 1;
            const scores = root.isCourse ? root.chart.proceed() : root.chart.finish();
            state.stageResult = { scores: scores, profiles: players, chartData: data,
                                  finalStage: finalStage };
            state.resultSession = root.arenaManaged ? root.arenaSession : null;
            if (state.resultSession && state.resultSession.submitLocalResult(scores[0])) {
                state.arenaRoundId = String(state.resultSession.presentedResult.roundId || "");
            }
        }
        const result = state.stageResult;
        state.phase = "stageResult";
        state.returned = false;
        // Retain the scores and submitted round if creating the screen fails.
        // Trying again only opens the result; it never saves or submits twice.
        const item = root.navigation.openResult(result.scores, result.profiles,
                                                result.chartData, root.gameplay, state.arenaRoundId);
        state.resultItem = item || null;
        if (!item) {
            state.phase = "finishing";
            state.blocked = true;
        }
        state.busy = false;
        if (!item) {
            root.presentationFailed();
        }
    }

    function presentCourseResult() {
        state.busy = true;
        if (!state.courseResult) {
            const players = root.profiles();
            const data = root.gameplay.charts;
            const course = root.gameplay.course;
            state.courseResult = { scores: root.chart.finish(), profiles: players,
                                   chartDatas: data, course: course };
        }
        const result = state.courseResult;
        state.phase = "courseResult";
        state.returned = false;
        const item = root.navigation.openCourseResult(result.scores, result.profiles,
                                                     result.chartDatas, result.course);
        if (!item) {
            state.phase = "stageResult";
            state.returned = true;
            state.blocked = true;
        }
        state.busy = false;
        if (!item) {
            root.presentationFailed();
        }
    }

    function synchronize() {
        if (!root.active || !root.chart || state.busy || state.blocked) {
            return;
        }
        if (state.phase === "stageResult" && state.returned) {
            if (root.isCourse && !state.stageResult.finalStage) {
                state.phase = "playing";
                state.started = false;
                state.waitForFinish = true;
                state.stageResult = null;
            } else if (root.isCourse) {
                root.presentCourseResult();
                return;
            } else {
                root.leave();
                return;
            }
        } else if (state.phase === "courseResult" && state.returned) {
            root.leave();
            return;
        }
        if (state.phase === "playing") {
            if (root.finished) {
                state.phase = "finishing";
                root.finishRequested();
            } else {
                if (state.announcedStage !== root.stageIndex) {
                    state.announcedStage = root.stageIndex;
                    root.stageActivated();
                }
                if (root.ready && root.startReady && !root.arenaManaged && !state.started) {
                    state.started = true;
                    root.chart.start();
                }
            }
        }
        if (state.phase === "finishing" && (!state.waitForFinish || root.finishReady)) {
            root.presentStageResult();
        }
    }

    onGameplayChanged: root.reset()
    onActiveChanged: {
        if (!root.active && (state.phase === "stageResult" || state.phase === "courseResult")) {
            state.returned = true;
        }
        Qt.callLater(root.synchronize);
    }
    onReadyChanged: Qt.callLater(root.synchronize)
    onFinishedChanged: Qt.callLater(root.synchronize)
    onStartReadyChanged: Qt.callLater(root.synchronize)
    onFinishReadyChanged: Qt.callLater(root.synchronize)
    property Connections resultLifetime: Connections {
        target: state.resultItem ? state.resultItem.Component : null
        function onDestruction() { root.endArenaPresentation(); }
    }
    Component.onCompleted: Qt.callLater(root.synchronize)
    Component.onDestruction: root.endArenaPresentation()
}

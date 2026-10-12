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

    readonly property bool ready: root.gameplay?.status === ChartRunner.Ready
    readonly property bool finished: root.gameplay?.status === ChartRunner.Finished
    readonly property bool arenaManaged: root.gameplay?.isArena ?? false
    readonly property int stageIndex: root.gameplay?.stageIndex ?? 0
    readonly property var chartData: root.gameplay ? root.gameplay.chartData : null
    readonly property bool playing: state.phase === "playing"
    readonly property bool startRequested: state.started

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
        if (!root.active || !root.gameplay || state.busy
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
            state.stageResult = root.gameplay.finishStage();
            if (!state.stageResult) {
                state.busy = false;
                state.blocked = true;
                root.presentationFailed();
                return;
            }
            state.resultSession = root.arenaManaged ? root.arenaSession : null;
            state.arenaRoundId = state.stageResult.arenaRoundId;
        }
        const result = state.stageResult;
        state.phase = "stageResult";
        state.returned = false;
        // Retain the scores and submitted round if creating the screen fails.
        // Trying again only opens the result; it never saves or submits twice.
        const item = root.navigation.openResult(result);
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
            state.courseResult = root.gameplay.finishCourse();
        }
        const result = state.courseResult;
        state.phase = "courseResult";
        state.returned = false;
        const item = root.navigation.openCourseResult(result);
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
        if (!root.active || !root.gameplay || state.busy || state.blocked) {
            return;
        }
        if (state.phase === "stageResult" && state.returned) {
            if (root.gameplay.course && root.stageIndex + 1 < root.gameplay.stageCount) {
                if (!root.gameplay.nextStage()) {
                    console.warn("Cannot continue the course; returning to selection");
                    root.leave();
                    return;
                }
                state.phase = "playing";
                state.started = false;
                state.waitForFinish = true;
                state.stageResult = null;
            } else if (root.gameplay.course) {
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
                if (root.ready && root.startReady && !state.started) {
                    state.started = root.gameplay.start();
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

import QtQml
import RhythmGameQml
import "../../RhythmGameQml" as Implementation

QtObject {
    id: root

    property bool active: false
    property bool course: false
    property bool failPresentation: false
    property bool arenaManaged: false
    property bool startReady: true
    property bool finishReady: true
    property bool animateFinish: false
    property bool delaying: false
    property int starts: 0
    property int stageSaves: 0
    property int courseSaves: 0
    property int results: 0
    property int courseResults: 0
    property int departures: 0
    property int activations: 0
    property int finishRequests: 0
    property int failures: 0
    property var lastData: null
    property var lastProfiles: null
    property var lastScores: null
    readonly property int finishedStatus: ChartRunner.Finished
    property QtObject gameplay: QtObject {
        readonly property QtObject _runner: root.runner
        readonly property int status: root.runner.status
        readonly property bool isCourse: root.course
        readonly property bool isArena: root.arenaManaged
        readonly property int stageIndex: root.runner.currentChartIndex
        readonly property int stageCount: root.course ? charts.length : 1
        readonly property var charts: root.runner.chartDatas || [root.runner.chartData]
        readonly property var course: root.runner.course
        readonly property var chartData: root.course ? charts[stageIndex] : root.runner.chartData
        readonly property var players: root.runner.player2
            ? [root.runner.player1, root.runner.player2] : [root.runner.player1]
    }
    property QtObject lastGameplay: null
    property string lastArenaRoundId: ""
    property QtObject lastResult: null

    property QtObject arena: QtObject {
        property bool acceptResult: true
        property int submissions: 0
        property int endings: 0
        property var lastScore: null
        property string endedRoundId: ""
        property var presentedResult: ({ roundId: "" })

        function submitLocalResult(score) {
            submissions++;
            lastScore = score;
            if (!acceptResult) return false;
            presentedResult = { roundId: "arena-round" };
            return true;
        }

        function endResultPresentation(roundId) {
            endings++;
            endedRoundId = roundId;
            if (presentedResult.roundId === roundId) {
                presentedResult = { roundId: "" };
            }
        }
    }

    property Component resultFactory: Component { QtObject {} }

    property QtObject runner: QtObject {
        property int status: ChartRunner.Ready
        property int currentChartIndex: 0
        property int completedStages: 0
        property var chartDatas: root.course ? [{ md5: "first" }, { md5: "second" }] : undefined
        property var chartData: ({ md5: "single" })
        property var course: ({ name: "test course" })
        property var player1: ({ profile: "first player" })
        property var player2: null

        function start() { root.starts++; }
        function proceed() {
            const scores = ["stage " + root.runner.currentChartIndex];
            root.stageSaves++;
            root.runner.completedStages++;
            // CourseRunner does not notify currentChartIndex on the final
            // stage. Keep the index observed by QML bindings at that stage.
            if (root.runner.completedStages < root.runner.chartDatas.length) {
                root.runner.currentChartIndex = root.runner.completedStages;
                root.runner.player1 = { profile: "next player" };
                root.runner.status = ChartRunner.Ready;
            }
            return scores;
        }
        function finish() {
            if (root.course) root.courseSaves++;
            else root.stageSaves++;
            root.runner.status = ChartRunner.Finished;
            return ["saved result"];
        }
    }

    property QtObject navigation: QtObject {
        function openResult(scores, profiles, data, gameplay, arenaRoundId) {
            root.results++;
            root.lastScores = scores;
            root.lastProfiles = profiles;
            root.lastData = data;
            root.lastGameplay = gameplay;
            root.lastArenaRoundId = arenaRoundId;
            if (root.failPresentation) return null;
            root.active = false;
            root.lastResult = root.resultFactory.createObject(root);
            return root.lastResult;
        }
        function openCourseResult(scores, profiles, data, course) {
            root.courseResults++;
            root.lastScores = scores;
            root.lastProfiles = profiles;
            root.lastData = data;
            if (root.failPresentation) return null;
            root.active = false;
            return root.navigation;
        }
        function returnToPreviousScreen() {
            root.departures++;
            root.active = false;
            return root.navigation;
        }
    }

    property Implementation.GameplaySession session: Implementation.GameplaySession {
        gameplay: root.gameplay
        navigation: root.navigation
        arenaSession: root.arena
        active: root.active
        startReady: root.startReady
        finishReady: root.finishReady && !root.delaying
        onStageActivated: root.activations++
        onFinishRequested: {
            root.finishRequests++;
            if (root.animateFinish) root.delaying = true;
        }
        onPresentationFailed: root.failures++
    }
}

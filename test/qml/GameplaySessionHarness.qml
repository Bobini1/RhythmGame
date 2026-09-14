import QtQml
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

    property QtObject runner: QtObject {
        property int status: 0
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
                root.runner.status = 0;
            }
            return scores;
        }
        function finish() {
            if (root.course) root.courseSaves++;
            else root.stageSaves++;
            root.runner.status = 2;
            return ["saved result"];
        }
    }

    property QtObject navigation: QtObject {
        function openResult(scores, profiles, data) {
            root.results++;
            root.lastScores = scores;
            root.lastProfiles = profiles;
            root.lastData = data;
            if (root.failPresentation) return null;
            root.active = false;
            return root.navigation;
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
        chart: root.runner
        navigation: root.navigation
        active: root.active
        ready: root.runner.status === 0
        finished: root.runner.status === 2
        startReady: root.startReady
        finishReady: root.finishReady && !root.delaying
        arenaManaged: root.arenaManaged
        onStageActivated: root.activations++
        onFinishRequested: {
            root.finishRequests++;
            if (root.animateFinish) root.delaying = true;
        }
        onPresentationFailed: root.failures++
    }
}

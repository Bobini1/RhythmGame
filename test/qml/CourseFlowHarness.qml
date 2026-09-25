import QtQml
import ScreenContractsTest
import "../../RhythmGameQml" as Implementation

QtObject {
    id: root
    required property GameplayContext gameplay
    property bool active: true
    property var visits: []
    property var results: []
    property CourseResultContext courseResult: null
    property int departures: 0
    property int failures: 0

    property QtObject navigation: QtObject {
        function openResult(scores, profiles, data, origin, roundId) {
            const result = ScreenContexts.createResult(scores, profiles, data, origin, roundId);
            if (!result) return null;
            root.results = root.results.concat([result]);
            root.visits = root.visits.concat(["result " + result.stageIndex]);
            root.active = false;
            return result;
        }
        function openCourseResult(scores, profiles, charts, course) {
            const result = ScreenContexts.createCourseResult(scores, profiles, charts, course);
            if (!result) return null;
            root.courseResult = result;
            root.visits = root.visits.concat(["courseResult"]);
            root.active = false;
            return result;
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
        active: root.active
        onPresentationFailed: root.failures++
    }

    Component.onDestruction: {
        for (const result of root.results) result.destroy();
        if (root.courseResult) root.courseResult.destroy();
    }
}

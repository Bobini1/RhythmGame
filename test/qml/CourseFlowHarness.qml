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
        function openResult(result) {
            if (!result) return null;
            root.results = root.results.concat([result]);
            root.visits = root.visits.concat(["result " + result.stageIndex]);
            root.active = false;
            return result;
        }
        function openCourseResult(result) {
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
        startReady: false
        onPresentationFailed: root.failures++
    }

}

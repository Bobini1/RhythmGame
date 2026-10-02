import QtQuick
import RhythmGameQml

/*!
    \qmltype ScoreDetails
    \inqmlmodule RhythmGameQml
    \brief Loads the recording and gauges for a saved result.

    Set result to a BmsResult or BmsResultCourse from profile.scoreDb.
    The profile defaults to the main profile. Bind enabled to the screen or
    panel that needs the data. Changing either input or disabling the component
    cancels its request. Other consumers of the same score keep their requests.

    The score property contains a BmsScore or BmsScoreCourse when ready.
    Course scores include both their stages and a combined timeline for graphs.
    Imported scores can be ready without replayData or gaugeHistory.
    Use the events in JavaScript to calculate the statistics your theme needs.
    Saves and imports refresh an open request automatically. Loading errors
    are logged. Call reload to try again.
*/
Item {
    id: root
    visible: false

    /*! Result to load, or null when no saved run is selected. */
    property var result: null
    /*! Profile whose database contains result. */
    property Profile profile: Rg.profileList.mainProfile
    enum Status { Idle, Loading, Ready, Error }
    /*! Current loading state. Ready does not imply a recording exists. */
    readonly property int status: !enabled || !result || !profile ? ScoreDetails.Idle
        : !state.current ? ScoreDetails.Loading : state.status
    /*! Loaded score, or null while disabled, loading, or after failure. */
    readonly property var score: enabled && state.current ? state.score : null
    /*! Whether the requested score is loading. */
    readonly property bool loading: status === ScoreDetails.Loading

    QtObject {
        id: state
        property var requestedResult: null
        property var requestedProfile: null
        readonly property bool current: requestedResult === root.result
            && requestedProfile === root.profile
        property int status: ScoreDetails.Idle
        property var score: null
        property int generation: 0
    }

    PendingReplyGroup { id: replies }

    /*! Loads the current result again. */
    function reload() {
        const generation = ++state.generation;
        replies.cancelAll();
        state.score = null;
        state.requestedResult = root.result;
        state.requestedProfile = root.profile;
        state.status = ScoreDetails.Idle;
        if (!root.enabled || !root.result || !root.profile) return;
        state.status = ScoreDetails.Loading;
        replies.track(root.profile.scoreDb.getScore(root.result)).then(score => {
            if (generation !== state.generation || !root.enabled || !state.current) return;
            state.score = score;
            state.status = ScoreDetails.Ready;
        }, () => {
            if (generation === state.generation && state.current)
                state.status = ScoreDetails.Error;
        });
    }

    Connections {
        target: root.profile?.scoreDb ?? null
        enabled: root.enabled && !!root.result
        function onResultsChanged() { Qt.callLater(root.reload); }
    }

    onResultChanged: Qt.callLater(root.reload)
    onProfileChanged: Qt.callLater(root.reload)
    onEnabledChanged: Qt.callLater(root.reload)
    Component.onCompleted: Qt.callLater(root.reload)
}

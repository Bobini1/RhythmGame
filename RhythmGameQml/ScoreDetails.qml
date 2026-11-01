import QtQuick
import RhythmGameQml

/*!
    \qmltype ScoreDetails
    \inqmlmodule RhythmGameQml
    \brief Loads replay events and gauge history for a saved score summary.

    Use this component when a graph or other display needs more than the
    points and clear type in a cached result. Set \l result to a BmsResult or
    BmsResultCourse from the profile's score database, and set \l profile to
    the score's owner. The profile defaults to the main profile.

    Selection exposes summaries through StandardSelectState::results.
    Other screens can read \c profile.scoreDb.chartResults[md5.toUpperCase()]
    or \c profile.scoreDb.courseResults[identifier]. Each value is a list of
    summaries; use an empty list when the key is absent.

    \l score is null while loading or after failure. When ready, it contains
    a BmsScore or BmsScoreCourse. Check its \c replayData and \c gaugeHistory
    separately: an imported score can be ready without either. A course score
    includes individual stage scores and combined data for graphs.

    Place the component inside the screen or panel that needs it. Changing
    result or profile, disabling it, or destroying it cancels its request.
    Other consumers of the same score keep their requests. Saves and imports
    refresh the data automatically. Call \l reload to retry after an error.

    \section1 Display the loading state

    \table
        \header \li Status \li What to show
        \row \li \c ScoreDetails.Idle \li Result or profile is missing, or the component is disabled.
        \row \li \c ScoreDetails.Loading \li A loading message; score is null.
        \row \li \c ScoreDetails.Ready \li The score, with separate messages for missing graph data.
        \row \li \c ScoreDetails.Error \li A failure message and a button calling reload().
    \endtable

    See the \l {../theme_tutorial_select.html}{chart details example} for a
    graph that distinguishes these states.
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

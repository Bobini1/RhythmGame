pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectActions
    \inqmlmodule RhythmGameQml
    \brief Opens playable selections and handles leaving selection.

    StandardSelectState already includes this component. Use it separately if
    your theme implements its own browsing state. \l openPlayable uses the current
    solo or battle profiles outside Arena. Replays use the main profile alone.
    In an Arena room it submits the chart
    choice to the session, and it handles a course choice without opening it.

    A true return from \l openPlayable means the request was handled. It does
    not guarantee that gameplay opened. Unsupported items return false.

    Set \l tryOpenPlayableAction to handle a request before the standard action.
    The function receives \c {item, autoplay, replay, replayScore}. Return true
    when it handles the request, or false to continue with the standard action.

    Call \l exit to leave selection. In Arena it leaves the room first, then
    returns to the browser. A room lost while another screen is open returns
    to the browser when selection becomes active again. Disabling this component
    suspends navigation and input.

    StandardArenaSelectOverlay supplies the Arena ready controls and shortcut.
*/
Item {
    id: root

    /*!
        Called as \c tryOpenPlayableAction(item,autoplay,replay,replayScore). Return true to
        handle the request, or false to continue with the standard action.
    */
    property var tryOpenPlayableAction: null
    /*! Calls \c exitAction() in place of leaving selection. */
    property var exitAction: null
    /*! Reports whether the local player is currently seated in an Arena room. */
    readonly property bool arenaSeated:
        Rg.arenaSession.state === ArenaSession.InRoom
        || Rg.arenaSession.state === ArenaSession.Reconnecting
    /*! Whether a saved result or replay is waiting for its full score. */
    readonly property bool loadingScore: state.loadingScore

    QtObject {
        id: state
        property bool arenaSelection: false
        property bool leaving: false
        property bool loadingScore: false
        property int scoreGeneration: 0

        function withScore(result, open) {
            if (state.loadingScore) return false;
            if (result.result) return open(result);
            const profile = Rg.profileList.mainProfile;
            const generation = ++state.scoreGeneration;
            state.loadingScore = true;
            scoreReplies.track(profile.scoreDb.getScore(result)).then(score => {
                if (generation !== state.scoreGeneration) return;
                state.loadingScore = false;
                if (root.enabled && !root.arenaSeated && profile === Rg.profileList.mainProfile)
                    open(score);
            }, () => {
                if (generation === state.scoreGeneration) state.loadingScore = false;
            });
            return true;
        }

        function returnFromRoom() {
            if (root.enabled && state.arenaSelection && !root.arenaSeated && !state.leaving) {
                state.leaving = !!globalRoot.returnToPreviousScreen();
            }
        }
    }

    PendingReplyGroup { id: scoreReplies }

    /*! Cancels a pending saved-result or replay action. */
    function cancelScoreLoading() {
        ++state.scoreGeneration;
        scoreReplies.cancelAll();
        state.loadingScore = false;
    }

    Connections {
        target: Rg.profileList
        function onMainProfileChanged() { root.cancelScoreLoading(); }
        function onBattleActiveChanged() { root.cancelScoreLoading(); }
    }
    Connections {
        target: Rg.profileList.battleProfiles
        function onPlayer1ProfileChanged() { root.cancelScoreLoading(); }
        function onPlayer2ProfileChanged() { root.cancelScoreLoading(); }
    }

    Component.onCompleted: state.arenaSelection = root.arenaSeated
    onArenaSeatedChanged: {
        if (arenaSeated) root.cancelScoreLoading();
        Qt.callLater(state.returnFromRoom);
    }
    onEnabledChanged: {
        if (!enabled) root.cancelScoreLoading();
        Qt.callLater(state.returnFromRoom);
    }

    /*! Leaves the room in Arena, or closes local selection. Folder history is separate. */
    function exit() {
        if (!root.enabled || state.leaving) return false;
        root.cancelScoreLoading();
        if (typeof root.exitAction === "function") {
            root.exitAction();
        } else if (root.arenaSeated) {
            Rg.arenaSession.leaveRoom();
        } else {
            state.leaving = !!globalRoot.returnToPreviousScreen();
        }
        return true;
    }

    /*!
        Opens \a score for chart or course \a item using the main profile.
        Supply a cached BmsResult/BmsResultCourse or a full BmsScore/BmsScoreCourse
        belonging to that profile. Cached results are loaded before opening.

        Returns false for unsupported input, inactive selection or Arena.
        For an asynchronous load, true means the request was accepted; it does
        not guarantee the result screen has opened. \l loadingScore tracks the
        wait, and \l cancelScoreLoading cancels it.
    */
    function openResult(item, score) {
        if (!root.enabled || root.arenaSeated || !score) return false;
        if (!(item instanceof course) && !(item instanceof ChartData)) return false;
        const profile = Rg.profileList.mainProfile;
        return state.withScore(score, loaded => item instanceof course
            ? !!globalRoot.openSavedCourseResult(loaded, profile, item)
            : !!globalRoot.openSavedResult(loaded, profile, item));
    }

    /*!
        Opens installed chart or course \a item. \a autoplay selects automatic
        play; \a replay selects a saved recording. Both default to false.
        \a replayScore accepts a cached result or full score and defaults to null.
        A replay needs recording data; a score without replay selects its saved
        note pattern for G-Battle. Do not combine autoplay and replay.

        Uses the active solo or battle profiles. Replay always uses the main
        profile alone. In Arena, manual chart activation proposes the chart;
        courses, autoplay and replay are handled without starting local play.
        True means the request was handled, including a rejected course or an
        asynchronous load. It does not guarantee that gameplay opened.
    */
    function openPlayable(item, autoplay = false, replay = false,
                          replayScore = null) {
        if (!root.enabled) return false;
        if (typeof tryOpenPlayableAction === "function"
                && tryOpenPlayableAction(
                    item, autoplay, replay, replayScore)) {
            return true;
        }
        const isChart = item instanceof ChartData;
        if (!isChart && !(item instanceof course)) {
            return false;
        }
        if (!isChart && item.unavailableReason) {
            console.warn(item.unavailableReason);
            return true;
        }
        if (root.arenaSeated) {
            if (isChart && !autoplay && !replay && !replayScore) {
                Rg.arenaSession.selectChart(item);
            }
            return true;
        }

        const profiles = Rg.profileList;
        if (replay && (!replayScore || !(replayScore.result || replayScore).replayAvailable)) return false;
        if (state.loadingScore) return false;
        const battle = profiles.battleActive && !replay;
        const player1 = battle
            ? profiles.battleProfiles.player1Profile : profiles.mainProfile;
        const player2 = battle
            ? profiles.battleProfiles.player2Profile : null;
        const useReplay = !!replay && !!replayScore;
        const autoplay2 = battle && !!autoplay;
        const open = function(score) {
            const players = [{ profile: player1, autoplay: !!autoplay, replay: useReplay, score }];
            if (player2) players.push({ profile: player2, autoplay: autoplay2 });
            if (isChart) globalRoot.openChart(item.path, players);
            else globalRoot.openCourse(item, players);
            return true;
        };
        return replayScore ? state.withScore(replayScore, open) : open(null);
    }
}

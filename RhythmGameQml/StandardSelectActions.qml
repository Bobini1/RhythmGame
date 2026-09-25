pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectActions
    \inqmlmodule RhythmGameQml
    \brief Opens playable selections and handles leaving selection and Arena ready input.

    StandardSelectState already includes this component. Use it separately if
    your theme implements its own browsing state. \l openPlayable uses the current
    solo or battle profiles outside Arena. In an Arena room it submits the chart
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

    StandardSelectInput forwards Start presses to \l handleStartPress. A custom
    input handler can call it directly. Pressing Start twice on the same player
    side toggles ready when the room allows it.
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
    /*! Describes the ready gesture for an Arena panel. */
    readonly property string readyShortcutDescription:
        qsTranslate("ArenaLegacySelectOverlay", "Press Start twice to toggle ready.")
    /*! Reports whether the local player is currently seated in an Arena room. */
    readonly property bool arenaSeated:
        Rg.arenaSession.state === ArenaSession.InRoom
        || Rg.arenaSession.state === ArenaSession.Reconnecting

    QtObject {
        id: state
        property bool arenaSelection: false
        property bool leaving: false
    }
    Timer { id: firstStart; interval: 500 }
    Timer { id: secondStart; interval: 500 }

    function returnFromRoom() {
        if (root.enabled && state.arenaSelection && !root.arenaSeated && !state.leaving) {
            state.leaving = !!globalRoot.returnToPreviousScreen();
        }
    }

    Component.onCompleted: state.arenaSelection = root.arenaSeated
    onArenaSeatedChanged: Qt.callLater(root.returnFromRoom)
    onEnabledChanged: {
        firstStart.stop();
        secondStart.stop();
        Qt.callLater(root.returnFromRoom);
    }

    /*! Leaves the room in Arena, or closes local selection. Folder history is separate. */
    function exit() {
        if (!root.enabled || state.leaving) return false;
        if (typeof root.exitAction === "function") {
            root.exitAction();
        } else if (root.arenaSeated) {
            Rg.arenaSession.leaveRoom();
        } else {
            state.leaving = !!globalRoot.returnToPreviousScreen();
        }
        return true;
    }

    /*! Toggles ready when the Arena session permits it. Returns whether it changed. */
    function toggleReady() {
        const session = Rg.arenaSession;
        if (!root.enabled || !root.arenaSeated || !session.roundsAvailable
                || session.currentRoundId.length > 0 || (!session.ready && !session.canReady)) {
            return false;
        }
        session.setReady(!session.ready);
        return true;
    }

    /*! Handles a Start \a key. Returns true for the second press of the ready gesture. */
    function handleStartPress(key) {
        if (!root.enabled || !root.arenaSeated
                || (key !== BmsKey.Start1 && key !== BmsKey.Start2)) {
            return false;
        }
        const timer = key === BmsKey.Start2 ? secondStart : firstStart;
        if (!timer.running) {
            timer.start();
            return false;
        }
        timer.stop();
        root.toggleReady();
        return true;
    }

    /*! Opens a saved chart or course result for \a item and \a score. */
    function openResult(item, score) {
        if (!root.enabled || root.arenaSeated || !score) return false;
        const profiles = [Rg.profileList.mainProfile];
        if (item instanceof course) {
            return !!globalRoot.openCourseResult([score], profiles, item.loadCharts(), item);
        }
        return item instanceof ChartData
            && !!globalRoot.openResult([score], profiles, item);
    }

    /*!
        Opens \a item using \a autoplay, \a replay, and \a replayScore to select the requested
        play mode.
    */
    function openPlayable(item, autoplay = false, replay = false,
                          replayScore = null) {
        if (!root.enabled) return false;
        if (typeof tryOpenPlayableAction === "function"
                && tryOpenPlayableAction(
                    item, autoplay, replay, replayScore)) {
            return true;
        }
        if (item instanceof ChartData) {
            if (arenaSeated) {
                if (!autoplay && !replay && !replayScore) {
                    Rg.arenaSession.selectChart(item);
                }
                return true;
            }
            let useReplay = !!replay && !!replayScore;
            if (Rg.profileList.battleActive) {
                globalRoot.openChart(
                    item.path, Rg.profileList.battleProfiles.player1Profile,
                    !!autoplay, useReplay, replayScore || null,
                    Rg.profileList.battleProfiles.player2Profile,
                    !!autoplay, false, null);
            } else {
                globalRoot.openChart(
                    item.path, Rg.profileList.mainProfile, !!autoplay,
                    useReplay, replayScore || null, null, false, false, null);
            }
            return true;
        }
        if (item instanceof course) {
            if (item.unavailableReason) {
                console.warn(item.unavailableReason);
                return true;
            }
            if (arenaSeated) {
                return true;
            }
            let useReplay = !!replay && !!replayScore;
            if (Rg.profileList.battleActive) {
                globalRoot.openCourse(
                    item, Rg.profileList.battleProfiles.player1Profile,
                    !!autoplay, useReplay, replayScore || null,
                    Rg.profileList.battleProfiles.player2Profile,
                    !!autoplay, false, null);
            } else {
                globalRoot.openCourse(
                    item, Rg.profileList.mainProfile, !!autoplay,
                    useReplay, replayScore || null, null, false, false, null);
            }
            return true;
        }
        return false;
    }
}

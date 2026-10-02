import QtQuick
import QtQuick.Controls
import RhythmGameQml

/*!
    \qmltype StandardMultiplayerFlow
    \inqmlmodule RhythmGameQml
    \brief Connects the Arena browser and opens room selection and gameplay.

    Place this component directly inside a multiplayer screen. Use \l session
    for the room list, createRoom(), joinRoom() and retry(). Call \l close from
    the screen's Back button. Removing the browser disconnects from Arena.

    Joining a room opens the configured selector on the ordinary screen stack.
    StandardSelectController returns to the browser when the room is left.
    Scheduled rounds open gameplay even while another screen, such as Settings,
    covers the selector. Closing gameplay returns to that retained screen.
*/
Item {
    id: root

    /*! The multiplayer screen that contains this flow. */
    property Item screen: parent
    /*! The Arena session used by this browser. */
    readonly property ArenaSession session: Rg.arenaSession

    QtObject {
        id: state
        property Item gameplayScreen: null
        readonly property bool active: root.enabled && globalRoot.currentScreen === root.screen
        readonly property bool inRoom: root.session.state === ArenaSession.InRoom
            || root.session.state === ArenaSession.Reconnecting

        function openRoom() {
            return state.active && state.inRoom ? globalRoot.openSelect() : null;
        }

        function openPreparedGameplay(runner) {
            if (!runner) {
                state.cancelPreparedGameplay();
            } else if (!state.gameplayScreen) {
                const session = root.session;
                const screen = globalRoot.openGameplay(
                    ScreenContexts.createGameplay(runner, session));
                if (screen) {
                    screen.StackView.removed.connect(() => session.releasePreparedGameplay(runner));
                } else {
                    session.abandonCurrentRound();
                }
                state.gameplayScreen = screen;
            }
        }

        function cancelPreparedGameplay() {
            const screen = state.gameplayScreen;
            state.gameplayScreen = null;
            if (screen && globalRoot.currentScreen === screen
                    && screen.gameplay.status !== ChartRunner.Finished) {
                globalRoot.returnToPreviousScreen();
            }
        }

        onActiveChanged: Qt.callLater(state.openRoom)
        onInRoomChanged: Qt.callLater(state.openRoom)
    }

    /*! Closes this browser when it is the current screen. */
    function close() {
        return state.active ? globalRoot.returnToPreviousScreen() : null;
    }

    Component.onCompleted: root.session.connectForBrowsing()
    Component.onDestruction: root.session.exitArena()

    Connections {
        target: root.session
        function onPreparedGameplayChanged(runner) { state.openPreparedGameplay(runner); }
        function onRoundLaunchCancelled() { state.cancelPreparedGameplay(); }
    }
}

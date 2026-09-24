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

    readonly property bool active: root.enabled && globalRoot.currentScreen === root.screen
    readonly property bool inRoom: root.session.state === ArenaSession.InRoom
        || root.session.state === ArenaSession.Reconnecting

    QtObject {
        id: state
        property Item gameplayScreen: null
    }

    /*! Opens the room selector, or returns null if the browser is inactive or loading fails. */
    function openRoom() {
        return root.active && root.inRoom ? globalRoot.openSelect() : null;
    }

    /*! Closes this browser when it is the current screen. */
    function close() {
        return root.active ? globalRoot.returnToPreviousScreen() : null;
    }

    function openPreparedGameplay(runner) {
        if (!runner) {
            cancelPreparedGameplay();
        } else if (!state.gameplayScreen) {
            const session = root.session;
            const screen = globalRoot.openGameplay(
                ScreenContexts.createGameplay(runner, session));
            if (screen) {
                screen.StackView.removed.connect(() => session.releasePreparedGameplay(runner));
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

    onActiveChanged: Qt.callLater(root.openRoom)
    onInRoomChanged: Qt.callLater(root.openRoom)
    Component.onCompleted: root.session.connectForBrowsing()
    Component.onDestruction: root.session.exitArena()

    Connections {
        target: root.session
        function onPreparedGameplayChanged(runner) { root.openPreparedGameplay(runner); }
        function onRoundLaunchCancelled() { root.cancelPreparedGameplay(); }
    }
}

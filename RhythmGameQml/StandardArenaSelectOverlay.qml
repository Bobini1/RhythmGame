pragma ComponentBehavior: Bound
pragma Translator: "ArenaSelectOverlay"
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardArenaSelectOverlay
    \inqmlmodule RhythmGameQml
    \inherits StandardArenaOverlay
    \brief Shows the Arena room, ready controls and chat on song selection.

    Place this component in a selection screen. It appears while seated in an
    Arena room, including reconnection. Bind \l navigationFocusTarget to the
    song list so closing chat can return keyboard focus there.

    Pressing Start twice on the same player side toggles ready. Set
    \l readyShortcutEnabled to false to provide your own ready input.
    The panel uses \l readyShortcutDescription for its hint. Replacing
    \c panelComponent keeps the shortcut. Hidden or disabled overlays do not
    handle ready input.

    A custom Ready button calls \l toggleReady and binds its enabled state to
    \l canToggleReady. Use \l ready for its label or check state and
    \l readyDisabledReason for a hint. The button and double-Start gesture use
    the same rules, including cancelling readiness. Disabling the shortcut
    leaves the public action available.

    The panel supplies its own chat tab. The chat shortcut is disabled by default
    because some selectors use F8 for another action. Set \c chatShortcutEnabled
    to true to enable it. Replace \c panelComponent to supply your own room UI.
*/
StandardArenaOverlay {
    id: root

    /*! Receives keyboard focus after chat closes. Defaults to the parent item. */
    property Item navigationFocusTarget: parent
    /*! Enables the double-Start ready gesture while the overlay is presented. */
    property bool readyShortcutEnabled: true
    /*! Describes the ready gesture. Empty when \l readyShortcutEnabled is false. */
    readonly property string readyShortcutDescription: root.readyShortcutEnabled
        ? qsTranslate("ArenaLegacySelectOverlay", "Press Start twice to toggle ready.") : ""
    /*! Whether the local player is ready or the round is already being prepared. */
    readonly property bool ready: !!root.session
        && (root.session.ready || root.session.currentRoundId.length > 0)
    /*! Whether a button or gesture can change readiness now. */
    readonly property bool canToggleReady: root.presentationActive && !!root.session
        && root.session.state === ArenaSession.InRoom && root.session.roundsAvailable
        && root.session.currentRoundId.length === 0
        && (root.session.ready || root.session.canReady)
    /*! Explains why readiness is unavailable. Empty when the room permits the action. */
    readonly property string readyDisabledReason: {
        if (!root.session || root.session.state !== ArenaSession.InRoom)
            return qsTr("Wait for the room connection.");
        if (!root.session.roundsAvailable)
            return qsTranslate("ArenaSelectPanel", "Update required to play in this room.");
        if (root.session.currentRoundId.length > 0)
            return qsTranslate("ArenaSelectPanel", "The synchronized round is already being prepared.");
        if (!root.session.ready && !root.session.canReady) {
            return root.session.availabilitySyncing
                ? qsTranslate("ArenaSelectPanel", "Song libraries are still being compared.")
                : qsTranslate("ArenaSelectPanel", "Choose a chart available to everyone before becoming ready.");
        }
        return "";
    }

    /*! Toggles readiness for a button or custom gesture. Returns false when unavailable. */
    function toggleReady() {
        if (!root.canToggleReady) return false;
        root.session.setReady(!root.session.ready);
        return true;
    }

    active: root.session.state === ArenaSession.InRoom
        || root.session.state === ArenaSession.Reconnecting
    objectName: "arenaSelectPlacementFrame"
    customizationLabel: qsTr("Arena song-select panel")
    chatShortcutEnabled: false
    layoutVariant: "select"
    minimumPixelSize: Qt.size(420, 320)
    visibilityCustomizable: false

    Item {
        id: readyInput

        enabled: root.presentationActive && root.readyShortcutEnabled
            && root.session && (root.session.state === ArenaSession.InRoom
                || root.session.state === ArenaSession.Reconnecting)

        Timer { id: firstStart; interval: 500 }
        Timer { id: secondStart; interval: 500 }

        onEnabledChanged: {
            firstStart.stop();
            secondStart.stop();
        }

        function press(timer) {
            if (!readyInput.enabled) return;
            if (!timer.running) {
                timer.start();
                return;
            }
            timer.stop();
            root.toggleReady();
        }

        Input.onStart1Pressed: readyInput.press(firstStart)
        Input.onStart2Pressed: readyInput.press(secondStart)
    }

    panelComponent: ArenaSelectPanel {
        navigationFocusTarget: root.navigationFocusTarget
        readyShortcutDescription: root.readyShortcutDescription
        ready: root.ready
        canToggleReady: root.canToggleReady
        readyDisabledReason: root.readyDisabledReason
        session: root.session
        onReadyToggleRequested: root.toggleReady()
        onChatSelected: chat => root.setChatOpen(chat)
    }
}

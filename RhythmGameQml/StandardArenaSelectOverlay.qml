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

    The panel supplies its own chat tab. The chat shortcut is disabled by default
    because some selectors use F8 for another action. Set \c chatShortcutEnabled
    to true to enable it. Replace \c panelComponent to supply your own room UI.
*/
StandardArenaOverlay {
    id: root

    /*! Receives keyboard focus after chat closes. Defaults to the parent item. */
    property Item navigationFocusTarget: parent
    /*! Describes the ready gesture provided by the selector's input handler. */
    property string readyShortcutDescription: ""

    active: root.session.state === ArenaSession.InRoom
        || root.session.state === ArenaSession.Reconnecting
    objectName: "arenaSelectPlacementFrame"
    customizationLabel: qsTr("Arena song-select panel")
    chatShortcutEnabled: false
    layoutVariant: "select"
    minimumPixelSize: Qt.size(420, 320)
    placementKind: "selectRoom"
    visibilityCustomizable: false

    panelComponent: ArenaSelectPanel {
        navigationFocusTarget: root.navigationFocusTarget
        readyShortcutDescription: root.readyShortcutDescription
        session: root.session
        onChatSelected: chat => root.setChatSelected(chat)
    }
}

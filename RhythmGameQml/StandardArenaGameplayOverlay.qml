pragma ComponentBehavior: Bound
pragma Translator: "ArenaOverlayHost"
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardArenaGameplayOverlay
    \inqmlmodule RhythmGameQml
    \inherits StandardArenaOverlay
    \brief Shows Arena standings and chat during gameplay.

    Bind \l gameplay to the screen's supplied context. The overlay stays hidden
    for local play and courses. F8 switches between standings and chat while
    this screen is active. Set \c chatShortcutEnabled to false to use your own
    input, or replace \c panelComponent to draw another panel.
*/
StandardArenaOverlay {
    id: root

    /*! Supplies the gameplay context for this screen. */
    required property GameplayContext gameplay

    active: root.gameplay?.arenaActive ?? false
    objectName: "arenaGameplayPlacementFrame"
    customizationLabel: qsTr("Arena gameplay panel")
    defaultExpanded: false
    layoutVariant: root.gameplay ? "k" + root.gameplay.keymode : ""
    placementKind: "gameplayLeaderboard"
    minimumPixelSize: Qt.size(320, 240)

    panelComponent: ArenaGameplayOverlay {
        session: root.session
        expanded: root.expanded
        onChatSelected: chat => root.setChatSelected(chat)
        onExpandedChanged: root.setExpanded(expanded)
    }
}

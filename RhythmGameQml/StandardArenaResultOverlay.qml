pragma ComponentBehavior: Bound
pragma Translator: "ArenaOverlayHost"
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardArenaResultOverlay
    \inqmlmodule RhythmGameQml
    \inherits StandardArenaOverlay
    \brief Shows standings and chat for an Arena result.

    Bind \l result to the screen's ResultContext. The panel appears only while
    its round matches the session's presented result. Local results and saved
    scores have no round ID and do not show the panel. Course summaries use
    CourseResultContext and do not need this component.

    F8 switches between standings and chat. A theme can replace \c panelComponent
    while keeping placement and chat behavior, as Default does for its result
    design. Disable result confirmation while the matching round's chat is open
    so typing a message does not close the result screen.
*/
StandardArenaOverlay {
    id: root

    /*! Supplies a chart result, or null when no chart result is presented. */
    property ResultContext result: null

    active: root.result?.arenaActive ?? false
    objectName: "arenaResultPlacementFrame"
    customizationLabel: qsTr("Arena result panel")
    defaultExpanded: true
    layoutVariant: "result"
    placementKind: "resultStandings"

    panelComponent: ArenaResultOverlay {
        session: root.session
        expanded: root.expanded
        onChatSelected: chat => root.setChatSelected(chat)
        onExpandedChanged: root.setExpanded(expanded)
    }
}

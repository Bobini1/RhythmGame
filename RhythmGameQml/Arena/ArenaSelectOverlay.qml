pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root

    required property var session
    required property var themeVars
    required property Item viewport
    property Item navigationFocusTarget: null
    property rect defaultPixelRectHint: Qt.rect(0, 0, 0, 0)
    readonly property alias placementFrame: placementFrame
    readonly property alias panel: panel

    anchors.fill: parent

    ArenaOverlayPlacementFrame {
        id: placementFrame

        objectName: "arenaSelectPlacementFrame"
        defaultPixelRectHint: root.defaultPixelRectHint
        layoutVariant: "select"
        minimumPixelSize: Qt.size(420, 320)
        moveHandle: panel.dragHandle
        placementKind: "selectRoom"
        themeVars: root.themeVars
        viewport: root.viewport

        ArenaSelectPanel {
            id: panel

            anchors.fill: parent
            navigationFocusTarget: root.navigationFocusTarget
            session: root.session
        }
    }
}

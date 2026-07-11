pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root

    required property var session
    required property var themeVars
    required property Item viewport
    readonly property alias placementFrame: placementFrame
    readonly property alias panel: panel

    anchors.fill: parent

    ArenaOverlayPlacementFrame {
        id: placementFrame

        objectName: "arenaSelectPlacementFrame"
        customizeMode: false
        directMoveEnabled: true
        directResizeEnabled: true
        layoutVariant: "select"
        minimumPixelSize: Qt.size(520, 320)
        moveHandle: panel.dragHandle
        placementKind: "selectRoom"
        themeVars: root.themeVars
        viewport: root.viewport

        ArenaSelectPanel {
            id: panel

            anchors.fill: parent
            session: root.session
        }
    }
}

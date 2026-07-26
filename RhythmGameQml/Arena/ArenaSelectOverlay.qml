pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root

    required property var session
    required property var themeVars
    required property Item viewport
    property Item navigationFocusTarget: null
    property rect defaultPixelRectHint: Qt.rect(0, 0, 0, 0)
    property bool customizeMode: false
    property bool presentationActive: true
    readonly property alias placementFrame: selectPlacementFrame
    readonly property alias panel: panel

    anchors.fill: parent

    ArenaOverlayPlacementFrame {
        id: selectPlacementFrame

        objectName: "arenaSelectPlacementFrame"
        customizationLabel: qsTr("Arena song-select panel")
        customizeMode: root.customizeMode
        defaultPixelRectHint: root.defaultPixelRectHint
        layoutVariant: "select"
        minimumPixelSize: Qt.size(420, 320)
        moveHandle: panel.dragHandle
        placementKind: "selectRoom"
        themeVars: root.themeVars
        viewport: root.viewport
        visible: selectPlacementFrame.overlayVisible || root.customizeMode

        onOverlayVisibilityCommitted: visible => {
            if (!visible && root.session.chatOpen === true) {
                root.session.setChatOpen(false);
            }
        }
        onPresentationStateReloaded: {
            if (root.presentationActive
                    && selectPlacementFrame.overlayVisible) {
                selectPlacementFrame.restoreChatSelection(root.session);
            }
        }

        ArenaSelectPanel {
            id: panel

            anchors.fill: parent
            navigationFocusTarget: root.navigationFocusTarget
            session: root.session

            onChatSelected: chat => selectPlacementFrame.setChatSelected(chat)
        }
    }

    Component.onCompleted: {
        if (root.presentationActive && selectPlacementFrame.overlayVisible) {
            selectPlacementFrame.restoreChatSelection(root.session);
        }
    }
    onPresentationActiveChanged: {
        if (root.presentationActive && selectPlacementFrame.overlayVisible) {
            selectPlacementFrame.restoreChatSelection(root.session);
        }
    }
}

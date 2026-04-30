pragma ValueTypeBehavior: Addressable

import QtQuick

MouseArea {
    id: readmePointer

    required property var screenRoot
    required property Item skinItem
    property real skinScale: 1

    readonly property var root: screenRoot
    readonly property bool ready: root !== undefined
        && root !== null
        && skinItem !== undefined
        && skinItem !== null

    enabled: ready
        && root.screenUpdatesActive
        && root.effectiveScreenKey === "select"
        && root.lr2ReadmeMode === 1
    visible: enabled
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    hoverEnabled: true
    z: 100250

    function updateReadmeMouse(mouse) {
        if (!ready || !mouse) {
            return false;
        }
        const point = readmePointer.mapToItem(skinItem, mouse.x, mouse.y);
        root.lr2ReadmeMouseX = point.x / skinScale;
        root.lr2ReadmeMouseY = point.y / skinScale;
        return true;
    }

    onPressed: (mouse) => {
        if (!updateReadmeMouse(mouse)) {
            return;
        }
        if (mouse.button === Qt.RightButton) {
            root.closeReadme();
        } else {
            root.lr2ReadmeMouseHeld = true;
        }
        mouse.accepted = true;
    }

    onPositionChanged: (mouse) => {
        if (!updateReadmeMouse(mouse)) {
            return;
        }
        mouse.accepted = true;
    }

    onReleased: (mouse) => {
        if (!updateReadmeMouse(mouse)) {
            return;
        }
        root.lr2ReadmeMouseHeld = false;
        mouse.accepted = true;
    }

    onCanceled: {
        if (ready) {
            root.lr2ReadmeMouseHeld = false;
        }
    }
    onEnabledChanged: {
        if (!enabled && ready) {
            root.lr2ReadmeMouseHeld = false;
        }
    }
    onWheel: (wheel) => {
        if (ready) {
            root.handleSelectWheel(wheel);
        }
    }
}

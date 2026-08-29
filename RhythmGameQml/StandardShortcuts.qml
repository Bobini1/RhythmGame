import QtQuick

// Common application shortcuts. Each action works through globalRoot by
// default and can be replaced by assigning the corresponding action property.
Item {
    id: root

    property var toggleFpsOverlayAction: null
    property var toggleFullscreenAction: null
    property var openSettingsAction: null
    property bool settingsEnabled: true

    function run(overrideAction, defaultAction) {
        if (typeof overrideAction === "function") {
            return overrideAction();
        }
        return defaultAction();
    }

    Shortcut {
        autoRepeat: false
        enabled: root.enabled
        sequence: "F1"
        onActivated: root.run(root.toggleFpsOverlayAction,
                              () => globalRoot.toggleFpsOverlay())
    }

    Shortcut {
        autoRepeat: false
        enabled: root.enabled
        sequence: "F4"
        onActivated: root.run(root.toggleFullscreenAction,
                              () => globalRoot.toggleFullScreen())
    }

    Shortcut {
        autoRepeat: false
        enabled: root.enabled && root.settingsEnabled
        sequence: "F12"
        onActivated: root.run(root.openSettingsAction,
                              () => globalRoot.openSettings())
    }
}

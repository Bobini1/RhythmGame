import QtQuick

/*!
    \qmltype StandardShortcuts
    \inqmlmodule RhythmGameQml
    \brief Provides common application-wide shortcuts.

    Each shortcut works through \c globalRoot by default and can be replaced by
    assigning the corresponding action property.
*/
Item {
    id: root

    /*! Optional replacement for toggling the FPS overlay with F1. */
    property var toggleFpsOverlayAction: null
    /*! Optional replacement for toggling fullscreen with F4. */
    property var toggleFullscreenAction: null
    /*! Optional replacement for opening settings with F12. */
    property var openSettingsAction: null
    /*! Whether the F12 settings shortcut is active. */
    property bool settingsEnabled: true

    function run(overrideAction, defaultAction) {
        if (typeof overrideAction === "function") {
            overrideAction();
            return true;
        }
        defaultAction();
        return true;
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

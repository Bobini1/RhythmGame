import QtQuick

/*!
    \qmltype StandardShortcuts
    \inqmlmodule RhythmGameQml
    \brief Provides fixed application-wide shortcuts.

    The application content frame owns this component. F1 toggles the FPS
    overlay and F4 toggles fullscreen. Screen-specific shortcuts belong to
    their corresponding screen components.
*/
Item {
    id: root

    Shortcut {
        autoRepeat: false
        enabled: root.enabled
        sequence: "F1"
        onActivated: globalRoot.toggleFpsOverlay()
    }

    Shortcut {
        autoRepeat: false
        enabled: root.enabled
        sequence: "F4"
        onActivated: globalRoot.toggleFullScreen()
    }
}

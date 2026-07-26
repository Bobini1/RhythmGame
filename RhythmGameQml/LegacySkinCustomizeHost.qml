pragma ComponentBehavior: Bound

import QtQuick

Loader {
    id: root

    required property var screen

    active: screen !== null
        && screen.legacySkinCustomizeAvailable === true
        && screen.customizeMode === true
    anchors.fill: parent
    sourceComponent: LegacySkinCustomizeOverlay {
        screen: root.screen
    }
}

pragma ComponentBehavior: Bound

import QtQuick

FocusScope {
    id: root

    required property var session
    required property var presentationItem
    required property var themeVars
    required property Item viewport
    readonly property bool nativePresentation: presentationItem !== null
        && presentationItem.arenaNativeSelectPresentation !== undefined
        && presentationItem.arenaNativeSelectPresentation === true

    objectName: "arenaLegacySelectOverlay"
    anchors.fill: parent
    visible: !nativePresentation

    Accessible.name: qsTr("Arena room overlay")
    Accessible.role: Accessible.Pane

    ArenaSelectOverlay {
        customizeMode: root.presentationItem
            && root.presentationItem.customizeMode === true
        navigationFocusTarget: root.presentationItem
        presentationActive: root.presentationItem
            && (root.presentationItem.screenUpdatesActive !== undefined
                ? root.presentationItem.screenUpdatesActive === true
                : root.presentationItem.enabled && root.presentationItem.visible)
        session: root.session
        themeVars: root.themeVars
        viewport: root.viewport
    }

    LegacySkinCustomizeHost {
        anchors.fill: parent
        screen: root.presentationItem
        z: 2000000
    }

    TransientInputFocusDismissLayer {}
}

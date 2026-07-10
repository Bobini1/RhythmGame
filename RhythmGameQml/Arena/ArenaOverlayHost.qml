pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: root

    required property var session
    required property var currentItem
    required property string resolvedSkinId
    required property string layoutVariant
    property bool expanded: false
    property bool customizeMode: false
    property var coordinatedScreen: null
    property bool customizationTransitionActive: false

    readonly property bool ownsArenaRunner: root.currentItem !== null
        && root.currentItem.chart !== undefined
        && root.session.arenaGameplayActive === true
        && root.session.arenaRunner !== null
        && root.currentItem.chart === root.session.arenaRunner
    readonly property bool ownsArenaResult: root.currentItem !== null
        && root.session.resultPresentationActive === true
        && (root.currentItem.chart === undefined
            || root.currentItem.chart === null)
    readonly property bool arenaShortcutEnabled: root.ownsArenaRunner
        || root.ownsArenaResult
    readonly property bool screenCoordinatesCustomization: root.currentItem !== null
        && typeof root.currentItem.setArenaCustomizeMode === "function"

    function setCustomizeMode(active) {
        const accepted = !!active && root.arenaShortcutEnabled;
        if (root.customizationTransitionActive
                || (root.customizeMode === accepted
                    && (!accepted
                        || root.coordinatedScreen === root.currentItem))) {
            return;
        }
        root.customizationTransitionActive = true;
        const previousScreen = root.coordinatedScreen;
        const nextScreen = accepted && root.screenCoordinatesCustomization
            ? root.currentItem : null;
        if (accepted && root.session.gameplayChatOpen === true) {
            root.session.setGameplayChatOpen(false);
        }
        root.session.setOverlayCustomizationActive(
                    accepted && root.session.arenaGameplayActive === true);
        root.customizeMode = accepted;
        if (previousScreen !== null && previousScreen !== nextScreen
                && typeof previousScreen.setArenaCustomizeMode === "function") {
            previousScreen.setArenaCustomizeMode(false);
        }
        if (nextScreen !== null
                && typeof nextScreen.setArenaCustomizeMode === "function") {
            nextScreen.setArenaCustomizeMode(accepted);
        }
        root.coordinatedScreen = nextScreen;
        root.customizationTransitionActive = false;
    }

    onOwnsArenaRunnerChanged: {
        if (!root.ownsArenaRunner) {
            root.expanded = false;
        }
    }
    onArenaShortcutEnabledChanged: {
        if (!root.arenaShortcutEnabled) {
            root.setCustomizeMode(false);
        }
    }
    onCurrentItemChanged: {
        if (root.customizeMode) {
            root.setCustomizeMode(false);
        }
    }

    Component.onDestruction: root.setCustomizeMode(false)

    Connections {
        target: root.session

        function onGameplayChatOpenChanged() {
            if (root.session.gameplayChatOpen === true && root.customizeMode) {
                root.setCustomizeMode(false);
            }
        }

        function onOverlayCustomizationActiveChanged() {
            if (!root.customizationTransitionActive && root.customizeMode
                    && root.ownsArenaRunner
                    && root.session.overlayCustomizationActive !== true) {
                root.setCustomizeMode(false);
            }
        }
    }

    Shortcut {
        autoRepeat: false
        context: Qt.ApplicationShortcut
        enabled: root.arenaShortcutEnabled
        sequence: "F2"

        onActivated: root.setCustomizeMode(!root.customizeMode)
    }

    Shortcut {
        autoRepeat: false
        context: Qt.ApplicationShortcut
        enabled: root.ownsArenaRunner
        sequence: "F8"

        onActivated: {
            if (root.customizeMode) {
                root.setCustomizeMode(false);
            }
            root.session.toggleGameplayChat();
        }
    }

    Shortcut {
        autoRepeat: false
        context: Qt.ApplicationShortcut
        enabled: root.customizeMode
        sequences: ["Return", "Enter", "Esc"]

        onActivated: root.setCustomizeMode(false)
    }

    MouseArea {
        objectName: "arenaLegacyCustomizationShield"
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        enabled: visible
        hoverEnabled: true
        preventStealing: true
        visible: root.customizeMode
            && !root.screenCoordinatesCustomization
        z: 0

        onWheel: wheel => wheel.accepted = true
    }

    Loader {
        id: gameplayOverlayLoader

        active: root.ownsArenaRunner
        height: Math.min(Math.max(0, root.height - 48),
                         root.expanded || root.session.gameplayChatOpen === true ? 640 : 360)
        sourceComponent: gameplayOverlayComponent
        width: Math.min(420, Math.max(0, root.width - 48))
        z: 1
        anchors {
            right: parent.right
            rightMargin: 24
            top: parent.top
            topMargin: 24
        }
    }

    Component {
        id: gameplayOverlayComponent

        ArenaGameplayOverlay {
            id: gameplayOverlay

            layoutVariant: root.layoutVariant
            placementKind: "gameplayLeaderboard"
            resolvedSkinId: root.resolvedSkinId
            session: root.session

            onExpandedChanged: root.expanded = gameplayOverlay.expanded
        }
    }
}

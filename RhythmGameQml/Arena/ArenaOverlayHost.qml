pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: root

    required property var session
    required property var currentItem
    required property string resolvedSkinId
    required property string layoutVariant
    property bool expanded: false

    readonly property bool ownsArenaRunner: root.currentItem !== null
        && root.currentItem.chart !== undefined
        && root.session.arenaGameplayActive === true
        && root.session.arenaRunner !== null
        && root.currentItem.chart === root.session.arenaRunner

    onOwnsArenaRunnerChanged: {
        if (!root.ownsArenaRunner) {
            root.expanded = false;
        }
    }

    Shortcut {
        autoRepeat: false
        context: Qt.ApplicationShortcut
        enabled: root.ownsArenaRunner
        sequence: "F8"

        onActivated: root.session.toggleGameplayChat()
    }

    Loader {
        id: gameplayOverlayLoader

        active: root.ownsArenaRunner
        height: Math.min(Math.max(0, root.height - 48),
                         root.expanded || root.session.gameplayChatOpen === true ? 640 : 360)
        sourceComponent: gameplayOverlayComponent
        width: Math.min(420, Math.max(0, root.width - 48))
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

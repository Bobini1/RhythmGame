pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: root

    required property var session
    required property var currentItem
    required property var themeVars
    required property string layoutVariant
    property string resultResolvedSkinId: ""
    property var resultThemeVars: null
    property bool expanded: false
    property bool resultExpanded: true

    readonly property bool ownsArenaRunner: root.currentItem !== null
        && root.currentItem.chart !== undefined
        && root.session.arenaGameplayActive === true
        && root.session.arenaRunner !== null
        && root.currentItem.chart === root.session.arenaRunner
    readonly property bool arenaNativeGameplayPresentation: root.currentItem
        !== null && root.currentItem.arenaNativeGameplayPresentation === true
    readonly property bool legacyArenaGameplay: root.ownsArenaRunner
        && !root.arenaNativeGameplayPresentation
    readonly property string currentArenaRoundId: root.currentItem
        && root.currentItem.arenaRoundId !== undefined
        ? String(root.currentItem.arenaRoundId || "") : ""
    readonly property bool ownsArenaResult: root.currentArenaRoundId.length > 0
        && root.session.resultPresentationActive === true
        && root.session.presentedResult !== null
        && root.session.presentedResult.valid === true
        && root.currentArenaRoundId
            === String(root.session.presentedResult.roundId || "")
    readonly property bool arenaNativeResultPresentation: root.currentItem
        !== null && root.currentItem.arenaNativeResultPresentation === true
    readonly property bool legacyArenaResult: root.ownsArenaResult
        && !root.arenaNativeResultPresentation
    readonly property bool arenaShortcutEnabled: root.ownsArenaRunner
        || root.ownsArenaResult
    readonly property string activeRoundId: root.ownsArenaRunner
        && root.session.liveStandings !== null
        && root.session.liveStandings !== undefined
        ? String(root.session.liveStandings.roundId || "")
        : (root.ownsArenaResult ? root.currentArenaRoundId : "")

    onOwnsArenaRunnerChanged: {
        if (!root.ownsArenaRunner) {
            root.expanded = false;
        }
    }
    onActiveRoundIdChanged: {
        if (root.session.chatOpen === true) {
            root.session.setChatOpen(false);
        }
    }
    onLegacyArenaResultChanged: {
        if (!root.legacyArenaResult) {
            root.resultExpanded = true;
        }
    }

    Shortcut {
        autoRepeat: false
        context: Qt.ApplicationShortcut
        enabled: root.arenaShortcutEnabled
        sequence: "F8"

        onActivated: root.session.toggleChat()
    }

    Loader {
        id: gameplayOverlayLoader

        active: root.legacyArenaGameplay
        sourceComponent: gameplayOverlayComponent
        z: 1
    }

    Component {
        id: gameplayOverlayComponent

        ArenaOverlayPlacementFrame {
            id: placementFrame

            objectName: "arenaGameplayPlacementFrame"
            layoutVariant: root.layoutVariant
            placementKind: "gameplayLeaderboard"
            themeVars: root.themeVars
            viewport: root
            minimumPixelSize: Qt.size(320, 240)
            moveHandle: gameplayOverlay.dragHandle

            ArenaGameplayOverlay {
                id: gameplayOverlay

                anchors.fill: parent
                session: root.session
                expanded: root.expanded

                onExpandedChanged: root.expanded = gameplayOverlay.expanded
            }
        }
    }

    Loader {
        id: resultOverlayLoader

        active: root.legacyArenaResult
        sourceComponent: resultOverlayComponent
    }

    Component {
        id: resultOverlayComponent

        ArenaOverlayPlacementFrame {
            id: resultPlacementFrame

            objectName: "arenaResultPlacementFrame"
            themeVars: root.resultThemeVars
            viewport: root
            placementKind: "resultStandings"
            layoutVariant: "result"
            moveHandle: resultOverlay.dragHandle

            ArenaResultOverlay {
                id: resultOverlay

                anchors.fill: parent
                layoutVariant: "result"
                placementKind: "resultStandings"
                resolvedSkinId: root.resultResolvedSkinId
                session: root.session
                expanded: root.resultExpanded

                onExpandedChanged: root.resultExpanded = expanded
            }
        }
    }
}

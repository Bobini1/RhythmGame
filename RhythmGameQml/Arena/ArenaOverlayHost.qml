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
    readonly property bool customizeMode: root.currentItem !== null
        && root.currentItem.customizeMode === true

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

    function gameplayPropertyPrefix() {
        switch (root.layoutVariant) {
        case "k5": return "arenaOverlayK5";
        case "k7": return "arenaOverlayK7";
        case "k10": return "arenaOverlayK10";
        case "k14": return "arenaOverlayK14";
        default: return "";
        }
    }

    function rememberChatSelection(chatSelected) {
        if (root.currentItem
                && typeof root.currentItem.rememberArenaChatSelection
                    === "function") {
            root.currentItem.rememberArenaChatSelection(chatSelected);
            return;
        }
        const vars = root.ownsArenaRunner
            ? root.themeVars : root.resultThemeVars;
        const prefix = root.ownsArenaRunner
            ? root.gameplayPropertyPrefix() : "arenaOverlayResult";
        if (vars && prefix.length > 0) {
            vars[prefix + "ChatSelected"] = !!chatSelected;
        }
    }

    Shortcut {
        autoRepeat: false
        context: Qt.ApplicationShortcut
        enabled: root.arenaShortcutEnabled
        sequence: "F8"

        onActivated: {
            root.session.toggleChat();
            root.rememberChatSelection(root.session.chatOpen === true);
        }
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
            customizationLabel: qsTr("Arena gameplay panel")
            customizeMode: root.customizeMode
            defaultExpanded: false
            layoutVariant: root.layoutVariant
            placementKind: "gameplayLeaderboard"
            themeVars: root.themeVars
            viewport: root
            minimumPixelSize: Qt.size(320, 240)
            moveHandle: gameplayOverlay.dragHandle
            visible: placementFrame.overlayVisible || root.customizeMode

            onOverlayVisibilityCommitted: visible => {
                if (!visible && root.session.chatOpen === true) {
                    root.session.setChatOpen(false);
                }
            }
            onPresentationStateReloaded: {
                if (placementFrame.overlayVisible) {
                    placementFrame.restoreChatSelection(root.session);
                }
            }

            ArenaGameplayOverlay {
                id: gameplayOverlay

                anchors.fill: parent
                session: root.session
                expanded: placementFrame.expanded

                onChatSelected: chat => placementFrame.setChatSelected(chat)
                onExpandedChanged: {
                    placementFrame.setExpanded(gameplayOverlay.expanded);
                }
            }

            Component.onCompleted: {
                if (placementFrame.overlayVisible) {
                    placementFrame.restoreChatSelection(root.session);
                }
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
            customizationLabel: qsTr("Arena result panel")
            customizeMode: root.customizeMode
            defaultExpanded: true
            themeVars: root.resultThemeVars
            viewport: root
            placementKind: "resultStandings"
            layoutVariant: "result"
            moveHandle: resultOverlay.dragHandle
            visible: resultPlacementFrame.overlayVisible || root.customizeMode

            onOverlayVisibilityCommitted: visible => {
                if (!visible && root.session.chatOpen === true) {
                    root.session.setChatOpen(false);
                }
            }
            onPresentationStateReloaded: {
                if (resultPlacementFrame.overlayVisible) {
                    resultPlacementFrame.restoreChatSelection(root.session);
                }
            }

            ArenaResultOverlay {
                id: resultOverlay

                anchors.fill: parent
                layoutVariant: "result"
                placementKind: "resultStandings"
                resolvedSkinId: root.resultResolvedSkinId
                session: root.session
                expanded: resultPlacementFrame.expanded

                onChatSelected: chat => {
                    resultPlacementFrame.setChatSelected(chat);
                }
                onExpandedChanged: {
                    resultPlacementFrame.setExpanded(resultOverlay.expanded);
                }
            }

            Component.onCompleted: {
                if (resultPlacementFrame.overlayVisible) {
                    resultPlacementFrame.restoreChatSelection(root.session);
                }
            }
        }
    }

    TransientInputFocusDismissLayer {}
}

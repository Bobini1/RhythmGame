pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: root

    required property var session
    required property var currentItem
    required property var themeVars
    required property var generalVars
    required property string layoutVariant
    property bool expanded: false
    property bool customizeMode: false
    property var coordinatedScreen: null
    property bool customizationTransitionActive: false
    property int unreadCount: 0

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
    readonly property string activeRoundId: root.ownsArenaRunner
        && root.session.liveStandings !== null
        && root.session.liveStandings !== undefined
        ? String(root.session.liveStandings.roundId || "") : ""
    readonly property var placementFrame: gameplayOverlayLoader.item
    readonly property rect chatDrawerRect: root.placementFrame !== null
        ? root.placementFrame.adjacentChatRect()
        : Qt.rect(0, 0, 1, 1)

    function acknowledgePlacementHint() {
        if (root.generalVars !== null && root.generalVars !== undefined
                && root.generalVars.arenaOverlayHintVersion < 1) {
            root.generalVars.arenaOverlayHintVersion = 1;
        }
        placementHintTimer.stop();
    }

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
        if (accepted) {
            root.acknowledgePlacementHint();
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
            root.unreadCount = 0;
            placementHintTimer.stop();
        } else if (root.generalVars !== null
                   && root.generalVars !== undefined
                   && root.generalVars.arenaOverlayHintVersion < 1) {
            placementHintTimer.restart();
        }
    }
    onActiveRoundIdChanged: {
        root.unreadCount = 0;
        if (root.session.gameplayChatOpen === true) {
            root.session.setGameplayChatOpen(false);
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

    Component.onCompleted: {
        if (root.ownsArenaRunner && root.generalVars !== null
                && root.generalVars !== undefined
                && root.generalVars.arenaOverlayHintVersion < 1) {
            placementHintTimer.start();
        }
    }
    Component.onDestruction: root.setCustomizeMode(false)

    Connections {
        target: root.session

        function onGameplayChatOpenChanged() {
            if (root.session.gameplayChatOpen === true) {
                root.unreadCount = 0;
            }
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

    Connections {
        target: root.session.chat
        enabled: root.ownsArenaRunner && root.session.chat !== null

        function onRowsInserted(parent, first, last) {
            if (root.session.gameplayChatOpen !== true
                    && root.activeRoundId.length > 0) {
                root.unreadCount += last - first + 1;
            }
        }

        function onModelReset() {
            root.unreadCount = 0;
        }
    }

    Timer {
        id: placementHintTimer

        interval: 6000
        repeat: false
        onTriggered: root.acknowledgePlacementHint()
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
            customizeMode: root.customizeMode
            minimumPixelSize: Qt.size(320, 240)

            onRequestExitCustomization: root.setCustomizeMode(false)

            ArenaGameplayOverlay {
                id: gameplayOverlay

                anchors.fill: parent
                session: root.session
                expanded: root.expanded
                unreadCount: root.unreadCount

                onExpandedChanged: root.expanded = gameplayOverlay.expanded
            }
        }
    }

    Loader {
        id: gameplayChatDrawer

        objectName: "arenaGameplayChatDrawer"
        active: root.ownsArenaRunner
        focus: visible
        visible: root.session.gameplayChatOpen === true
        x: root.chatDrawerRect.x
        y: root.chatDrawerRect.y
        width: root.chatDrawerRect.width
        height: root.chatDrawerRect.height
        sourceComponent: gameplayChatComponent
        z: 2

        onVisibleChanged: {
            if (visible) {
                gameplayChatDrawer.forceActiveFocus();
            }
        }
        onLoaded: {
            if (visible) {
                gameplayChatDrawer.forceActiveFocus();
            }
        }
    }

    Component {
        id: gameplayChatComponent

        ArenaGameplayChat {
            session: root.session
        }
    }

    Rectangle {
        id: placementHint

        objectName: "arenaOverlayPlacementHint"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 24
        border.color: "#70ffffff"
        border.width: 1
        color: "#e6101218"
        height: placementHintText.implicitHeight + 20
        radius: 5
        visible: root.ownsArenaRunner
            && !root.customizeMode
            && root.generalVars !== null
            && root.generalVars !== undefined
            && root.generalVars.arenaOverlayHintVersion < 1
            && placementHintTimer.running
        width: Math.min(parent.width - 48,
                        placementHintText.implicitWidth + 32)
        z: 3

        Text {
            id: placementHintText

            anchors.centerIn: parent
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Press F2 to move Arena standings")
            textFormat: Text.PlainText
            width: Math.max(1, parent.width - 24)
            wrapMode: Text.Wrap
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.acknowledgePlacementHint()
        }
    }
}

pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: root

    required property var session
    required property var currentItem
    required property var themeVars
    required property var generalVars
    required property string layoutVariant
    property string resultResolvedSkinId: ""
    property var resultThemeVars: null
    property bool expanded: false
    property bool customizeMode: false
    property var coordinatedScreen: null
    property bool customizationTransitionActive: false
    property bool resultExpanded: true
    property bool resultCustomizeMode: false
    property bool resultInputGuardActive: false

    function updateResultInputGuardTarget() {
        const item = root.currentItem;
        const method = "setArenaResultCustomizationActive";
        if (item && typeof item[method] === "function") {
            item[method](root.resultInputGuardActive);
        }
    }

    readonly property bool ownsArenaRunner: root.currentItem !== null && root.currentItem.chart !== undefined && root.session.arenaGameplayActive === true && root.session.arenaRunner !== null && root.currentItem.chart === root.session.arenaRunner
    readonly property string currentArenaRoundId: root.currentItem && root.currentItem.arenaRoundId !== undefined ? String(root.currentItem.arenaRoundId || "") : ""
    readonly property bool ownsArenaResult: root.currentArenaRoundId.length > 0 && root.session.resultPresentationActive === true && root.session.presentedResult !== null && root.session.presentedResult.valid === true && root.currentArenaRoundId === String(root.session.presentedResult.roundId || "")
    readonly property bool arenaNativeResultPresentation: root.currentItem !== null && root.currentItem.arenaNativeResultPresentation === true
    readonly property bool legacyArenaResult: root.ownsArenaResult && !root.arenaNativeResultPresentation
    readonly property bool arenaShortcutEnabled: root.ownsArenaRunner || root.ownsArenaResult
    readonly property bool coordinatedShortcutEnabled: root.ownsArenaRunner || (root.ownsArenaResult && root.arenaNativeResultPresentation)
    readonly property bool screenCoordinatesCustomization: root.currentItem !== null && typeof root.currentItem.setArenaCustomizeMode === "function"
    readonly property string activeRoundId: root.ownsArenaRunner && root.session.liveStandings !== null && root.session.liveStandings !== undefined ? String(root.session.liveStandings.roundId || "") : (root.ownsArenaResult ? root.currentArenaRoundId : "")
    readonly property var placementFrame: gameplayOverlayLoader.item || resultOverlayLoader.item || (root.currentItem && root.currentItem.arenaOverlayPlacementFrame !== undefined ? root.currentItem.arenaOverlayPlacementFrame : null)
    function acknowledgePlacementHint() {
        if (placementHint.activeFocus) {
            if (root.currentItem !== null && typeof root.currentItem.forceActiveFocus === "function") {
                root.currentItem.forceActiveFocus();
            } else {
                root.forceActiveFocus();
            }
        }
        if (root.generalVars !== null && root.generalVars !== undefined && root.generalVars.arenaOverlayHintVersion < 1) {
            root.generalVars.arenaOverlayHintVersion = 1;
        }
        placementHintTimer.stop();
    }

    function setCustomizeMode(active) {
        const accepted = !!active && root.coordinatedShortcutEnabled;
        if (root.customizationTransitionActive || (root.customizeMode === accepted && (!accepted || root.coordinatedScreen === root.currentItem))) {
            return;
        }
        root.customizationTransitionActive = true;
        const previousScreen = root.coordinatedScreen;
        const nextScreen = accepted && root.screenCoordinatesCustomization ? root.currentItem : null;
        if (accepted && root.session.chatOpen === true) {
            root.session.setChatOpen(false);
        }
        if (accepted) {
            root.acknowledgePlacementHint();
        }
        root.session.setOverlayCustomizationActive(accepted && root.session.arenaGameplayActive === true);
        root.customizeMode = accepted;
        if (previousScreen !== null && previousScreen !== nextScreen && typeof previousScreen.setArenaCustomizeMode === "function") {
            previousScreen.setArenaCustomizeMode(false);
        }
        if (nextScreen !== null && typeof nextScreen.setArenaCustomizeMode === "function") {
            nextScreen.setArenaCustomizeMode(accepted);
        }
        root.coordinatedScreen = nextScreen;
        root.customizationTransitionActive = false;
    }

    onOwnsArenaRunnerChanged: {
        if (!root.ownsArenaRunner) {
            root.expanded = false;
            placementHintTimer.stop();
        } else if (root.generalVars !== null && root.generalVars !== undefined && root.generalVars.arenaOverlayHintVersion < 1) {
            placementHintTimer.restart();
        }
    }
    onActiveRoundIdChanged: {
        if (root.session.chatOpen === true) {
            root.session.setChatOpen(false);
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
        root.updateResultInputGuardTarget();
    }

    Component.onCompleted: {
        if (root.ownsArenaRunner && root.generalVars !== null && root.generalVars !== undefined && root.generalVars.arenaOverlayHintVersion < 1) {
            placementHintTimer.start();
        }
    }
    Component.onDestruction: {
        root.setCustomizeMode(false);
        root.resultInputGuardActive = false;
        root.updateResultInputGuardTarget();
    }

    Connections {
        target: root.session

        function onChatOpenChanged() {
            if (root.session.chatOpen === true && root.customizeMode) {
                root.setCustomizeMode(false);
            }
        }

        function onOverlayCustomizationActiveChanged() {
            if (!root.customizationTransitionActive && root.customizeMode && root.ownsArenaRunner && root.session.overlayCustomizationActive !== true) {
                root.setCustomizeMode(false);
            }
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
        enabled: root.coordinatedShortcutEnabled
        sequence: "F2"

        onActivated: root.setCustomizeMode(!root.customizeMode)
    }

    onLegacyArenaResultChanged: {
        if (!root.legacyArenaResult) {
            root.resultExpanded = true;
            root.resultCustomizeMode = false;
            root.resultInputGuardActive = false;
        } else {
            root.updateResultInputGuardTarget();
        }
    }

    onResultCustomizeModeChanged: {
        if (root.resultCustomizeMode) {
            root.resultInputGuardActive = true;
            return;
        }
        Qt.callLater(function () {
            if (!root.resultCustomizeMode) {
                root.resultInputGuardActive = false;
            }
        });
    }

    onResultInputGuardActiveChanged: root.updateResultInputGuardTarget()

    Shortcut {
        autoRepeat: false
        context: Qt.ApplicationShortcut
        enabled: root.arenaShortcutEnabled
        sequence: "F8"

        onActivated: {
            if (root.customizeMode) {
                root.setCustomizeMode(false);
            }
            root.session.toggleChat();
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
        visible: root.customizeMode && !root.screenCoordinatesCustomization
        z: 0

        onWheel: wheel => wheel.accepted = true
    }

    Shortcut {
        autoRepeat: false
        context: Qt.ApplicationShortcut
        enabled: root.legacyArenaResult
        sequence: "F2"

        onActivated: root.resultCustomizeMode = !root.resultCustomizeMode
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
            directMoveEnabled: true
            directResizeEnabled: true
            minimumPixelSize: Qt.size(320, 240)
            moveHandle: gameplayOverlay.dragHandle

            onRequestExitCustomization: root.setCustomizeMode(false)

            ArenaGameplayOverlay {
                id: gameplayOverlay

                anchors.fill: parent
                session: root.session
                expanded: root.expanded

                onExpandedChanged: root.expanded = gameplayOverlay.expanded
            }
        }
    }

    Rectangle {
        id: placementHint

        objectName: "arenaOverlayPlacementHint"
        activeFocusOnTab: visible
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 24
        border.color: activeFocus ? "#8fdcff" : "#70ffffff"
        border.width: activeFocus ? 2 : 1
        color: "#e6101218"
        height: placementHintText.implicitHeight + 20
        radius: 5
        visible: root.ownsArenaRunner && !root.customizeMode && root.generalVars !== null && root.generalVars !== undefined && root.generalVars.arenaOverlayHintVersion < 1 && placementHintTimer.running
        width: Math.min(parent.width - 48, placementHintText.implicitWidth + 32)
        z: 3

        Accessible.role: Accessible.Button
        Accessible.name: placementHintText.text
        Accessible.description: qsTr("Press F2 to customize the Arena standings. Press Enter or Space to dismiss this hint.")
        Accessible.focusable: visible
        Accessible.onPressAction: root.acknowledgePlacementHint()

        Keys.priority: Keys.BeforeItem
        Keys.onPressed: event => {
            if (event.key !== Qt.Key_Return && event.key !== Qt.Key_Enter && event.key !== Qt.Key_Space) {
                return;
            }
            root.acknowledgePlacementHint();
            event.accepted = true;
        }

        Text {
            id: placementHintText

            anchors.centerIn: parent
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Press F2 to move Arena standings")
            textFormat: Text.PlainText
            width: Math.max(1, parent.width - 24)
            wrapMode: Text.Wrap

            Accessible.ignored: true
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.acknowledgePlacementHint()
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
            customizeMode: root.resultCustomizeMode
            directMoveEnabled: true
            directResizeEnabled: true
            moveHandle: resultOverlay.dragHandle

            onRequestExitCustomization: root.resultCustomizeMode = false

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

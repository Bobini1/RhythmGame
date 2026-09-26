import QtQuick
import QtQml
import RhythmGameQml
import QtQuick.Controls
import QtCore

ApplicationWindow {
    id: contentContainer

    height: 720
    width: 1280
    visibility: Window.Windowed

    Settings {
        id: settings
        property alias height: contentContainer.height
        property alias width: contentContainer.width
        property int visibility
    }
    property bool settingsRestored: false
    Component.onCompleted: {
        if (settings.visibility) {
            contentContainer.visibility = settings.visibility;
        }
    }
    onClosing: function(close) {
        if (Rg.arenaSession.active) {
            Rg.arenaSession.exitArena();
        }
    }
    onActiveChanged: {
        if (active && !settingsRestored) {
            settingsRestored = true;
        }
    }
    onVisibilityChanged: {
        if (settingsRestored && (contentContainer.visibility === Window.Windowed || contentContainer.visibility === Window.FullScreen || contentContainer.visibility === Window.Maximized)) {
            settings.visibility = contentContainer.visibility;
        }
    }
    Shortcut {
        autoRepeat: false
        sequence: "F1"
        onActivated: globalRoot.toggleFpsOverlay()
    }

    Shortcut {
        autoRepeat: false
        sequence: "F4"
        onActivated: globalRoot.toggleFullScreen()
    }

    Item {
        id: globalRoot

        QtObject {
            id: frameState

            readonly property Profile mainProfile: Rg.profileList.mainProfile
            readonly property Component mainComponent:
                frameImplementation.componentFor("main")
            readonly property Component resultComponent:
                frameImplementation.componentFor("result")
            readonly property Component courseResultComponent:
                frameImplementation.componentFor("courseResult")
            readonly property Component multiplayerComponent:
                frameImplementation.componentFor("multiplayer")
            readonly property Component settingsComponent:
                frameImplementation.componentFor("settings")
            readonly property Component selectComponent:
                frameImplementation.componentFor("select")
            readonly property Component decideComponent:
                frameImplementation.componentFor("decide")
            property var activeSettingsItem: null
            property bool fpsOverlayVisible: false
        }

        QtObject {
            id: frameImplementation

            function componentFor(screenKey) {
                const themeName = frameState.mainProfile.themeConfig[screenKey];
                return Qt.createComponent(Rg.themes.availableThemeFamilies[themeName].screens[screenKey].script);
            }

            function createScreen(component, properties) {
                if (!component || sceneStack.busy) {
                    return null;
                }
                // Create before pushing so creation failures leave the stack unchanged.
                const item = component.createObject(sceneStack,
                    Object.assign({}, properties, { "enabled": false, "visible": false }));
                // An incompatible QObject can leave a typed property null even
                // when Qt creates the root. Reject it before changing screens.
                if (item && properties
                        && ((properties.gameplay && item.gameplay !== properties.gameplay)
                            || (properties.result && item.result !== properties.result))) {
                    console.error("Screen did not accept its injected context");
                    item.destroy();
                    return null;
                }
                if (item) {
                    // StackView does not destroy items created outside the stack.
                    item.StackView.removed.connect(() => item.destroy());
                }
                return item;
            }

            function pushScreen(item) {
                if (!item) {
                    return null;
                }
                // Destroy the prepared screen if the stack rejects the push.
                if (sceneStack.busy || sceneStack.pushItem(item) !== item) {
                    item.destroy();
                    return null;
                }
                sceneStack.updateEnabledStates();
                return item;
            }

            function createGameplayScreen(gameplay) {
                if (!gameplay) {
                    return null;
                }
                const battle = gameplay.players.length === 2;
                const screenKey = "k" + gameplay.keymode + (battle ? "battle" : "");
                return frameImplementation.createScreen(
                    frameImplementation.componentFor(screenKey), { "gameplay": gameplay });
            }

            function openDecide(runner) {
                const gameplay = ScreenContexts.createGameplay(runner);
                const item = frameImplementation.createScreen(
                    frameState.decideComponent, { "gameplay": gameplay });
                if (!item) {
                    runner.destroy();
                    return null;
                }
                item.QmlUtils.adopt(runner);
                gameplay._screen = item;
                return frameImplementation.pushScreen(item);
            }
        }

        function isFullScreen(): var {
            return contentContainer.visibility === Window.FullScreen;
        }

        function setFullScreen(enabled: var): void {
            contentContainer.visibility = enabled ? Window.FullScreen : Window.Windowed;
        }

        function toggleFullScreen(): void {
            setFullScreen(!isFullScreen());
        }

        function toggleFpsOverlay(): void {
            frameState.fpsOverlayVisible = !frameState.fpsOverlayVisible;
        }

        function localFileUrl(path: var): var {
            let value = String(path || "").trim();
            if (value.length === 0) {
                return "";
            }
            if (/^file:\/\//i.test(value) || /^[A-Za-z][A-Za-z0-9+.-]*:\/\//.test(value)) {
                return value;
            }
            return Rg.fileQuery.localFileUrl(value);
        }

        function openLocalFolder(path: var): var {
            let localPath = Rg.songAssets
                ? Rg.songAssets.containingFolder(path)
                : path;
            return Rg.fileQuery.openFolder(localPath);
        }

        /*! The currently presented screen. */
        readonly property Item currentScreen: sceneStack.currentItem

        function returnToPreviousScreen(): var {
            return sceneStack.pop();
        }

        function quitApplication(): void {
            Qt.quit();
        }

        function openSettings(section = ""): var {
            let item = frameState.activeSettingsItem === sceneStack.currentItem
                ? frameState.activeSettingsItem : null;
            if (!item) {
                item = frameImplementation.pushScreen(
                    frameImplementation.createScreen(frameState.settingsComponent,
                                                     { "initialSection": section }));
                frameState.activeSettingsItem = item;
            }
            if (item && section) {
                item.initialSection = section;
            }
            return item;
        }

        function openArenaBrowser(): var {
            return frameImplementation.pushScreen(
                frameImplementation.createScreen(frameState.multiplayerComponent, {}));
        }

        function openSelect(): var {
            return frameImplementation.pushScreen(frameImplementation.createScreen(
                frameState.selectComponent, {}));
        }

        function openChart(path: var, profile1: var, autoplay1: var, replay1: var, score1: var, profile2: var, autoplay2: var, replay2: var, score2: var): var {
            if (sceneStack.busy) {
                return null;
            }
            let chart = Rg.chartLoader.loadChart(path, profile1, autoplay1, replay1, score1, profile2, autoplay2, replay2, score2);
            if (!chart) {
                console.error("Failed to load chart");
                return;
            }
            return frameImplementation.openDecide(chart);
        }

        function openCourse(course: var, profile1: var, autoplay1: var, replay1: var, score1: var, profile2: var, autoplay2: var, replay2: var, score2: var): var {
            if (sceneStack.busy) {
                return null;
            }
            let runner = Rg.chartLoader.loadCourse(course, profile1, autoplay1, replay1, score1, profile2, autoplay2, replay2, score2);
            if (!runner) {
                console.error("Failed to load course");
                return;
            }
            return frameImplementation.openDecide(runner);
        }

        function openGameplay(gameplay: GameplayContext): var {
            if (!gameplay) {
                return null;
            }
            const item = frameImplementation.pushScreen(
                frameImplementation.createGameplayScreen(gameplay));
            if (item) {
                if (!gameplay.isArena) item.QmlUtils.adopt(gameplay._runner);
                gameplay._screen = item;
            }
            return item;
        }

        /*! Replaces screen and any screens above it with local gameplay. */
        function replaceGameplay(gameplay: GameplayContext, screen = currentScreen): var {
            if (!gameplay || gameplay.isArena || !screen || screen.StackView.view !== sceneStack
                    || sceneStack.busy) {
                return null;
            }
            // Create first: an invalid skin must leave the existing screen intact.
            const item = frameImplementation.createGameplayScreen(gameplay);
            if (!item) {
                return null;
            }
            // Removed screens must not resume input or pending completion.
            for (let i = screen.StackView.index; i < sceneStack.depth; ++i) {
                const item = sceneStack.get(i, StackView.DontLoad);
                if (item) {
                    item.enabled = false;
                }
            }
            if (sceneStack.replace(screen, item, StackView.Immediate) !== item) {
                item.destroy();
                sceneStack.updateEnabledStates();
                return null;
            }
            item.QmlUtils.adopt(gameplay._runner);
            gameplay._screen = item;
            sceneStack.updateEnabledStates();
            return item;
        }

        function openResult(scores: var, profiles: var, chartData: var, gameplay = null, arenaRoundId = ""): var {
            const context = ScreenContexts.createResult(scores, profiles, chartData, gameplay, arenaRoundId);
            if (!context) {
                return null;
            }
            const item = frameImplementation.pushScreen(
                frameImplementation.createScreen(frameState.resultComponent, { "result": context }));
            if (item) item.QmlUtils.adopt(context);
            else context.destroy();
            return item;
        }

        function openCourseResult(scores: var, profiles: var, chartDatas: var, course: var): var {
            const context = ScreenContexts.createCourseResult(scores, profiles, chartDatas, course);
            if (!context) {
                return null;
            }
            const item = frameImplementation.pushScreen(
                frameImplementation.createScreen(frameState.courseResultComponent, { "result": context }));
            if (item) item.QmlUtils.adopt(context);
            else context.destroy();
            return item;
        }

        anchors.fill: parent

        StackView {
            id: sceneStack

            onCurrentItemChanged: {
                Qt.callLater(updateEnabledStates);
            }

            onDepthChanged: {
                Qt.callLater(updateEnabledStates);
            }

            Component.onCompleted: {
                Qt.callLater(updateEnabledStates);
            }

            function updateEnabledStates(): void {
                let topIndex = depth - 1;
                for (let i = 0; i < depth; ++i) {
                    let item = get(i, StackView.ForceLoad);
                    if (item) {
                        let active = i === topIndex;
                        item.enabled = active;
                        item.visible = active;
                    }
                }
            }

            anchors.fill: parent
            initialItem: frameState.mainComponent

            popEnter: null
            popExit: null
            pushEnter: null
            pushExit: null
            replaceEnter: null
            replaceExit: null
        }
        Binding {
            target: Rg.programSettings
            property: "continuousRendering"
            value: sceneStack.currentItem?.gameplay?.status === ChartRunner.Running
        }
        Loader {
            id: debugLogLoader

            active: false
            anchors.fill: parent
            asynchronous: true
            source: "Log.qml"
        }
        Shortcut {
            autoRepeat: false
            sequence: "F10"

            onActivated: {
                debugLogLoader.active = !debugLogLoader.active;
            }
        }
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: 8
            border.color: "#80ffffff"
            border.width: 1
            color: "#c0000000"
            height: fpsText.implicitHeight + 10
            radius: 2
            visible: frameState.fpsOverlayVisible
            width: fpsText.implicitWidth + 14
            z: 1000000

            Text {
                id: fpsText
                anchors.centerIn: parent
                color: "white"
                font.bold: true
                font.pixelSize: 18
                text: (Rg.programSettings.presentationFps > 0
                    ? Rg.programSettings.presentationFps
                    : "--") + " FPS"
            }
        }
    }
}

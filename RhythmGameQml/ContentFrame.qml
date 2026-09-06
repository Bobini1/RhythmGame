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
            readonly property var arenaSession: Rg.arenaSession
            readonly property Component k7Component:
                frameImplementation.componentFor("k7")
            readonly property Component k7battleComponent:
                frameImplementation.componentFor("k7battle")
            readonly property Component k14Component:
                frameImplementation.componentFor("k14")
            readonly property Component k5Component:
                frameImplementation.componentFor("k5")
            readonly property Component k5battleComponent:
                frameImplementation.componentFor("k5battle")
            readonly property Component k10Component:
                frameImplementation.componentFor("k10")
            readonly property Component mainComponent:
                frameImplementation.componentFor("main")
            readonly property Component resultComponent:
                frameImplementation.componentFor("result")
            readonly property Component courseResultComponent:
                frameImplementation.componentFor("courseResult", "result")
            readonly property var multiplayerScreen:
                frameImplementation.configuredScreen("multiplayer")
            readonly property Component settingsComponent:
                frameImplementation.componentFor("settings")
            readonly property Component selectComponent:
                frameImplementation.componentFor("select")
            readonly property Component decideComponent:
                frameImplementation.componentFor("decide")
            property var activeSettingsItem: null
            property Item activeArenaItem: null
            property Item activeArenaGameplayItem: null
            property var activeArenaGameplayRunner: null
            property bool fpsOverlayVisible: false
        }

        QtObject {
            id: frameImplementation

            function configuredScreen(screenKey, fallbackKey) {
                let themeName = frameState.mainProfile.themeConfig[screenKey];
                let family = themeName
                    ? Rg.themes.availableThemeFamilies[themeName] : null;
                if (family && family.screens && family.screens[screenKey]) {
                    return family.screens[screenKey];
                }
                if (fallbackKey) {
                    themeName = frameState.mainProfile.themeConfig[fallbackKey];
                    family = themeName
                        ? Rg.themes.availableThemeFamilies[themeName] : null;
                    if (family && family.screens
                            && family.screens[fallbackKey]) {
                        return family.screens[fallbackKey];
                    }
                }
                return null;
            }

            function componentFor(screenKey, fallbackKey) {
                const screen = frameImplementation.configuredScreen(
                    screenKey, fallbackKey);
                return screen && screen.script
                    ? Qt.createComponent(screen.script) : null;
            }

            function normalizeLocalPath(path) {
                let value = String(path || "").trim();
                if (value.length === 0) {
                    return "";
                }
                if (/^file:\/\//i.test(value)) {
                    let url = value;
                    if (/^file:\/\/\//i.test(url)) {
                        value = url.slice(8);
                    } else {
                        value = url.slice(7);
                    }
                    value = decodeURIComponent(value);
                }
                value = value.replace(/\\/g, "/");
                while (value.length > 3 && value.endsWith("/")) {
                    value = value.slice(0, -1);
                }
                return value;
            }

            function rootSongFolderForPath(path) {
                let target = frameImplementation.normalizeLocalPath(path);
                if (target.length === 0 || !Rg.rootSongFoldersConfig
                        || !Rg.rootSongFoldersConfig.folders) {
                    return null;
                }
                let targetLower = target.toLowerCase();
                let folders = Rg.rootSongFoldersConfig.folders;
                let best = null;
                let bestLength = -1;
                for (let i = 0; i < folders.rowCount(); ++i) {
                    let folder = folders.at(i);
                    let folderPath = frameImplementation.normalizeLocalPath(
                        folder ? folder.name : "");
                    if (folderPath.length === 0) {
                        continue;
                    }
                    let folderLower = folderPath.toLowerCase();
                    let matches = targetLower === folderLower
                        || targetLower.startsWith(folderLower + "/");
                    if (matches && folderLower.length > bestLength) {
                        best = folder;
                        bestLength = folderLower.length;
                    }
                }
                return best;
            }

            function currentScreen() {
                return sceneStack.currentItem || null;
            }

            function gameplayLayoutVariant(screenItem) {
                if (!screenItem || !screenItem.chart) {
                    return "";
                }
                const declared = String(screenItem.screen
                                        || screenItem.screenKey || "");
                const supported = ["k5", "k7", "k10", "k14"];
                if (supported.indexOf(declared) >= 0) {
                    return declared;
                }
                switch (Number(screenItem.chart.keymode)) {
                case 5: return "k5";
                case 7: return "k7";
                case 10: return "k10";
                case 14: return "k14";
                default: return "";
                }
            }

            function gameplayThemeVars(layoutVariant) {
                if (layoutVariant.length === 0) {
                    return null;
                }
                const themeName = frameState.mainProfile.themeConfig[
                    layoutVariant];
                const screenVars = frameState.mainProfile.vars.themeVars[
                    layoutVariant];
                return screenVars && screenVars[themeName]
                    ? screenVars[themeName] : null;
            }

            function callCurrentScreen(method, args) {
                let screen = frameImplementation.currentScreen();
                if (screen && typeof screen[method] === "function") {
                    return screen[method].apply(screen, args || []);
                }
                return false;
            }

            function currentLr2Settings(screenKey) {
                let themeName = frameState.mainProfile.themeConfig[screenKey];
                let screenVars = frameState.mainProfile.vars.themeVars[screenKey];
                if (screenVars && screenVars[themeName]) {
                    let source = screenVars[themeName];
                    let result = {};
                    let keys = source.keys ? source.keys() : Object.keys(source);
                    for (let key of keys) {
                        result[key] = source[key];
                    }
                    return result;
                }
                return undefined;
            }

            function resolvedThemeVars(screenKey) {
                const themeName = frameState.mainProfile.themeConfig[screenKey];
                const screenVars = frameState.mainProfile.vars.themeVars[
                    screenKey];
                return screenVars && screenVars[themeName]
                    ? screenVars[themeName] : null;
            }

            function selectScreenProperties() {
                let selectScreen =
                    frameImplementation.configuredScreen("select");
                let props = {};
                if (selectScreen && selectScreen.csvPath) {
                    props["csvPath"] = selectScreen.csvPath;
                    props["skinSettings"] =
                        frameImplementation.currentLr2Settings("select");
                    props["skinSettingsData"] =
                        selectScreen.settingsData || "";
                    props["screenKey"] = "select";
                }
                return props;
            }

            function createScreen(component, properties) {
                if (!component) {
                    return null;
                }
                const item = component.createObject(sceneStack,
                    Object.assign({}, properties, { "enabled": false, "visible": false }));
                if (item) {
                    // StackView does not destroy items created outside the stack.
                    item.StackView.removed.connect(() => item.destroy());
                }
                return item;
            }

            function gameplayDescriptor(runner, arenaManagedRunner) {
                let keys = runner.keymode;
                let battle = runner.player1 && runner.player2;
                let screenKey = "k" + keys + (battle ? "battle" : "");
                let component = frameState[screenKey + "Component"];
                let screenObj =
                    frameImplementation.configuredScreen(screenKey);
                let props = {
                    "chart": runner,
                    "arenaManagedRunner": arenaManagedRunner === true
                };
                if (screenObj && screenObj.csvPath) {
                    props["csvPath"] = screenObj.csvPath;
                    props["skinSettings"] =
                        frameImplementation.currentLr2Settings(screenKey);
                    props["skinSettingsData"] = screenObj.settingsData || "";
                    props["screenKey"] = screenKey;
                }
                return {
                    "component": component,
                    "properties": props
                };
            }

            function openPreparedArenaGameplay(runner) {
                if (!runner) {
                    frameImplementation.closePreparedArenaGameplay();
                    return;
                }
                if (frameState.activeArenaGameplayItem
                        && frameState.activeArenaGameplayItem.StackView.view
                           === sceneStack) {
                    return;
                }
                frameState.activeArenaGameplayRunner = runner;
                frameState.activeArenaGameplayItem =
                    globalRoot.openGameplay(runner, true);
            }

            function closePreparedArenaGameplay() {
                let item = frameState.activeArenaGameplayItem;
                const runner = frameState.activeArenaGameplayRunner;
                frameState.activeArenaGameplayItem = null;
                frameState.activeArenaGameplayRunner = null;
                if (item && sceneStack.currentItem === item
                        && (!runner || runner.status !== ChartRunner.Finished)) {
                    sceneStack.popCurrentItem();
                }
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

        function scanRootSongFolderForPath(path: var): var {
            let folder = frameImplementation.rootSongFolderForPath(path);
            return !!folder && !!Rg.rootSongFoldersConfig && !!Rg.rootSongFoldersConfig.scanningQueue && Rg.rootSongFoldersConfig.scanningQueue.scan(folder);
        }

        function reloadTableForItem(item: var): var {
            if (!item || item.url === undefined) {
                return false;
            }
            let targetUrl = String(item.url || "");
            if (targetUrl.length === 0) {
                return false;
            }
            let tables = Rg.tables.getList();
            for (let i = 0; i < tables.length; ++i) {
                if (String(tables[i].url || "") === targetUrl) {
                    Rg.tables.reload(i);
                    return true;
                }
            }
            return false;
        }

        /*! The currently presented screen. */
        readonly property Item currentScreen: sceneStack.currentItem

        /*! Returns the retained screen immediately before screen, or null. */
        function previousScreen(screen = currentScreen): var {
            if (!screen || screen.StackView.view !== sceneStack
                    || screen.StackView.index <= 0) {
                return null;
            }
            return sceneStack.get(screen.StackView.index - 1, StackView.DontLoad);
        }

        function returnToPreviousScreen(): var {
            return sceneStack.pop();
        }

        function quitApplication(): void {
            Qt.quit();
        }

        function openSettings(initialTabIndex: var): void {
            let item = frameState.activeSettingsItem === sceneStack.currentItem
                ? frameState.activeSettingsItem : null;
            if (!item) {
                item = sceneStack.pushItem(frameState.settingsComponent);
                frameState.activeSettingsItem = item;
            }
            if (item && initialTabIndex !== undefined && "initialTabIndex" in item) {
                item.initialTabIndex = initialTabIndex;
            }
        }

        function openArenaBrowser(): void {
            if (frameState.activeArenaItem) {
                return;
            }
            Rg.arenaSession.connectForBrowsing();
            let item = sceneStack.pushItem(arenaShellComponent, {
                "session": Rg.arenaSession
            });
            frameState.activeArenaItem = item;
            if (!item) {
                Rg.arenaSession.exitArena();
            }
        }

        function openSelect(): void {
            sceneStack.pushItem(frameState.selectComponent,
                                frameImplementation.selectScreenProperties());
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
            let decideScreen = Rg.themes.availableThemeFamilies[
                frameState.mainProfile.themeConfig.decide].screens.decide;
            let props = {
                "chart": chart
            };
            if (decideScreen.csvPath) {
                props["csvPath"] = decideScreen.csvPath;
                props["skinSettings"] =
                    frameImplementation.currentLr2Settings("decide");
                props["skinSettingsData"] = decideScreen.settingsData || "";
                props["screenKey"] = "decide";
            }
            const item = frameImplementation.createScreen(frameState.decideComponent, props);
            if (item) {
                item.QmlUtils.adopt(chart);
                sceneStack.pushItem(item);
            } else {
                chart.destroy();
            }
            return item;
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
            let decideScreen = Rg.themes.availableThemeFamilies[
                frameState.mainProfile.themeConfig.decide].screens.decide;
            let props = {
                "chart": runner
            };
            if (decideScreen.csvPath) {
                props["csvPath"] = decideScreen.csvPath;
                props["skinSettings"] =
                    frameImplementation.currentLr2Settings("decide");
                props["skinSettingsData"] = decideScreen.settingsData || "";
                props["screenKey"] = "decide";
            }
            const item = frameImplementation.createScreen(frameState.decideComponent, props);
            if (item) {
                item.QmlUtils.adopt(runner);
                sceneStack.pushItem(item);
            } else {
                runner.destroy();
            }
            return item;
        }

        function openGameplay(runner: var, arenaManagedRunner: var): var {
            if (arenaManagedRunner !== true) {
                return globalRoot.replaceGameplay(runner);
            }
            const descriptor = frameImplementation.gameplayDescriptor(
                runner, arenaManagedRunner);
            return sceneStack.pushItem(descriptor.component, descriptor.properties);
        }

        /*! Replaces screen and any screens above it with local gameplay. */
        function replaceGameplay(runner: var, screen = currentScreen): var {
            if (!runner || !screen || screen.StackView.view !== sceneStack
                    || sceneStack.busy) {
                return null;
            }
            const descriptor = frameImplementation.gameplayDescriptor(runner, false);
            // Create first: an invalid skin must leave the existing screen intact.
            const item = frameImplementation.createScreen(descriptor.component,
                                                           descriptor.properties);
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
            item.QmlUtils.adopt(runner);
            sceneStack.updateEnabledStates();
            return item;
        }

        function openResult(scores: var, profiles: var, chartData: var): void {
            let resultScreen = frameImplementation.configuredScreen("result");
            let arenaRoundId = "";
            if (scores && scores.length > 0 && scores[0] && Rg.arenaSession.submitLocalResult(scores[0])) {
                arenaRoundId = String(Rg.arenaSession.presentedResult.roundId || "");
            }
            let props = {
                "scores": scores,
                "profiles": profiles,
                "chartData": chartData
            };
            if (resultScreen && resultScreen.csvPath) {
                props["csvPath"] = resultScreen.csvPath;
                props["skinSettings"] =
                    frameImplementation.currentLr2Settings("result");
                props["skinSettingsData"] = resultScreen.settingsData || "";
                props["screenKey"] = "result";
            }
            const item = sceneStack.pushItem(frameState.resultComponent, props);
            if (arenaRoundId.length === 0) {
                return;
            }
            if (!item || !frameImplementation.callCurrentScreen(
                    "presentArenaResult", [arenaRoundId])) {
                Rg.arenaSession.endResultPresentation(arenaRoundId);
            }
        }

        function openCourseResult(scores: var, profiles: var, chartDatas: var, course: var): void {
            let hasCourseResultScreen = frameImplementation.configuredScreen(
                "courseResult") !== null;
            let courseResultScreen = frameImplementation.configuredScreen(
                "courseResult", "result");
            let props = {
                "scores": scores,
                "profiles": profiles,
                "chartDatas": chartDatas,
                "course": course
            };
            if (courseResultScreen && courseResultScreen.csvPath) {
                let settingsKey = hasCourseResultScreen ? "courseResult" : "result";
                props["csvPath"] = courseResultScreen.csvPath;
                props["skinSettings"] =
                    frameImplementation.currentLr2Settings(settingsKey);
                props["skinSettingsData"] = courseResultScreen.settingsData || "";
                props["screenKey"] = settingsKey;
            }
            sceneStack.pushItem(frameState.courseResultComponent, props);
        }

        anchors.fill: parent

        Connections {
            target: Rg.arenaSession

            function onPreparedGameplayChanged(runner) {
                frameImplementation.openPreparedArenaGameplay(runner);
            }

            function onRoundLaunchCancelled() {
                frameImplementation.closePreparedArenaGameplay();
            }
        }

        Component {
            id: arenaShellComponent

            FocusScope {
                id: arenaShell

                required property ArenaSession session
                property bool closing: false
                readonly property bool showSelect: session.state === ArenaSession.InRoom || session.state === ArenaSession.Reconnecting

                function syncSelectHost(): void {
                    const host = arenaSelectLoader.item;
                    if (!host) {
                        return;
                    }
                    if (showSelect) {
                        host.openSelectScreen();
                        host.forceActiveFocus();
                    } else {
                        host.closeSelectScreen();
                    }
                }

                onShowSelectChanged: syncSelectHost()

                function requestCloseArena(): void {
                    if (closing) {
                        return;
                    }
                    closing = true;
                    Qt.callLater(function () {
                        session.exitArena();
                        if (arenaShell.StackView.view) {
                            arenaShell.StackView.view.popCurrentItem();
                        }
                    });
                }

                function requestLeaveRoom(): void {
                    Qt.callLater(function () {
                        session.leaveRoom();
                    });
                }

                StackView.onRemoved: {
                    frameState.activeArenaItem = null;
                    if (session.active) {
                        session.exitArena();
                    }
                }

                Loader {
                    id: arenaBrowserLoader

                    property bool readyToLoad: false
                    readonly property url configuredSource:
                        frameState.multiplayerScreen
                        ? frameState.multiplayerScreen.script
                        : ""

                    anchors.fill: parent
                    active: true
                    enabled: !arenaShell.showSelect
                    visible: !arenaShell.showSelect

                    function loadConfiguredScreen(): void {
                        if (configuredSource.toString().length === 0) {
                            arenaShell.requestCloseArena();
                            return;
                        }
                        setSource(configuredSource, {
                            "session": arenaShell.session,
                            "activeProfile": frameState.mainProfile
                        });
                    }

                    Component.onCompleted: {
                        readyToLoad = true;
                        loadConfiguredScreen();
                    }
                    onConfiguredSourceChanged: {
                        if (readyToLoad) {
                            loadConfiguredScreen();
                        }
                    }
                    onLoaded: {
                        if (!arenaShell.showSelect && status === Loader.Ready && item) {
                            item.forceActiveFocus();
                        }
                    }
                    onStatusChanged: {
                        if (status === Loader.Error) {
                            console.warn("Failed to load configured multiplayer screen:", configuredSource);
                            arenaShell.requestCloseArena();
                        }
                    }
                    onVisibleChanged: {
                        if (visible && status === Loader.Ready && item) {
                            item.forceActiveFocus();
                        }
                    }
                }

                Connections {
                    target: arenaBrowserLoader.status === Loader.Ready
                        ? arenaBrowserLoader.item
                        : null

                    function onCreateRequested(name, password): void {
                        arenaShell.session.createRoom(name, password);
                    }
                    function onExitRequested(): void {
                        arenaShell.requestCloseArena();
                    }
                    function onJoinRequested(roomId, password): void {
                        arenaShell.session.joinRoom(roomId, password);
                    }
                    function onRetryRequested(): void {
                        arenaShell.session.retry();
                    }
                }

                Component {
                    id: arenaSelectComponent

                    FocusScope {
                        id: arenaSelectHost

                        readonly property var currentScreen: selectStack.currentItem
                        readonly property bool nativeArenaPresentation: currentScreen !== null && currentScreen.arenaNativeSelectPresentation !== undefined && currentScreen.arenaNativeSelectPresentation === true

                        function openSelectScreen(): void {
                            if (currentScreen) {
                                return;
                            }
                            const item = selectStack.pushItem(
                                frameState.selectComponent,
                                frameImplementation.selectScreenProperties());
                            if (item) {
                                item.forceActiveFocus();
                            }
                        }

                        function closeSelectScreen(): void {
                            selectStack.clear(StackView.Immediate);
                        }

                        StackView {
                            id: selectStack

                            anchors.fill: parent
                        }

                        Loader {
                            anchors.fill: parent
                            active: arenaSelectHost.currentScreen !== null && !arenaSelectHost.nativeArenaPresentation
                            sourceComponent: legacySelectOverlayComponent
                            z: 1000000
                        }

                        Component {
                            id: legacySelectOverlayComponent

                            ArenaLegacySelectOverlay {
                                presentationItem: arenaSelectHost.currentScreen
                                session: arenaShell.session
                                themeVars:
                                    frameImplementation.resolvedThemeVars(
                                        "select")
                                viewport: arenaSelectHost
                            }
                        }
                    }
                }

                Loader {
                    id: arenaSelectLoader

                    anchors.fill: parent
                    active: true
                    enabled: arenaShell.showSelect
                    sourceComponent: arenaSelectComponent
                    visible: arenaShell.showSelect

                    onLoaded: {
                        arenaShell.syncSelectHost();
                    }
                }
            }
        }

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

            popEnter: Transition {
                PropertyAnimation {
                    duration: 0
                    properties: "opacity"
                }
            }
            popExit: Transition {
                PropertyAnimation {
                    duration: 0
                    properties: "opacity"
                }
            }
            pushEnter: Transition {
                PropertyAnimation {
                    duration: 0
                    properties: "opacity"
                }
            }
            pushExit: Transition {
                PropertyAnimation {
                    duration: 0
                    properties: "opacity"
                }
            }
            replaceEnter: Transition {
                PropertyAnimation {
                    duration: 0
                    properties: "opacity"
                }
            }
            replaceExit: Transition {
                PropertyAnimation {
                    duration: 0
                    properties: "opacity"
                }
            }
        }
        Binding {
            target: Rg.programSettings
            property: "continuousRendering"
            value: !!sceneStack.currentItem
                && !!sceneStack.currentItem.chart
                && sceneStack.currentItem.chart.status === ChartRunner.Running
        }
        ArenaOverlayHost {
            id: arenaOverlayHost

            anchors.fill: parent
            currentItem: sceneStack.currentItem
            layoutVariant: frameImplementation.gameplayLayoutVariant(
                sceneStack.currentItem)
            resultResolvedSkinId: String(
                frameState.mainProfile.themeConfig.result || "")
            resultThemeVars: frameImplementation.resolvedThemeVars("result")
            session: frameState.arenaSession
            themeVars: frameImplementation.gameplayThemeVars(layoutVariant)
            z: 2000000
        }
        LegacySkinCustomizeHost {
            anchors.fill: parent
            screen: sceneStack.currentItem
            z: 2100000
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

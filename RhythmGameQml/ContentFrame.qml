import QtQuick
import QtQml
import RhythmGameQml
import QtQuick.Controls
import QtQuick.Window
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
        enabled: globalRoot.currentScreenSupports("reloadCurrentFolderOrTable")
        sequence: "F2"
        onActivated: globalRoot.reloadCurrentFolderOrTable()
    }
    Shortcut {
        autoRepeat: false
        enabled: globalRoot.currentScreenSupports("openSelectedFolder")
        sequence: "F3"
        onActivated: globalRoot.openCurrentSelectionFolder()
    }
    Shortcut {
        autoRepeat: false
        sequence: "F4"
        onActivated: globalRoot.toggleFullScreen()
    }
    Shortcut {
        autoRepeat: false
        enabled: globalRoot.currentScreenSupports("openSelectedInternetRanking")
        sequence: "F11"
        onActivated: globalRoot.openCurrentInternetRanking()
    }
    Shortcut {
        autoRepeat: false
        sequence: "F12"
        onActivated: globalRoot.openSettings()
    }

    Item {
        id: globalRoot

        readonly property Profile mainProfile: Rg.profileList.mainProfile
        function configuredScreen(screenKey: var, fallbackKey: var) : var {
            let themeName = mainProfile.themeConfig[screenKey];
            let family = themeName ? Rg.themes.availableThemeFamilies[themeName] : null;
            if (family && family.screens && family.screens[screenKey]) {
                return family.screens[screenKey];
            }
            if (fallbackKey) {
                themeName = mainProfile.themeConfig[fallbackKey];
                family = themeName ? Rg.themes.availableThemeFamilies[themeName] : null;
                if (family && family.screens && family.screens[fallbackKey]) {
                    return family.screens[fallbackKey];
                }
            }
            return null;
        }

        readonly property Component k7Component: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.k7].screens.k7.script)
        readonly property Component k7battleComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.k7battle].screens.k7battle.script)
        readonly property Component k14Component: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.k14].screens.k14.script)
        readonly property Component k5Component: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.k5].screens.k5.script)
        readonly property Component k5battleComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.k5battle].screens.k5battle.script)
        readonly property Component k10Component: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.k10].screens.k10.script)
        readonly property Component mainComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.main].screens.main.script)
        readonly property Component resultComponent: Qt.createComponent(configuredScreen("result").script)
        readonly property Component courseResultComponent: Qt.createComponent(configuredScreen("courseResult", "result").script)
        readonly property Component settingsComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.settings].screens.settings.script)
        readonly property Component selectComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.select].screens.select.script)
        readonly property Component decideComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.decide].screens.decide.script)
        property bool fpsOverlayVisible: false
        property int fpsOverlayValue: -1
        property int fpsOverlayFrameCount: 0
        property double fpsOverlayLastSampleMs: 0

        function isFullScreen() : var {
            return contentContainer.visibility === Window.FullScreen;
        }

        function setFullScreen(enabled: var) : void {
            contentContainer.visibility = enabled ? Window.FullScreen : Window.Windowed;
        }

        function toggleFullScreen() : void {
            setFullScreen(!isFullScreen());
        }

        function toggleFpsOverlay() : void {
            fpsOverlayVisible = !fpsOverlayVisible;
            fpsOverlayValue = -1;
            fpsOverlayFrameCount = 0;
            fpsOverlayLastSampleMs = 0;
        }

        function normalizeLocalPath(path: var) : var {
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

        function localFileUrl(path: var) : var {
            let value = String(path || "").trim();
            if (value.length === 0) {
                return "";
            }
            if (/^file:\/\//i.test(value) || /^[A-Za-z][A-Za-z0-9+.-]*:\/\//.test(value)) {
                return value;
            }
            value = value.replace(/\\/g, "/");
            return value[0] === "/" ? "file://" + encodeURI(value) : "file:///" + encodeURI(value);
        }

        function openLocalFolder(path: var) : var {
            let url = localFileUrl(path);
            return url.length > 0 && Qt.openUrlExternally(url);
        }

        function rootSongFolderForPath(path: var) : var {
            let target = normalizeLocalPath(path);
            if (target.length === 0 || !Rg.rootSongFoldersConfig || !Rg.rootSongFoldersConfig.folders) {
                return null;
            }
            let targetLower = target.toLowerCase();
            let folders = Rg.rootSongFoldersConfig.folders;
            let best = null;
            let bestLength = -1;
            for (let i = 0; i < folders.rowCount(); ++i) {
                let folder = folders.at(i);
                let folderPath = normalizeLocalPath(folder ? folder.name : "");
                if (folderPath.length === 0) {
                    continue;
                }
                let folderLower = folderPath.toLowerCase();
                let matches = targetLower === folderLower || targetLower.startsWith(folderLower + "/");
                if (matches && folderLower.length > bestLength) {
                    best = folder;
                    bestLength = folderLower.length;
                }
            }
            return best;
        }

        function scanRootSongFolderForPath(path: var) : var {
            let folder = rootSongFolderForPath(path);
            return !!folder
                && !!Rg.rootSongFoldersConfig
                && !!Rg.rootSongFoldersConfig.scanningQueue
                && Rg.rootSongFoldersConfig.scanningQueue.scan(folder);
        }

        function reloadTableForItem(item: var) : var {
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

        function currentScreen() : var {
            return sceneStack.currentItem || null;
        }

        function callCurrentScreen(method: var, args: var) : var {
            let screen = currentScreen();
            if (screen && typeof screen[method] === "function") {
                return screen[method].apply(screen, args || []);
            }
            return false;
        }

        function currentScreenSupports(method: var) : var {
            let screen = currentScreen();
            return !!screen && typeof screen[method] === "function";
        }

        function reloadCurrentFolderOrTable() : var {
            return callCurrentScreen("reloadCurrentFolderOrTable");
        }

        function openCurrentSelectionFolder() : var {
            return callCurrentScreen("openSelectedFolder");
        }

        function openCurrentInternetRanking() : var {
            return callCurrentScreen("openSelectedInternetRanking");
        }

        function openSettings(initialTabIndex: var) : void {
            let item = sceneStack.pushItem(settingsComponent);
            if (item && initialTabIndex !== undefined && "initialTabIndex" in item) {
                item.initialTabIndex = initialTabIndex;
            }
        }

        function currentLr2Settings(screenKey: var) : var {
            let themeName = mainProfile.themeConfig[screenKey];
            let screenVars = mainProfile.vars.themeVars[screenKey];
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

        function openSelect() : void {
            let selectScreen = Rg.themes.availableThemeFamilies[mainProfile.themeConfig.select].screens.select;
            let props = {};
            if (selectScreen && selectScreen.csvPath) {
                props["csvPath"] = selectScreen.csvPath;
                props["skinSettings"] = currentLr2Settings("select");
                props["skinSettingsData"] = selectScreen.settingsData || "";
                props["screenKey"] = "select";
            }
            sceneStack.pushItem(selectComponent, props);
        }

        function openChart(path: var, profile1: var, autoplay1: var, replay1: var, score1: var, profile2: var, autoplay2: var, replay2: var, score2: var) : var {
            let chart = Rg.chartLoader.loadChart(path, profile1, autoplay1, replay1, score1, profile2, autoplay2, replay2, score2);
            if (!chart) {
                console.error("Failed to load chart");
                return;
            }
            let decideScreen = Rg.themes.availableThemeFamilies[mainProfile.themeConfig.decide].screens.decide;
            let props = { "chart": chart };
            if (decideScreen.csvPath) {
                props["csvPath"] = decideScreen.csvPath;
                props["skinSettings"] = currentLr2Settings("decide");
                props["skinSettingsData"] = decideScreen.settingsData || "";
                props["screenKey"] = "decide";
            }
            sceneStack.pushItem(decideComponent, props);
        }

        function openCourse(course: var, profile1: var, autoplay1: var, replay1: var, score1: var, profile2: var, autoplay2: var, replay2: var, score2: var) : var {
            let runner = Rg.chartLoader.loadCourse(course, profile1, autoplay1, replay1, score1, profile2, autoplay2, replay2, score2);
            if (!runner) {
                console.error("Failed to load course");
                return;
            }
            let decideScreen = Rg.themes.availableThemeFamilies[mainProfile.themeConfig.decide].screens.decide;
            let props = { "chart": runner };
            if (decideScreen.csvPath) {
                props["csvPath"] = decideScreen.csvPath;
                props["skinSettings"] = currentLr2Settings("decide");
                props["skinSettingsData"] = decideScreen.settingsData || "";
                props["screenKey"] = "decide";
            }
            sceneStack.pushItem(decideComponent, props);
        }

        function openGameplay(runner: var) : void {
            let keys = runner.keymode;
            let battle = runner.player1 && runner.player2;
            let screenKey = "k" + keys + (battle ? "battle" : "");
            let component = globalRoot[screenKey + "Component"];
            let screenObj = Rg.themes.availableThemeFamilies[mainProfile.themeConfig[screenKey]].screens[screenKey];
            let props = { "chart": runner };
            if (screenObj && screenObj.csvPath) {
                props["csvPath"] = screenObj.csvPath;
                props["skinSettings"] = currentLr2Settings(screenKey);
                props["skinSettingsData"] = screenObj.settingsData || "";
                props["screenKey"] = screenKey;
            }
            sceneStack.pushItem(component, props);
        }

        function openResult(scores: var, profiles: var, chartData: var) : void {
            let resultScreen = configuredScreen("result");
            let props = {
                "scores": scores,
                "profiles": profiles,
                "chartData": chartData
            };
            if (resultScreen && resultScreen.csvPath) {
                props["csvPath"] = resultScreen.csvPath;
                props["skinSettings"] = currentLr2Settings("result");
                props["skinSettingsData"] = resultScreen.settingsData || "";
                props["screenKey"] = "result";
            }
            sceneStack.pushItem(resultComponent, props);
        }

        function openCourseResult(scores: var, profiles: var, chartDatas: var, course: var) : void {
            let hasCourseResultScreen = configuredScreen("courseResult") !== null;
            let courseResultScreen = configuredScreen("courseResult", "result");
            let props = {
                "scores": scores,
                "profiles": profiles,
                "chartDatas": chartDatas,
                "course": course
            };
            if (courseResultScreen && courseResultScreen.csvPath) {
                let settingsKey = hasCourseResultScreen ? "courseResult" : "result";
                props["csvPath"] = courseResultScreen.csvPath;
                props["skinSettings"] = currentLr2Settings(settingsKey);
                props["skinSettingsData"] = courseResultScreen.settingsData || "";
                props["screenKey"] = settingsKey;
            }
            sceneStack.pushItem(courseResultComponent, props);
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

            function updateEnabledStates() : void {
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
            initialItem: globalRoot.mainComponent

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
        FrameAnimation {
            running: globalRoot.fpsOverlayVisible

            onTriggered: {
                let now = Date.now();
                if (globalRoot.fpsOverlayLastSampleMs <= 0) {
                    globalRoot.fpsOverlayLastSampleMs = now;
                    globalRoot.fpsOverlayFrameCount = 0;
                    return;
                }
                globalRoot.fpsOverlayFrameCount += 1;
                let elapsed = now - globalRoot.fpsOverlayLastSampleMs;
                if (globalRoot.fpsOverlayValue < 0 && elapsed > 0) {
                    globalRoot.fpsOverlayValue = Math.round(1000 / elapsed);
                }
                if (elapsed >= 500) {
                    globalRoot.fpsOverlayValue = Math.round(globalRoot.fpsOverlayFrameCount * 1000 / elapsed);
                    globalRoot.fpsOverlayFrameCount = 0;
                    globalRoot.fpsOverlayLastSampleMs = now;
                }
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
            visible: globalRoot.fpsOverlayVisible
            width: fpsText.implicitWidth + 14
            z: 1000000

            Text {
                id: fpsText
                anchors.centerIn: parent
                color: "white"
                font.bold: true
                font.pixelSize: 18
                text: (globalRoot.fpsOverlayValue >= 0 ? globalRoot.fpsOverlayValue : "--") + " FPS"
            }
        }
    }
}

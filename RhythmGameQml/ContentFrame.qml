import QtQuick
import QtQml
import RhythmGameQml
import QtQuick.Controls
import QtCore
import org.kde.kirigami as Kirigami

Kirigami.AbstractApplicationWindow {
    id: globalRoot

    height: 720
    width: 1280
    visibility: Window.Windowed

    pageStack: StackView {
        id: __pageStack
        anchors {
            fill: parent
        }
        initialItem: globalRoot.mainComponent

        focus: true
    }

    wideScreen: width >= (globalRoot.pageStack.defaultColumnWidth) + ((contextDrawer && !(contextDrawer instanceof Kirigami.ContextDrawer)) ? contextDrawer.width : 0) + (globalDrawer ? globalDrawer.width : 0)

    Settings {
        id: settings
        property alias height: globalRoot.height
        property alias width: globalRoot.width
        property int visibility
    }
    Component.onCompleted: {
        if (settings.visibility) {
            globalRoot.visibility = settings.visibility;
        }
        pageStack.currentItem?.forceActiveFocus();
    }
    Shortcut {
        autoRepeat: false
        sequence: "F11"

        onActivated: {
            if (globalRoot.visibility === Window.FullScreen) {
                globalRoot.visibility = Window.Windowed;
                settings.visibility = Window.Windowed;
            } else {
                globalRoot.visibility = Window.FullScreen;
                settings.visibility = Window.FullScreen;
            }
        }
    }
    readonly property Profile mainProfile: Rg.profileList.mainProfile
    readonly property Component k7Component: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.k7].screens.k7.script)
    readonly property Component k7battleComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.k7battle].screens.k7battle.script)
    readonly property Component k14Component: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.k14].screens.k14.script)
    readonly property Component mainComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.main].screens.main.script)
    readonly property Component resultComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.result].screens.result.script)
    readonly property Component settingsComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.settings].screens.settings.script)
    readonly property Component selectComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.select].screens.select.script)

    function openChart(path, profile1, autoplay1, score1, profile2, autoplay2, score2) {
        let chart = Rg.chartLoader.loadChart(path, profile1, autoplay1, score1, profile2, autoplay2, score2);
        if (!chart) {
            console.error("Failed to load chart");
            return;
        }
        let keys = chart.keymode;
        let battle = chart.player1 && chart.player2;
        let screen = "k" + keys + (battle ? "battle" : "");
        let component = globalRoot[screen + "Component"];
        pageStack.pushItem(component, {
            "chart": chart
        });
    }

    function openCourse(course, profile1, autoplay1, score1, profile2, autoplay2, score2) {
        let runner = Rg.chartLoader.loadCourse(course, profile1, autoplay1, score1, profile2, autoplay2, score2);
        if (!runner) {
            console.error("Failed to load course");
            return;
        }
        let keys = runner.keymode;
        let battle = runner.player1 && runner.player2;
        let screen = "k" + keys + (battle ? "battle" : "");
        let component = globalRoot[screen + "Component"];
        pageStack.pushItem(component, {
            "chart": runner
        });
    }

    function openResult(scores, profiles, chartData) {
        pageStack.pushItem(resultComponent, {
            "scores": scores,
            "profiles": profiles,
            "chartData": chartData
        });
    }

    function openCourseResult(scores, profiles, chartDatas, course) {
        pageStack.pushItem(resultComponent, {
            "scores": scores,
            "profiles": profiles,
            "chartDatas": chartDatas,
            "course": course
        });
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
}

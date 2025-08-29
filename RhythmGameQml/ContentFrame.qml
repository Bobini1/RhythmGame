import QtQuick
import QtQml
import RhythmGameQml
import QtQuick.Controls.Basic 2.15
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
    Component.onCompleted: {
        if (settings.visibility) {
            contentContainer.visibility = settings.visibility;
        }
    }
    Shortcut {
        autoRepeat: false
        sequence: "F11"

        onActivated: {
            if (contentContainer.visibility === Window.FullScreen) {
                contentContainer.visibility = Window.Windowed;
                settings.visibility = Window.Windowed;
            } else {
                contentContainer.visibility = Window.FullScreen;
                settings.visibility = Window.FullScreen;
            }
        }
    }

    Item {
        id: globalRoot

        readonly property Profile mainProfile: Rg.profileList.mainProfile
        readonly property Component k7Component: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.k7].screens.k7.script)
        readonly property Component k7battleComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.k7battle].screens.k7battle.script)
        readonly property Component k14Component: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.k14].screens.k14.script)
        readonly property Component mainComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.main].screens.main.script)
        readonly property Component resultComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.result].screens.result.script)
        readonly property Component settingsComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.settings].screens.settings.script)
        readonly property Component songWheelComponent: Qt.createComponent(Rg.themes.availableThemeFamilies[mainProfile.themeConfig.songWheel].screens.songWheel.script)

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
            sceneStack.pushItem(component, {
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
            sceneStack.pushItem(component, {
                "chart": runner
            });
        }

        function openResult(scores, profiles, chartData) {
            sceneStack.pushItem(resultComponent, {
                "scores": scores,
                "profiles": profiles,
                "chartData": chartData
            });
        }

        function openCourseResult(scores, profiles, chartDatas, course) {
            sceneStack.pushItem(resultComponent, {
                "scores": scores,
                "profiles": profiles,
                "chartDatas": chartDatas,
                "course": course
            });
        }

        anchors.fill: parent

        StackView {
            id: sceneStack

            onCurrentItemChanged: {
                updateEnabledStates();
            }

            function updateEnabledStates() {
                let topIndex = depth - 1;
                for (let i = 0; i < depth; ++i) {
                    let item = get(i, StackView.ForceLoad);
                    if (item) {
                        item.enabled = (i === topIndex);
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
    }
}

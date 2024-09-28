import QtQuick
import QtQml
import RhythmGameQml
import QtQuick.Controls.Basic 2.15
import QtCore

ApplicationWindow {
    id: contentContainer

    height: 720
    visible: true
    width: 1280
    property var sceneStack: sceneStack

    Settings {
        property alias height: contentContainer.height
        property alias width: contentContainer.width
    }
    Component {
        id: chartContext

        FocusScope {
            id: chartFocusScope

            readonly property bool active: StackView.status === StackView.Active
            required property Chart chart

            Loader {
                id: loader

                anchors.fill: parent
                source: Themes.availableThemeFamilies[ProfileList.currentProfile.themeConfig.gameplay].screens.gameplay.script
            }
        }
    }
    Component {
        id: resultContext

        FocusScope {
            id: resultFocusScope

            readonly property bool active: StackView.status === StackView.Active
            required property ChartData chartData
            required property BmsScoreAftermath result

            Loader {
                id: loader

                anchors.fill: parent
                source: Themes.availableThemeFamilies[ProfileList.currentProfile.themeConfig.result].screens.result.script
            }
        }
    }
    Item {
        id: globalRoot

        readonly property Component gameplayComponent: chartContext
        readonly property Component mainComponent: Qt.createComponent(Themes.availableThemeFamilies[ProfileList.currentProfile.themeConfig.main].screens.main.script)
        readonly property Component resultComponent: resultContext
        readonly property Component settingsComponent: Qt.createComponent(Themes.availableThemeFamilies[ProfileList.currentProfile.themeConfig.settings].screens.settings.script)
        readonly property Component songWheelComponent: Qt.createComponent(Themes.availableThemeFamilies[ProfileList.currentProfile.themeConfig.songWheel].screens.songWheel.script)

        function openChart(path) {
            let chart = ChartLoader.loadChart(path);
            if (!chart) {
                console.error("Failed to load chart");
                return;
            }
            sceneStack.push(gameplayComponent, {
                "chart": chart
            });
        }

        function openResult(result, chartData) {
            sceneStack.push(resultComponent, {
                "result": result,
                "chartData": chartData
            });
        }

        function urlToPath(urlString) {
            let s;
            if (urlString.startsWith("file:///")) {
                let k = urlString.charAt(9) === ':' ? 8 : 7;
                s = urlString.substring(k);
            } else {
                s = urlString;
            }
            return decodeURIComponent(s);
        }

        anchors.fill: parent

        Component.onCompleted: {
            if (ProgramSettings.chartPath != "")
                openChart(ProgramSettings.chartPath);
        }

        StackView {
            id: sceneStack

            anchors.fill: parent
            initialItem: (ProgramSettings.chartPath != "") ? null : globalRoot.mainComponent

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
            sequence: "F11"

            onActivated: {
                debugLogLoader.active = !debugLogLoader.active;
            }
        }
    }
}

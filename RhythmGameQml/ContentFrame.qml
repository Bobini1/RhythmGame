import QtQuick
import QtQml
import RhythmGameQml
import QtQuick.Controls 2.15
import QtCore

ApplicationWindow {
    id: contentContainer

    height: 720
    visibility: Window.FullScreen
    visible: true
    width: 1280

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

            InputItem {
                id: inputItem

                chart: chartFocusScope.chart
            }
            Loader {
                id: loader

                anchors.fill: parent
                source: SceneUrls.gameplayScene
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
                source: SceneUrls.resultScene
            }
        }
    }
    Item {
        id: globalRoot

        readonly property Component gameplayComponent: chartContext
        readonly property Component mainComponent: Qt.createComponent(SceneUrls.mainScene)
        readonly property Component resultComponent: resultContext
        readonly property Component settingsComponent: Qt.createComponent(SceneUrls.settingsScene)
        readonly property Component songWheelComponent: Qt.createComponent(SceneUrls.songWheelScene)

        function openChart(path: url) {
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
            if (ProgramSettings.chartPath != "") {
                openChart(ProgramSettings.chartPath);
            }
        }

        StackView {
            id: sceneStack

            anchors.fill: parent
            initialItem: (ProgramSettings.chartPath != "") ? null : globalRoot.mainComponent

            popEnter: Transition {
                PropertyAnimation {
                    duration: 0
                    property: "opacity"
                }
            }
            popExit: Transition {
                PropertyAnimation {
                    duration: 0
                    property: "opacity"
                }
            }
            pushEnter: Transition {
                PropertyAnimation {
                    duration: 0
                    property: "opacity"
                }
            }
            pushExit: Transition {
                PropertyAnimation {
                    duration: 0
                    property: "opacity"
                }
            }
            replaceEnter: Transition {
                PropertyAnimation {
                    duration: 0
                    property: "opacity"
                }
            }
            replaceExit: Transition {
                PropertyAnimation {
                    duration: 0
                    property: "opacity"
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

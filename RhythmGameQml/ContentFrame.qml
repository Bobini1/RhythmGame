import QtQuick
import QtQml
import RhythmGameQml
import QtQuick.Controls 2.15
import QtCore

ApplicationWindow {
    id: contentContainer

    height: 720
    visible: true
    width: 1280

    Settings {
        property alias height: contentContainer.height
        property alias width: contentContainer.width
    }
    Component {
        id: chartContext

        Item {
            required property var chart

            focus: true

            Keys.onPressed: event => {
                if (event.isAutoRepeat) {
                    return;
                }
                if (event.key === Qt.Key_Escape) {
                    sceneStack.pop();
                } else {
                    chart.passKey(event.key);
                }
                event.accepted = true;
            }

            Loader {
                id: loader

                anchors.fill: parent
                source: "file://" + SceneUrls.gameplaySceneUrl
            }
        }
    }
    Item {
        id: globalRoot

        readonly property Component gameplayComponent: chartContext
        readonly property Component mainComponent: Qt.createComponent("file://" + SceneUrls.mainSceneUrl)
        readonly property Component settingsComponent: Qt.createComponent("file://" + SceneUrls.settingsSceneUrl)
        readonly property Component songWheelComponent: Qt.createComponent("file://" + SceneUrls.songWheelSceneUrl)

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

            onCurrentItemChanged: {
                currentItem.forceActiveFocus();
            }
        }
        Loader {
            id: debugLogLoader

            active: false
            anchors.fill: parent
            source: "Log.qml"
        }
        Shortcut {
            autoRepeat: false
            sequence: "F11"

            onActivated: {
                debugLogLoader.active = !debugLogLoader.active;
            }
        }
        Shortcut {
            enabled: true
            sequence: "Esc"

            onActivated: {
                sceneStack.pop();
            }
        }
    }
}

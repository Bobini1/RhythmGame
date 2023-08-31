import QtQuick 2.0
import QtQml
import RhythmGameQml
import QtQuick.Controls 2.15

Item {
    id: contentContainer

    anchors.fill: parent

    Component {
        id: chartContext

        Item {
            required property var chart

            focus: true

            Keys.onPressed: event => {
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
                source: SceneUrls.gameplaySceneUrl
            }
        }
    }
    Item {
        id: root

        readonly property Component gameplayComponent: chartContext
        readonly property Component mainComponent: Qt.createComponent(SceneUrls.mainSceneUrl)
        readonly property Component songWheelComponent: Qt.createComponent(SceneUrls.songWheelSceneUrl)

        function openChart(path: URL) {
            let chart = ChartLoader.loadChart(path);
            if (!chart) {
                console.error("Failed to load chart");
                return;
            }
            sceneStack.push(gameplayComponent, {
                    "chart": chart
                });
        }

        anchors.fill: parent

        Component.onCompleted: {
            if (ProgramSettings.chartPath) {
                openChart(ProgramSettings.chartPath);
            }
        }

        StackView {
            id: sceneStack

            anchors.fill: parent
            initialItem: ProgramSettings.chartPath ? null : root.mainComponent

            onCurrentItemChanged: {
                currentItem.forceActiveFocus();
            }
        }
    }
}

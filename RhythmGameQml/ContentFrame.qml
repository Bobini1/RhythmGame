import QtQuick 2.0
import QtQml
import RhythmGameQml
import QtQuick.Controls 2.15

Item {
    id: contentContainer

    Component.OnCompleted: {
        if (ProgramSettings.chartPath) {
            root.openChart(ProgramSettings.chartPath);
        }
    }
    anchors.fill: parent

    Component {
        id: chartContext

        Item {
            required property Chart chart

            Keys.onPressed: event => {
                if (event.key === Qt.Key_Escape) {
                    sceneStack.pop();
                }
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
            sceneStack.push(gameplayComponent, {
                    "chartData": chart
                });
        }

        anchors.fill: parent

        StackView {
            id: sceneStack

            anchors.fill: parent
            initialItem: ProgramSettings.chartPath ? null : root.mainComponent
        }
    }
}

import QtQuick
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

        // create a rectangle that shows when you click f11 that shows debug logs
        Loader {
            id: debugLogLoader

            active: false
            anchors.fill: parent
            sourceComponent: debugLogComponent
        }
        Component {
            id: debugLogComponent

            Rectangle {
                id: debugLog

                function scrollToBottom() {
                    logScroll.ScrollBar.vertical.position = 1.0 - logScroll.ScrollBar.vertical.size;
                }

                anchors.fill: parent
                color: "black"
                opacity: 0.5

                states: State {
                    id: flick

                    name: "autoscroll"

                    PropertyChanges {
                        position: 1.0 - logScroll.ScrollBar.vertical.size
                        target: logScroll.ScrollBar.vertical
                    }
                }

                Component.onCompleted: {
                    scrollToBottom();
                }

                Connections {
                    function onPositionChanged() {
                        if (1.0 - logScroll.ScrollBar.vertical.size - logScroll.ScrollBar.vertical.position < 0.01) {
                            //change state
                            if (state !== "autoscroll") {
                                state = "autoscroll";
                            }
                        } else {
                            if (state === "") {
                                state = "";
                            }
                        }
                    }

                    target: logScroll.ScrollBar.vertical
                }
                ScrollView {
                    id: logScroll

                    anchors.fill: parent

                    ListView {
                        id: debugLogText

                        anchors.fill: parent
                        model: Logger.history

                        delegate: TextEdit {
                            color: "yellow"
                            font.family: "Courier"
                            font.pixelSize: 20
                            readOnly: true
                            text: display
                            textFormat: TextEdit.PlainText
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
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

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

        // create a rectangle that shows when you click f12 that shows debug logs
        Rectangle {
            id: debugLog

            anchors.fill: parent
            color: "black"
            opacity: 0.5
            visible: false

            // make it scrollable
            ScrollView {
                id: logScroll

                function isAtBottom() {
                    return logScroll.ScrollBar.vertical.position === 1.0 - logScroll.ScrollBar.vertical.size;
                }
                function scrollToBottom() {
                    logScroll.ScrollBar.vertical.position = 1.0 - logScroll.ScrollBar.vertical.size;
                }

                anchors.fill: parent

                Component.onCompleted: {
                    scrollToBottom();
                }

                TextEdit {
                    id: debugLogText

                    anchors.fill: parent
                    color: "yellow"
                    font.family: "Courier"
                    font.pixelSize: 20
                    readOnly: true
                    text: ""
                    wrapMode: Text.WordWrap

                    Connections {
                        function onLogged(message) {
                            let wasAtBottom = logScroll.isAtBottom();
                            // get current selection
                            let selectionStart = debugLogText.selectionStart;
                            let selectionEnd = debugLogText.selectionEnd;
                            debugLogText.text += message + "\n";
                            // restore selection
                            debugLogText.select(selectionStart, selectionEnd);
                            if (wasAtBottom) {
                                logScroll.scrollToBottom();
                            }
                        }

                        target: Logger
                    }
                }
            }
        }
        Shortcut {
            context: Qt.ApplicationShortcut
            sequence: "F11"

            onActivated: {
                debugLog.visible = !debugLog.visible;
            }
        }
    }
}

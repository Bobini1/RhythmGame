import QtQuick
import QtQml
import RhythmGameQml
import QtQuick.Controls 2.15

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
        console.log("Debug log initialized");
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

import RhythmGameQml
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: bg

    color: "white"
    radius: 32

    scale: 0

    width: 1600
    height: 900

    Component {
        id: delegateComponent

        Text {
            text: modelData
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 32

            color: index === Tumbler.tumbler.currentIndex ? "white" : "black"
        }
    }


    TumblerFrame {
        width: 360
        height: 220

        DarkHighlightLabel {
            width: parent.width
            height: 40

            anchors.centerIn: parent
        }
        Tumbler {
            id: randoms
            model: ["NORMAL", "MIRROR", "SPIRAL", "RANDOM", "S-RANDOM", "H-RANDOM", "R-RANDOM", "R-RANDOM"]
            delegate: delegateComponent
            wrap: true
            visibleItemCount: 5

            anchors.fill: parent
        }
    }

    states: State {
        name: "shown"; when: InputTranslator.start
        PropertyChanges {
            target: bg
            scale: 1
        }
    }

    transitions: Transition {
        NumberAnimation {
            properties: "scale"
            easing.type: Easing.InOutQuad
            duration: 150
        }
    }
}
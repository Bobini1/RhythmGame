import RhythmGameQml
import QtQuick
import QtQuick.Controls.Basic

BorderImage {
    id: frame
    required property var model
    property var strings: model
    required property string prop
    property var up: -1
    property var down: -1
    required property Profile profile

    source: root.iniImagesUrl + "option.png/tumbler_frame"

    border {
        left: 15
        right: 15
        top: 15
        bottom: 15
    }

    Component {
        id: delegateComponent

        Text {
            text: frame.strings[index]
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 32

            color: {
                let c = Qt.color("black");
                let value = Math.max(0, 1 - Math.abs(Tumbler.displacement));
                c.hslLightness = value;
                return c;
            }
        }
    }
    DarkHighlightLabel {
        id: label
        width: parent.width
        height: 40

        anchors.centerIn: parent
    }
    PathView {
        id: tumbler
        clip: true

        delegate: delegateComponent
        model: frame.model
        pathItemCount: 6
        dragMargin: width / 2
        path: Path {
            startX: frame.width / 2
            startY: -label.height / 2
            PathLine {
                x: frame.width / 2; y: frame.height + label.height / 2
            }
        }
        preferredHighlightBegin: 3 / 6
        preferredHighlightEnd: 3 / 6

        anchors.fill: parent

        function getIndex(text) {
            let index = 0;
            let proposition = null;
            for (let choice of frame.model) {
                if (text === choice) {
                    // special handling for duplicated elements
                    if (proposition === null || Math.abs(currentIndex - index) < Math.abs(currentIndex - proposition)) {
                        proposition = index
                    }
                }
                index++;
            }
            return proposition;
        }

        Component.onCompleted: {
            ready = true;
        }

        property bool ready: false

        Binding {
            delayed: true
            tumbler.currentIndex: tumbler.getIndex(frame.profile.vars.globalVars[frame.prop]);
        }

        Binding {
            target: frame.profile.vars.globalVars
            property: frame.prop
            when: tumbler.ready && !tumbler.flicking
            value: if (!tumbler.flicking) {
                return frame.model[tumbler.currentIndex];
            }
        }

        highlightMoveDuration: 150
        WheelHandler {
            property int previousDirection: 0

            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            target: tumbler
            onWheel: (wheel) => {
                if (wheel.angleDelta.y > 0) {
                    tumbler.decrementCurrentIndex();
                } else {
                    tumbler.incrementCurrentIndex();
                }
            }
        }

        Input.onButtonPressed: (profile, key) => {
            if (profile !== frame.profile) {
                return;
            }
            if (key === up && bg.open) {
                tumbler.decrementCurrentIndex();
            }
            if (key === down && bg.open) {
                tumbler.incrementCurrentIndex();
            }
        }
    }
}
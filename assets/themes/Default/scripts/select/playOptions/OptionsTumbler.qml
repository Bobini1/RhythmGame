import RhythmGameQml
import QtQuick
import QtQuick.Controls.Basic

TumblerFrame {
    id: frame
    required property var model
    required property string prop

    DarkHighlightLabel {
        width: parent.width
        height: 40

        anchors.centerIn: parent
    }
    Tumbler {
        id: tumbler
        delegate: delegateComponent
        wrap: true
        model: frame.model
        visibleItemCount: 5

        anchors.fill: parent

        function findView(parent) {
            for (let i = 0; i < parent.children.length; ++i) {
                let child = parent.children[i];
                if (child.hasOwnProperty("currentIndex")) {
                    return child;
                }

                let grandChild = findView(child);
                if (grandChild)
                    return grandChild;
            }

            return null;
        }


        function getIndex(text) {
            let index = 0;
            for (let choice of frame.model) {
                if (text === choice) {
                    return index;
                    break;
                }
                index++;
            }
        }

        currentIndex: getIndex(ProfileList.currentProfile.vars.globalVars[prop]);

        onCurrentIndexChanged: (_) => {
            // doesn't work!
            ProfileList.currentProfile.vars.globalVars[prop] = frame.model[currentIndex];
            currentIndex = Qt.binding(() => getIndex(ProfileList.currentProfile.vars.globalVars[prop]))
        }

        WheelHandler {
            target: tumbler
            onWheel: (wheel) => {
                let view = tumbler.findView(tumbler);
                view.highlightMoveDuration = 200;
                if (wheel.angleDelta.y > 0) {
                    tumbler.findView(tumbler).decrementCurrentIndex();
                } else {
                    tumbler.findView(tumbler).incrementCurrentIndex();
                }
            }
        }
    }
}
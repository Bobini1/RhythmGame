import QtQuick
import RhythmGameQml
import QtQuick.Dialogs

Row {
    height: rect.height

    width: text.width + 390

    required property string prop
    property string description: prop
    property bool global: false
    readonly property var src: global ? ProfileList.currentProfile.vars.globalVars : root.vars

    Text {
        id: text
        anchors.verticalCenter: rect.verticalCenter
        color: "white"
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        text: parent.description
        verticalAlignment: Text.AlignVCenter
        width: 110
    }

    Rectangle {
        id: rect

        height: 30
        border {
            color: "white"
            width: 2
        }

        width: 390

        color: src[prop]

        Component.onCompleted: {
            print(width, height)
        }

        MouseArea {
            anchors.fill: parent
            onClicked: colorDialog.open()
        }

        ColorDialog {
            id: colorDialog

            selectedColor: src[prop]

            onSelectedColorChanged: {
                // noinspection SillyAssignmentJS
                selectedColor = selectedColor;
                src[prop] = selectedColor;
            }

            onAccepted: {
                selectedColor = Qt.binding(() => src[prop]);
            }

            onRejected: {
                selectedColor = Qt.binding(() => src[prop]);
            }
        }
    }
}
import QtQuick
import RhythmGameQml
import QtQuick.Dialogs

Row {
    height: rect.height

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
            color: "black"
            width: 1
        }

        width: parent.width - text.width

        color: src[prop]

        MouseArea {
            anchors.fill: parent
            onClicked: colorDialog.open()
        }

        ColorDialog {
            id: colorDialog

            selectedColor: src[prop]

            onSelectedColorChanged: {
                src[prop] = selectedColor;
                selectedColor = Qt.binding(() => src[prop]);
            }
        }
    }
}

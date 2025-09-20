import QtQuick
import RhythmGameQml
import QtQuick.Dialogs

Row {
    height: rect.height

    width: text.width + 390

    required property string prop
    property string description: prop
    required property var src

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

        MouseArea {
            anchors.fill: parent
            onClicked:
            {
                rect.oldColor = src[prop]
                colorDialog.open()
            }
        }

        property color oldColor

        ColorDialog {
            id: colorDialog

            selectedColor: src[prop]

            Binding {
                target: src
                property: prop
                value: colorDialog.selectedColor
            }

            onRejected: {
                src[prop] = rect.oldColor
            }
        }

    }
}

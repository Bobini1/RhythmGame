import QtQuick
import RhythmGameQml
import QtQuick.Dialogs

Row {
    required property string prop
    property string description: prop
    required property var src
    height: 40
    spacing: 10

    Text {
        id: text
        anchors.verticalCenter: rect.verticalCenter
        color: "white"
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        text: parent.description
        verticalAlignment: Text.AlignVCenter
        width: 160
        fontSizeMode: Text.Fit
    }

    Rectangle {
        id: rect

        height: 30
        border {
            color: "white"
            width: 2
        }

        width: 330

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

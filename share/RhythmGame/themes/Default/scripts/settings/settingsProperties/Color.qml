import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml
import QtQuick
import ".."

RowLayout {
    id: colorPicker
    spacing: 12
    Layout.fillWidth: true
    Layout.minimumHeight: 34

    property var destination
    property string id_
    property var colorPickers
    property alias name: strLabel.text
    property alias description: strLabel.description
    property var default_

    SettingsLabel {
        id: strLabel
    }

    Rectangle {
        id: colorPickerRectangle

        Layout.preferredHeight: 30
        border {
            color: palette.shadow
            width: 1
        }
        Layout.fillWidth: true
        Layout.preferredWidth: 400
        Layout.minimumWidth: 200

        color: colorPicker.destination[colorPicker.id_]

        MouseArea {
            anchors.fill: parent
            onClicked: colorDialog.open()
        }

        ColorDialog {
            id: colorDialog

            selectedColor: colorPicker.destination[colorPicker.id_]
            title: colorPicker.name

            onAccepted: {
                colorPicker.destination[colorPicker.id_] = colorDialog.selectedColor;
            }
        }
    }


    ResetButton {
        destination: colorPicker.destination
        id_: colorPicker.id_
        default_: colorPicker.default_
        Layout.preferredWidth: 84
        Layout.minimumWidth: 76

        onClicked: {
            colorPicker.destination[colorPicker.id_] = colorPicker.default_
        }
    }
}

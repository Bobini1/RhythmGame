import QtQuick.Dialogs
import QtQuick.Controls
import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import ".."

RowLayout {
    id: str
    spacing: 12
    Layout.fillWidth: true
    Layout.minimumHeight: 34

    property var destination
    property string id_
    property alias name: strLabel.text
    property alias description: strLabel.description
    property var default_

    SettingsLabel {
        id: strLabel
    }

    TextField {
        id: textEdit
        text: destination[id_]
        Layout.fillWidth: true
        Layout.preferredWidth: 460
        Layout.minimumWidth: 220

        onTextChanged: {
            str.destination[str.id_] = text;
        }
    }


    ResetButton {
        destination: str.destination
        id_: str.id_
        default_: str.default_
        Layout.preferredWidth: 84
        Layout.minimumWidth: 76

        onClicked: {
            str.destination[str.id_] = str.default_
        }
    }
}


import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import ".."

RowLayout {
    id: str
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
        Layout.preferredWidth: 400
        Layout.minimumWidth: 200

        onTextChanged: {
            str.destination[str.id_] = text;
        }
    }


    ResetButton {
        destination: str.destination
        id_: str.id_
        default_: str.default_

        onClicked: {
            str.destination[str.id_] = str.default_
        }
    }
}


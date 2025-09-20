import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import ".."

RowLayout {
    id: booleanProp
    property var destination
    property string id_
    property alias name: strLabel.text
    property alias description: strLabel.description
    property var default_
    SettingsLabel {
        id: strLabel
    }
    CheckBox {
        id: checkBox

        checked: destination[id_]
        onCheckedChanged: {
            destination[id_] = checked
        }
        Layout.fillWidth: true
        Layout.preferredWidth: 400
        Layout.minimumWidth: 200
    }
    ResetButton {
        destination: booleanProp.destination
        id_: booleanProp.id_
        default_: booleanProp.default_

        onClicked: {
            booleanProp.destination[booleanProp.id_] = booleanProp.default_
        }
    }
}

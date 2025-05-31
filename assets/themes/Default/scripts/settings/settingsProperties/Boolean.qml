import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick

CheckBox {
    id: checkBox

    property var destination
    property string id_

    checked: destination[id_]
    onCheckedChanged: {
        destination[id_] = checked
    }
}


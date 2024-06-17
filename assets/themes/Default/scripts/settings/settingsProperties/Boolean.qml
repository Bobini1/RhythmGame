import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick

CheckBox {
    id: checkBox

    checked: destination[props.id]
    onCheckedChanged: {
        destination[props.id] = checked
        checked = Qt.binding(() => destination[props.id])
    }
}
import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick

TextArea {
    id: textEdit
    implicitWidth: 200
    text: destination[props.id]

    onTextChanged: {
        destination[props.id] = text;
    }
}
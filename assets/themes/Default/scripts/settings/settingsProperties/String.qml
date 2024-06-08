import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick

TextArea {
    id: textEdit

    anchors.fill: parent
    text: destination[props.id]

    onTextChanged: {
        destination[props.id] = text;
    }
}
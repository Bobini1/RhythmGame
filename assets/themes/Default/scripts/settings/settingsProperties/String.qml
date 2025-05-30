import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick

TextArea {
    id: textEdit
    implicitWidth: 200
    text: destination[id_]
    property var destination
    property string id_

    onTextChanged: {
        destination[id_] = text;
    }
}
import QtQuick
import QtQuick.Layouts

TextEdit {
    id: text
    font.pixelSize: 16
    font.bold: true
    readOnly: true
    Layout.alignment: Qt.AlignVCenter
    Layout.preferredWidth: 200
    wrapMode: TextEdit.Wrap
}
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

TextEdit {
    property string description
    font.pixelSize: 16
    font.bold: true
    wrapMode: TextEdit.Wrap
    readOnly: true
    HoverHandler {
        id: hoverHandler
    }
    ToolTip.text: description
    ToolTip.visible: hoverHandler.hovered && description
    Layout.fillWidth: true
    Layout.minimumWidth: 200
    Layout.maximumWidth: 1000
}
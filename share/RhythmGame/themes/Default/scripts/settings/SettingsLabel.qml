import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Label {
    property string description
    font.pixelSize: 16
    font.bold: true
    HoverHandler {
        id: hoverHandler
    }
    ToolTip.text: description
    ToolTip.visible: hoverHandler.hovered && description
    Layout.fillWidth: true
    Layout.minimumWidth: 200
    Layout.maximumWidth: 1000
    Layout.preferredWidth: 300
}
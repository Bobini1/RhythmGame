import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Control {
    id: root

    property alias text: titleLabel.text
    property alias description: descriptionLabel.text

    Layout.fillWidth: true
    Layout.minimumWidth: 200
    Layout.maximumWidth: 1000
    Layout.preferredWidth: 300
    padding: 0

    HoverHandler {
        id: hoverHandler
    }

    ToolTip.text: root.description
    ToolTip.visible: hoverHandler.hovered && root.description.length > 0

    contentItem: ColumnLayout {
        spacing: 2

        Label {
            id: titleLabel
            color: root.palette.windowText
            font.pixelSize: 16
            font.bold: true
            elide: Text.ElideRight
            maximumLineCount: 1
            Layout.fillWidth: true
        }

        Label {
            id: descriptionLabel
            visible: text.length > 0
            color: Qt.rgba(root.palette.windowText.r, root.palette.windowText.g, root.palette.windowText.b, 0.68)
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}

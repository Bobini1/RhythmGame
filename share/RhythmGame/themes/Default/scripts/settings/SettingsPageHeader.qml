import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "SettingsColors.js" as SettingsColors

Control {
    id: header

    property string title
    property string subtitle

    Layout.fillWidth: true
    padding: 0

    contentItem: ColumnLayout {
        spacing: 4

        Label {
            text: header.title
            visible: header.title.length > 0
            color: header.palette.windowText
            font.pixelSize: 24
            font.bold: true
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        Label {
            text: header.subtitle
            visible: header.subtitle.length > 0
            color: SettingsColors.alpha(header.palette.windowText, 0.7)
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "SettingsColors.js" as SettingsColors

Control {
    id: emptyState

    property string title
    property string subtitle

    Layout.fillWidth: true
    padding: 0

    contentItem: ColumnLayout {
        spacing: 6

        Label {
            text: emptyState.title
            visible: emptyState.title.length > 0
            color: emptyState.palette.windowText
            font.pixelSize: 16
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        Label {
            text: emptyState.subtitle
            visible: emptyState.subtitle.length > 0
            color: SettingsColors.alpha(emptyState.palette.windowText, 0.68)
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }
    }
}

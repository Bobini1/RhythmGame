import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "SettingsColors.js" as SettingsColors

Frame {
    id: panel

    property string title
    property string subtitle
    default property alias content: contentLayout.data

    padding: 16
    implicitWidth: Math.max(360, contentItem.implicitWidth + leftPadding + rightPadding)

    background: Rectangle {
        radius: 8
        color: SettingsColors.panel(panel.palette)
        border.width: 1
        border.color: SettingsColors.panelBorder(panel.palette)
    }

    contentItem: ColumnLayout {
        spacing: 12

        ColumnLayout {
            visible: panel.title.length > 0 || panel.subtitle.length > 0
            spacing: 3
            Layout.fillWidth: true

            Label {
                text: panel.title
                visible: panel.title.length > 0
                color: panel.palette.windowText
                font.pixelSize: 17
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: panel.subtitle
                visible: panel.subtitle.length > 0
                color: SettingsColors.alpha(panel.palette.windowText, 0.72)
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        ColumnLayout {
            id: contentLayout
            spacing: 10
            Layout.fillWidth: true
        }
    }
}

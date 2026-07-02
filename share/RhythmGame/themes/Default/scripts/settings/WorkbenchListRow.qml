import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "SettingsColors.js" as SettingsColors

ItemDelegate {
    id: row

    property string primaryText
    property string secondaryText
    property string metaText
    property bool selected: false
    default property alias actions: actionRow.data

    implicitHeight: Math.max(48, contentItem.implicitHeight + topPadding + bottomPadding)
    padding: 10
    leftPadding: 14
    rightPadding: 10
    topPadding: 8
    bottomPadding: 8

    background: Rectangle {
        radius: 6
        color: SettingsColors.rowFill(row.palette, row.selected, row.hovered)

        Rectangle {
            width: 3
            radius: 2
            color: row.palette.highlight
            visible: row.selected
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
        }
    }

    contentItem: RowLayout {
        spacing: 10

        ColumnLayout {
            spacing: 2
            Layout.fillWidth: true
            Layout.minimumWidth: 120

            RowLayout {
                spacing: 8
                Layout.fillWidth: true

                Label {
                    text: row.primaryText
                    color: row.palette.windowText
                    font.bold: row.selected
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    Layout.fillWidth: true
                }

                Label {
                    text: row.metaText
                    visible: row.metaText.length > 0
                    color: SettingsColors.alpha(row.palette.windowText, 0.62)
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    Layout.maximumWidth: 160
                }
            }

            Label {
                text: row.secondaryText
                visible: row.secondaryText.length > 0
                color: SettingsColors.alpha(row.palette.windowText, 0.7)
                elide: Text.ElideRight
                maximumLineCount: 1
                Layout.fillWidth: true
            }
        }

        RowLayout {
            id: actionRow
            spacing: 6
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        }
    }
}

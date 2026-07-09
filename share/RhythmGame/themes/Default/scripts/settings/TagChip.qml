import QtQuick
import QtQuick.Controls

import "SettingsColors.js" as SettingsColors

Label {
    id: chip

    padding: 5
    leftPadding: 8
    rightPadding: 8
    topPadding: 3
    bottomPadding: 3
    color: SettingsColors.alpha(chip.palette.windowText, 0.76)
    font.pixelSize: 12
    elide: Text.ElideRight
    maximumLineCount: 1

    background: Rectangle {
        radius: 999
        color: SettingsColors.chipFill(chip.palette)
        border.width: 1
        border.color: SettingsColors.panelBorder(chip.palette)
    }
}

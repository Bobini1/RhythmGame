import QtQuick
import QtQuick.Controls

import "SettingsColors.js" as SettingsColors

Label {
    id: chip

    enum Tone {
        Neutral,
        Accent,
        Danger
    }

    property int tone: StatusChip.Neutral

    padding: 5
    leftPadding: 8
    rightPadding: 8
    topPadding: 3
    bottomPadding: 3
    color: chip.tone === StatusChip.Danger ? SettingsColors.dangerText(chip.palette)
           : chip.tone === StatusChip.Accent ? chip.palette.highlight
           : SettingsColors.alpha(chip.palette.windowText, 0.76)
    font.pixelSize: 12
    font.bold: chip.tone !== StatusChip.Neutral
    elide: Text.ElideRight
    maximumLineCount: 1

    background: Rectangle {
        radius: 999
        color: chip.tone === StatusChip.Danger ? SettingsColors.dangerFill(chip.palette, false, false)
               : chip.tone === StatusChip.Accent ? SettingsColors.alpha(chip.palette.highlight, SettingsColors.isLight(chip.palette.window) ? 0.14 : 0.22)
               : SettingsColors.chipFill(chip.palette)
        border.width: 1
        border.color: chip.tone === StatusChip.Accent ? SettingsColors.alpha(chip.palette.highlight, 0.48)
                      : SettingsColors.panelBorder(chip.palette)
    }
}

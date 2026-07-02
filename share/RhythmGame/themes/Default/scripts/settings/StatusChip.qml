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
           : chip.tone === StatusChip.Accent ? chip.palette.highlightedText
           : SettingsColors.alpha(chip.palette.windowText, 0.76)
    font.pixelSize: 12
    font.bold: chip.tone !== StatusChip.Neutral
    elide: Text.ElideRight
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    maximumLineCount: 1

    background: Rectangle {
        radius: 999
        color: chip.tone === StatusChip.Danger ? SettingsColors.dangerFill(chip.palette, false, false)
               : chip.tone === StatusChip.Accent ? SettingsColors.primaryFill(chip.palette, false, false)
               : SettingsColors.chipFill(chip.palette)
        border.width: 1
        border.color: chip.tone === StatusChip.Accent ? SettingsColors.alpha(chip.palette.highlightedText, 0.35)
                      : SettingsColors.panelBorder(chip.palette)
    }
}

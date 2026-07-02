import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "SettingsColors.js" as SettingsColors

Button {
    id: control

    enum Tone {
        Primary,
        Secondary,
        Tertiary,
        Danger
    }

    property int tone: ActionButton.Secondary

    implicitHeight: 32
    implicitWidth: Math.max(88, contentItem.implicitWidth + leftPadding + rightPadding)
    padding: 8
    leftPadding: 12
    rightPadding: 12
    topPadding: 5
    bottomPadding: 5

    readonly property color fillColor: {
        switch (control.tone) {
        case ActionButton.Primary:
            return SettingsColors.primaryFill(control.palette, control.down, control.hovered);
        case ActionButton.Tertiary:
            return SettingsColors.tertiaryFill(control.palette, control.down, control.hovered);
        case ActionButton.Danger:
            return SettingsColors.dangerFill(control.palette, control.down, control.hovered);
        default:
            return SettingsColors.secondaryFill(control.palette, control.down, control.hovered);
        }
    }

    readonly property color foregroundColor: {
        if (!control.enabled) {
            return SettingsColors.alpha(control.palette.buttonText, 0.45);
        }
        if (control.tone === ActionButton.Primary) {
            return control.palette.highlightedText;
        }
        if (control.tone === ActionButton.Danger) {
            return SettingsColors.dangerText(control.palette);
        }
        return control.palette.buttonText;
    }

    contentItem: Label {
        text: control.text
        font: control.font
        color: control.foregroundColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        maximumLineCount: 1
    }

    background: Rectangle {
        implicitHeight: 32
        radius: 6
        color: control.enabled ? control.fillColor : SettingsColors.alpha(control.palette.button, 0.35)
        border.width: control.visualFocus ? 2 : (control.tone === ActionButton.Tertiary ? 0 : 1)
        border.color: control.visualFocus ? control.palette.highlight : SettingsColors.panelBorder(control.palette)
    }
}

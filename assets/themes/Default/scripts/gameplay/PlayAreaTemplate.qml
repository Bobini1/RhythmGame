import RhythmGameQml
import QtQuick
import "../common/TaoQuickCustom"

Item {
    id: playAreaTemplate
    required property var columns
    readonly property real spacing: ProfileList.currentProfile.vars.themeVars["gameplay"].spacing
    x: ProfileList.currentProfile.vars.themeVars["gameplay"].playAreaX - template.borderMargin
    anchors.topMargin: -template.borderMargin
    height: ProfileList.currentProfile.vars.themeVars["gameplay"].playAreaHeight + template.borderMargin * 2
    width: playAreaTemplate.columns.reduce((a, b) => a + root.columnSizes[b], 0) + (playAreaTemplate.columns.length - 1) * playAreaTemplate.spacing + template.borderMargin * 2
    TemplateDragBorder {
        id: template
        visible: root.customizeMode
        topBorderResizable: false
        minimumWidth: ProfileList.currentProfile.vars.themeVars["gameplay"].spacing * 7 + 1 + borderMargin
        onBorderPressedChanged: {
            if (borderPressed) {
                // remove the binding
                playAreaTemplate.width = playAreaTemplate.width
            } else {
                playAreaTemplate.width = Qt.binding(() => playAreaTemplate.columns.reduce((a, b) => a + root.columnSizes[b], 0) + (playAreaTemplate.columns.length - 1) * playAreaTemplate.spacing + template.borderMargin * 2)
            }
        }

        color: "transparent"
        rotationEnabled: false
    }
    onHeightChanged: {
        ProfileList.currentProfile.vars.themeVars["gameplay"].playAreaHeight = height - template.borderMargin * 2;
        height = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].playAreaHeight + template.borderMargin * 2);
    }
    onWidthChanged: {
        let spacing = ProfileList.currentProfile.vars.themeVars["gameplay"].spacing;
        let oldWithoutSpacing = playAreaTemplate.columns.reduce((a, b) => a + root.columnSizes[b], 0);
        let newWithoutSpacing = width - spacing * 7 - template.borderMargin * 2;
        let newWidths = [];
        for (let i = 5; i < 8; i++) {
            newWidths.push(newWithoutSpacing / oldWithoutSpacing * root.columnSizes[i]);
        }
        ProfileList.currentProfile.vars.themeVars["gameplay"].blackWidth = newWidths[0];
        ProfileList.currentProfile.vars.themeVars["gameplay"].whiteWidth = newWidths[1];
        ProfileList.currentProfile.vars.themeVars["gameplay"].scratchWidth = newWidths[2];

    }
    onXChanged: {
        ProfileList.currentProfile.vars.themeVars["gameplay"].playAreaX = x + template.borderMargin;
        x = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].playAreaX - template.borderMargin);
    }
}
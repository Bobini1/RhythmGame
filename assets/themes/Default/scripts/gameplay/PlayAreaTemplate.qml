import RhythmGameQml
import QtQuick
import "../common/TaoQuickCustom"
import QtQuick.Controls.Basic

Item {
    id: playAreaTemplate

    required property var columns
    readonly property real spacing: root.vars.spacing

    signal clicked(var mouse)
    signal doubleClicked(var mouse)

    height: root.vars.playAreaHeight
    width: playAreaTemplate.columns.reduce((a, b) => a + root.columnSizes[b], 0) + (playAreaTemplate.columns.length - 1) * playAreaTemplate.spacing
    x: root.vars.playAreaX
    y: root.vars.playAreaY

    onHeightChanged: {
        root.vars.playAreaHeight = height;
        height = Qt.binding(() => root.vars.playAreaHeight);
    }
    onWidthChanged: {
        let spacing = root.vars.spacing;
        let oldWithoutSpacing = playAreaTemplate.columns.reduce((a, b) => a + root.columnSizes[b], 0);
        let newWithoutSpacing = width - spacing * 7;
        let newWidths = [];
        for (let i = 5; i < 8; i++) {
            newWidths.push(newWithoutSpacing / oldWithoutSpacing * root.columnSizes[i]);
        }
        root.vars.blackWidth = newWidths[0];
        root.vars.whiteWidth = newWidths[1];
        root.vars.scratchWidth = newWidths[2];
    }
    onXChanged: {
        root.vars.playAreaX = x;
        x = Qt.binding(() => root.vars.playAreaX);
    }
    onYChanged: {
        root.vars.playAreaY = y;
        y = Qt.binding(() => root.vars.playAreaY);
    }

    TemplateDragBorder {
        id: template

        anchors.fill: parent
        anchors.margins: -borderMargin
        color: "transparent"

        onBorderPressedChanged: {
            if (borderPressed) {
                // remove the binding
                // noinspection SillyAssignmentJS
                playAreaTemplate.width = playAreaTemplate.width;
            } else {
                playAreaTemplate.width = Qt.binding(() => playAreaTemplate.columns.reduce((a, b) => a + root.columnSizes[b], 0) + (playAreaTemplate.columns.length - 1) * playAreaTemplate.spacing);
            }
        }
        onClicked: mouse => playAreaTemplate.clicked(mouse)
        onDoubleClicked: mouse => playAreaTemplate.doubleClicked(mouse)
    }
}
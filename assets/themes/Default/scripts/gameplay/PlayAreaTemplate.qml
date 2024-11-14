import RhythmGameQml
import QtQuick
import "../common/TaoQuickCustom"
import QtQuick.Controls.Basic

Item {
    id: playAreaTemplate

    required property var columns
    readonly property real spacing: playAreaTemplate.vars.spacing
    required property var vars
    readonly property list<real> columnSizes: root.getColumnSizes(vars)

    signal clicked(var mouse)
    signal doubleClicked(var mouse)

    height: playAreaTemplate.vars.playAreaHeight
    width: playAreaTemplate.columns.reduce((a, b) => a + playAreaTemplate.columnSizes[b], 0) + (playAreaTemplate.columns.length - 1) * playAreaTemplate.spacing
    x: playAreaTemplate.vars.playAreaX
    y: playAreaTemplate.vars.playAreaY

    onHeightChanged: {
        playAreaTemplate.vars.playAreaHeight = height;
        height = Qt.binding(() => playAreaTemplate.vars.playAreaHeight);
    }
    onWidthChanged: {
        let spacing = playAreaTemplate.vars.spacing;
        let oldWithoutSpacing = playAreaTemplate.columns.reduce((a, b) => a + playAreaTemplate.columnSizes[b], 0);
        let newWithoutSpacing = width - spacing * 7;
        let newWidths = [];
        for (let i = 5; i < 8; i++) {
            newWidths.push(newWithoutSpacing / oldWithoutSpacing * playAreaTemplate.columnSizes[i]);
        }
        playAreaTemplate.vars.blackWidth = newWidths[0];
        playAreaTemplate.vars.whiteWidth = newWidths[1];
        playAreaTemplate.vars.scratchWidth = newWidths[2];
    }
    onXChanged: {
        playAreaTemplate.vars.playAreaX = x;
        x = Qt.binding(() => playAreaTemplate.vars.playAreaX);
    }
    onYChanged: {
        playAreaTemplate.vars.playAreaY = y;
        y = Qt.binding(() => playAreaTemplate.vars.playAreaY);
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
                playAreaTemplate.width = Qt.binding(() => playAreaTemplate.columns.reduce((a, b) => a + playAreaTemplate.columnSizes[b], 0) + (playAreaTemplate.columns.length - 1) * playAreaTemplate.spacing);
            }
        }
        onClicked: mouse => playAreaTemplate.clicked(mouse)
        onDoubleClicked: mouse => playAreaTemplate.doubleClicked(mouse)
    }
}
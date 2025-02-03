import RhythmGameQml
import QtQuick
import "../common/TaoQuickCustom"
import QtQuick.Controls.Basic

Item {
    id: playAreaTemplate

    required property var columns
    readonly property real spacing: playAreaTemplate.vars.spacing
    required property var vars
    required property string varSuffix
    readonly property list<real> columnSizes: root.getColumnSizes(vars)

    signal clicked(var mouse)
    signal doubleClicked(var mouse)

    height: playAreaTemplate.vars.playAreaHeight
    width: playAreaTemplate.columns.reduce((a, b) => a + playAreaTemplate.columnSizes[b], 0) + (playAreaTemplate.columns.length - 1) * playAreaTemplate.spacing
    x: playAreaTemplate.vars["playAreaX" + playAreaTemplate.varSuffix]
    y: playAreaTemplate.vars["playAreaY" + playAreaTemplate.varSuffix]

    onHeightChanged: {
        playAreaTemplate.vars.playAreaHeight = height;
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
        playAreaTemplate.vars["playAreaX" + playAreaTemplate.varSuffix] = x;
    }
    onYChanged: {
        playAreaTemplate.vars["playAreaY" + playAreaTemplate.varSuffix] = y;
    }

    TemplateDragBorder {
        id: template

        anchors.fill: parent
        anchors.margins: -borderMargin
        color: "transparent"

        Binding {
            delayed: true
            playAreaTemplate.width: {
                if (!template.borderPressed) {
                    return playAreaTemplate.columns.reduce((a, b) => a + playAreaTemplate.columnSizes[b], 0) + (playAreaTemplate.columns.length - 1) * playAreaTemplate.spacing;
                }
            }
        }
        Binding {
            delayed: true
            playAreaTemplate.height: playAreaTemplate.vars.playAreaHeight
        }
        onClicked: mouse => playAreaTemplate.clicked(mouse)
        onDoubleClicked: mouse => playAreaTemplate.doubleClicked(mouse)
    }
}
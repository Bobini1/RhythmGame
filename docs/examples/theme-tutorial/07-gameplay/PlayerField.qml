pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml

ColumnLayout {
    id: field
    required property Player player

    // [columns]
    readonly property var columns: {
        const mode = field.player.score.keymode;
        const left = mode === 5 || mode === 10 ? [7, 0, 1, 2, 3, 4]
            : [7, 0, 1, 2, 3, 4, 5, 6];
        if (mode === 10)
            return left.concat([8, 9, 10, 11, 12, 15]);
        if (mode === 14)
            return left.concat([8, 9, 10, 11, 12, 13, 14, 15]);
        return left;
    }
    // [columns]

    Label {
        Layout.fillWidth: true
        text: qsTr("%1: %2 points, %3 combo").arg(field.player.profile.vars.generalVars.name)
            .arg(field.player.score.points).arg(field.player.score.combo)
        elide: Text.ElideRight
    }

    // [visible-range]
    readonly property real visibleBeats: 6
    function updateVisibleRange() {
        field.player.state.setVisiblePositionSpans(field.visibleBeats, field.visibleBeats);
    }
    onPlayerChanged: updateVisibleRange()
    Component.onCompleted: updateVisibleRange()
    // [visible-range]

    Row {
        id: lanes
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 2

        Repeater {
            model: field.columns
            NoteLane {
                required property int modelData
                width: Math.max(0, (lanes.width - (field.columns.length - 1) * lanes.spacing) / field.columns.length)
                height: lanes.height
                player: field.player
                columnState: field.player.state.columnStates[modelData]
                visibleBeats: field.visibleBeats
                scratch: modelData === 7 || modelData === 15
            }
        }
    }
}

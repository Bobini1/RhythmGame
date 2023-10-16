import QtQuick
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt5Compat.GraphicalEffects

Item {
    id: playArea

    property double blackWidth: 48.0 / totalWidthAbs * parent.width
    property list<int> columns: []
    readonly property list<int> columnsReversedMapping: {
        var mapping = [];
        for (var i = 0; i < columns.length; i++) {
            mapping[columns[i]] = i;
        }
        return mapping;
    }
    property double redWidth: 108.0 / totalWidthAbs * parent.width
    readonly property int spacing: 2
    readonly property int totalWidthAbs: 3 * 48 + 4 * 60 + 108 + spacing * 7
    property double whiteWidth: 60.0 / totalWidthAbs * parent.width

    anchors.bottom: parent.bottom
    anchors.top: parent.top
    width: totalWidthAbs

    Playfield {
        id: playfield

        border.color: "black"
        border.width: 10
        columns: playArea.columns
        spacing: playArea.spacing
        y: chart.position * root.greenNumber + parent.height
    }
    Row {
        id: laserRow

        function hideLaser(index) {
            laserRow.children[playArea.columnsReversedMapping[index]].stop();
        }
        function shootLaser(index) {
            laserRow.children[playArea.columnsReversedMapping[index]].start();
        }

        anchors.bottom: parent.bottom
        height: parent.height
        spacing: playArea.spacing
        width: parent.width

        Repeater {
            id: laserRowChildren

            model: playArea.columns.length

            // laser beam (animated)
            LaserBeam {
                columnIndex: playArea.columns[index]
                image: root.laserImages[index]
            }
        }
    }
    Judgements {
        anchors.centerIn: parent
    }
    Connections {
        function onEmptyHit(hit) {
            laserRow.shootLaser(hit.column);
        }
        function onEmptyRelease(release) {
            laserRow.hideLaser(release.column);
        }
        function onLnEndHit(hit) {
            laserRow.hideLaser(hit.column);
        }
        function onLnEndMissed(misses) {
            for (let miss of misses) {
                if (miss.points.deviation > 0.0) {
                    continue;
                }
                laserRow.hideLaser(miss.column);
            }
        }
        function onLnEndSkipped(skips) {
            for (let skip of skips) {
                laserRow.hideLaser(skip.column);
            }
        }
        function onNoteHit(tap) {
            if (playArea.columns.indexOf(tap.column) === -1) {
                return;
            }
            laserRow.shootLaser(tap.column);
            if (tap.noteIndex !== -1) {
                if (tap.removesNote) {
                    playfield.removeNote(tap.column, tap.noteIndex);
                }
            }
        }

        target: chart.score
    }
    Item {
        id: playAreaBg

        anchors.fill: parent
        // this is so that we can blend with glow
        layer.enabled: true
        z: -1

        Repeater {
            id: columnSeparators

            model: playArea.columns.length - 1

            Rectangle {
                anchors.bottom: parent.bottom
                color: "#1e1e1e"
                height: parent.height
                width: playArea.spacing
                x: {
                    let cpos = 0;
                    for (let i = 0; i < index + 1; i++) {
                        cpos += root.columnSizes[playfield.columns[i]];
                    }
                    return cpos + index * playArea.spacing;
                }
                z: -2
            }
        }
        Repeater {
            id: columnBgs

            model: playArea.columns.length

            Rectangle {
                anchors.bottom: parent.bottom
                color: playfield.columns[index] % 2 === 0 ? "#050505" : "#000000"
                height: parent.height
                width: root.columnSizes[playfield.columns[index]]
                x: {
                    let cpos = 0;
                    for (let i = 0; i < index; i++) {
                        cpos += root.columnSizes[playfield.columns[i]];
                    }
                    return cpos + index * playArea.spacing;
                }
                z: -2
            }
        }
    }
    Image {
        id: glow

        anchors.bottom: parent.bottom
        source: root.imagesUrl + "glow.png"
        visible: false
        width: parent.width
        z: -1
    }
    Blend {
        anchors.fill: glow
        foregroundSource: glow
        mode: "addition"
        opacity: (Math.abs(chart.position % 1) > 0.5 ? Math.abs(chart.position % 1) : 1 - Math.abs(chart.position % 1)) * 0.2 + 0.1
        source: playAreaBg
        z: -1
    }
}


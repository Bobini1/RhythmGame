import QtQuick
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
import RhythmGameQml

Item {
    id: playArea

    property list<int> columns: []
    readonly property list<int> columnsReversedMapping: {
        var mapping = [];
        for (var i = 0; i < columns.length; i++) {
            mapping[columns[i]] = i;
        }
        return mapping;
    }
    readonly property int spacing: ProfileList.currentProfile.vars.themeVars["gameplay"].spacing

    anchors.bottom: parent.bottom
    anchors.top: parent.top
    width: columns.reduce((a, b) => a + root.columnSizes[b], 0) + (columns.length - 1) * spacing

    Playfield {
        id: playfield
        width: parent.width

        columns: playArea.columns
        spacing: playArea.spacing
        y: Math.floor(chart.position * root.greenNumber + parent.height)
        z: 1
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
        function onLnEndHit(tap) {
            if (playArea.columns.indexOf(tap.column) === -1) {
                return;
            }
            playfield.markLnEndAsMissed(tap.column, tap.noteIndex);
        }
        function onLnEndMissed(miss) {
            if (playArea.columns.indexOf(miss.column) === -1) {
                return;
            }
            playfield.deactivateLn(miss.column, miss.noteIndex - 1);
            playfield.markLnEndAsMissed(miss.column, miss.noteIndex);
        }
        function onLnEndSkipped(skip) {
            if (playArea.columns.indexOf(skip.column) === -1) {
                return;
            }
            playfield.markLnEndAsMissed(skip.column, skip.noteIndex);
        }
        function onMineHit(hit) {
            if (playArea.columns.indexOf(hit.column) === -1) {
                return;
            }
            playfield.removeNote(hit.column, hit.noteIndex);
        }
        function onMissed(miss) {
            if (playArea.columns.indexOf(miss.column) === -1) {
                return;
            }
            playfield.removeNote(miss.column, miss.noteIndex);
        }
        function onNoteHit(tap) {
            if (playArea.columns.indexOf(tap.column) === -1) {
                return;
            }
            if (tap.points.noteRemoved) {
                playfield.removeNote(tap.column, tap.noteIndex);
                playfield.activateLn(tap.column, tap.noteIndex);
            }
        }
        function onPressed(columnIndex) {
            if (playArea.columns.indexOf(columnIndex) === -1) {
                return;
            }
            laserRow.shootLaser(columnIndex);
        }
        function onReleased(columnIndex) {
            if (playArea.columns.indexOf(columnIndex) === -1) {
                return;
            }
            laserRow.hideLaser(columnIndex);
        }

        target: chart.score
    }
    Item {
        id: playAreaBg

        anchors.fill: parent
        z: -1

        BarLinePositioner {
            barLines: chart.notes.barLines
            heightMultiplier: root.greenNumber
            width: parent.width
            y: Math.floor(chart.position * root.greenNumber + parent.height)
        }
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
        opacity: (Math.abs(chart.position % 1) > 0.5 ? Math.abs(chart.position % 1) : 1 - Math.abs(chart.position % 1)) * 0.2 + 0.1
        source: root.imagesUrl + "glow.png"
        visible: true
        width: parent.width
        z: -1
    }
    Repeater {
        id: explosions

        model: playArea.columns.length

        Item {
            id: bombWrapper

            property bool ln: false

            function restart() {
                bomb.restart();
            }

            anchors.bottom: parent.bottom
            width: root.columnSizes[playfield.columns[index]]
            x: {
                let cpos = 0;
                for (let i = 0; i < index; i++) {
                    cpos += root.columnSizes[playfield.columns[i]];
                }
                return cpos + index * playArea.spacing;
            }
            z: 1

            AnimatedSprite {
                id: bomb

                anchors.centerIn: parent
                finishBehavior: AnimatedSprite.FinishAtFinalFrame
                frameCount: bombWrapper.ln ? 8 : 16
                frameDuration: 25
                frameHeight: 600
                frameWidth: 600
                frameY: bombWrapper.ln ? 0 : 600
                height: 300
                loops: bombWrapper.ln ? AnimatedSprite.Infinite : 1
                running: false
                source: root.imagesUrl + "explosion.png"
                visible: running
                width: 300
            }
        }
    }
    Connections {
        function onLnEndHit(hit) {
            if (playArea.columns.indexOf(hit.column) === -1) {
                return;
            }
            let item = explosions.itemAt(playArea.columnsReversedMapping[hit.column]);
            item.ln = false;
            item.restart();
        }
        function onLnEndMissed(miss) {
            if (playArea.columns.indexOf(miss.column) === -1) {
                return;
            }
            let item = explosions.itemAt(playArea.columnsReversedMapping[miss.column]);
            item.ln = false;
        }
        function onNoteHit(tap) {
            if (playArea.columns.indexOf(tap.column) === -1) {
                return;
            }
            if (tap.points.noteRemoved) {
                let item = explosions.itemAt(playArea.columnsReversedMapping[tap.column]);
                if (root.visibleNotes[tap.column][tap.noteIndex].type === Note.Type.LongNoteBegin) {
                    item.ln = true;
                }
                item.restart();
            }
        }

        target: chart.score
    }
}


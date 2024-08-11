import QtQuick
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
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
    readonly property int spacing: root.vars.spacing

    height: root.vars.playAreaHeight
    width: playfield.width

    Item {
        id: playObjectContainer
        anchors.fill: parent
        clip: true
        layer.enabled: true
        layer.smooth: true

        Image {
            id: laneCover
            source: root.imagesUrl + "lanecover/" + root.vars.lanecover
            visible: ProfileList.currentProfile.vars.globalVars.laneCoverOn
            height: parent.height
            width: parent.width
            y: height * (-1 + ProfileList.currentProfile.vars.globalVars.laneCoverRatio)
            z: 7
        }

        Image {
            id: liftCover
            source: root.imagesUrl + "liftcover/" + root.vars.liftcover
            visible: ProfileList.currentProfile.vars.globalVars.liftOn || ProfileList.currentProfile.vars.globalVars.hiddenOn
            height: parent.height * Math.min(1,
                ProfileList.currentProfile.vars.globalVars.liftOn * ProfileList.currentProfile.vars.globalVars.liftRatio +
                ProfileList.currentProfile.vars.globalVars.hiddenOn * ProfileList.currentProfile.vars.globalVars.hiddenRatio)
            width: parent.width
            fillMode: Image.PreserveAspectCrop
            y: parent.height - height
            z: 6
        }

        Rectangle {
            id: judgeLine
            color: root.vars.judgeLineColor

            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height * ProfileList.currentProfile.vars.globalVars.liftOn * ProfileList.currentProfile.vars.globalVars.liftRatio
            width: parent.width
            height: root.vars.judgeLineThickness
            z: 0
        }

        BarLinePositioner {
            barLines: chart.notes.barLines
            heightMultiplier: root.greenNumber
            width: parent.width
            y: -root.vars.thickness / 2 + chart.position * root.greenNumber + parent.height *
                (1 - ProfileList.currentProfile.vars.globalVars.liftOn * ProfileList.currentProfile.vars.globalVars.liftRatio)
            z: 2
        }

        Playfield {
            id: playfield

            columns: playArea.columns
            spacing: playArea.spacing
            y: -root.vars.thickness / 2 + chart.position * root.greenNumber + parent.height *
                (1 - ProfileList.currentProfile.vars.globalVars.liftOn * ProfileList.currentProfile.vars.globalVars.liftRatio)
            z: 3
        }

        Row {
            id: laserRow

            z: 5

            function hideLaser(index) {
                laserRow.children[playArea.columnsReversedMapping[index]].stop();
            }
            function shootLaser(index) {
                laserRow.children[playArea.columnsReversedMapping[index]].start();
            }

            anchors.bottom: judgeLine.bottom
            height: parent.height
            spacing: playArea.spacing

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

        Image {
            id: glow

            anchors.bottom: judgeLine.bottom
            opacity: (Math.abs(chart.position % 1) > 0.5 ? Math.abs(chart.position % 1) : 1 - Math.abs(chart.position % 1)) * 0.2 + 0.1
            source: root.imagesUrl + "glow/" + root.vars.glow
            width: parent.width
            z: 1
        }

        Item {
            id: noteAnchor

            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height * ProfileList.currentProfile.vars.globalVars.liftOn * ProfileList.currentProfile.vars.globalVars.liftRatio
            width: parent.width
            z: 4
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

        Repeater {
            id: columnSeparators

            model: playArea.columns.length - 1

            Rectangle {
                anchors.bottom: parent.bottom
                color: {
                    let base = Qt.color("#1e1e1e");
                    let mod = root.vars.laneBrightness;
                    if (root.vars.laneBrightness < 0) {
                        mod = base.hsvValue * root.vars.laneBrightness;
                    }
                    base.hsvValue = Math.max(0, Math.min(base.hsvValue + mod, 1));
                    return base;
                }
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
                color: {
                    let base = Qt.color(playfield.columns[index] % 2 === 0 ? "#050505" : "#000000");
                    let mod = root.vars.laneBrightness;
                    if (root.vars.laneBrightness < 0) {
                        mod = Qt.color("#1e1e1e").hsvValue * root.vars.laneBrightness;
                    }
                    base.hslValue = Math.max(0, Math.min(base.hsvValue + mod, 1));
                    return base;
                }
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
            anchors.bottomMargin: parent.height * ProfileList.currentProfile.vars.globalVars.liftOn * ProfileList.currentProfile.vars.globalVars.liftRatio
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
                frameHeight: bombSize.sourceSize.height / 4
                frameWidth: bombSize.sourceSize.width / 4
                frameY: bombWrapper.ln ? 0 : frameHeight
                height: frameHeight / 2
                loops: bombWrapper.ln ? AnimatedSprite.Infinite : 1
                running: false
                source: root.imagesUrl + "bomb/" + root.vars.bomb
                visible: running
                width: frameWidth / 2
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
    // to get the sourceSize of the bomb image
    Image {
        id: bombSize

        source: root.imagesUrl + "bomb/" + root.vars.bomb
        visible: false
    }
}


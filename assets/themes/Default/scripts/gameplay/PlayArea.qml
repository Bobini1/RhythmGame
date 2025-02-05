import QtQuick
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Qt5Compat.GraphicalEffects
import RhythmGameQml

Item {
    id: playArea

    required property list<int> columns
    readonly property list<int> columnsReversedMapping: {
        var mapping = [];
        for (var i = 0; i < columns.length; i++) {
            mapping[columns[i]] = i;
        }
        return mapping;
    }
    readonly property double heightMultiplier: {
        let baseSpeed = ((1 / profile.vars.globalVars.noteScreenTimeMillis) || 0) * 60000 * vars.playAreaHeight / chart.chartData.initialBpm;
        let laneCoverMod = profile.vars.globalVars.laneCoverOn * profile.vars.globalVars.laneCoverRatio;
        let liftMod = profile.vars.globalVars.liftOn * profile.vars.globalVars.liftRatio;
        return baseSpeed * Math.max(0, Math.min(1 - laneCoverMod - liftMod, 1));
    }
    required property Profile profile
    required property var score
    required property var notes
    readonly property int spacing: playArea.vars.spacing
    readonly property var vars: profile.vars.themeVars[chartFocusScope.screen]
    readonly property var globalVars: profile.vars.globalVars
    readonly property list<real> columnSizes: root.getColumnSizes(vars)

    height: playArea.vars.playAreaHeight
    width: playfield.width
    property real position: 0

    Connections {
        function onStarted() {
            notesAnimator.running = true;
        }
        function onOver() {
            notesAnimator.running = false;
        }

        target: chart
    }

    FrameAnimation {
        id: notesAnimator

        running: false
        onTriggered: {
            playArea.position = chart.position;
        }
    }

    Item {
        id: playObjectContainer

        anchors.fill: parent
        clip: true
        layer.enabled: true
        layer.smooth: true

        Image {
            id: laneCover

            height: parent.height
            source: root.imagesUrl + "lanecover/" + playArea.vars.lanecover
            visible: playArea.globalVars.laneCoverOn
            width: parent.width
            y: height * (-1 + playArea.globalVars.laneCoverRatio)
            z: 7
        }
        Image {
            id: liftCover

            fillMode: Image.PreserveAspectCrop
            height: parent.height * Math.min(1, playArea.globalVars.liftOn * playArea.globalVars.liftRatio + playArea.globalVars.hiddenOn * playArea.globalVars.hiddenRatio)
            source: root.imagesUrl + "liftcover/" + playArea.vars.liftcover
            visible: playArea.globalVars.liftOn || playArea.globalVars.hiddenOn
            width: parent.width
            y: parent.height - height
            z: 6
        }
        Rectangle {
            id: judgeLine

            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height * playArea.globalVars.liftOn * playArea.globalVars.liftRatio
            color: playArea.vars.judgeLineColor
            height: playArea.vars.judgeLineThickness
            width: parent.width
            z: 0
        }
        BarLinePositioner {
            // barlines are always the same for all players
            barLines: (chart.notes1 || chart.notes2).barLines
            heightMultiplier: playArea.heightMultiplier
            width: parent.width
            position: playArea.position
            y: -playArea.vars.thickness / 2 + playArea.position * playArea.heightMultiplier + parent.height * (1 - playArea.globalVars.liftOn * playArea.globalVars.liftRatio)
            z: 2
        }
        Playfield {
            id: playfield

            columns: playArea.columns
            spacing: playArea.spacing
            notes: playArea.notes
            noteThickness: playArea.vars.thickness
            heightMultiplier: playArea.heightMultiplier
            columnSizes: playArea.columnSizes
            noteImage: playArea.vars.notes
            mineImage: playArea.vars.mine
            notesStay: playArea.vars.notesStay
            position: playArea.position
            y: -playArea.vars.thickness / 2 + playArea.position * playArea.heightMultiplier + parent.height * (1 - playArea.globalVars.liftOn * playArea.globalVars.liftRatio)
            z: 3
        }
        Row {
            id: laserRow

            function hideLaser(index) {
                laserRow.children[playArea.columnsReversedMapping[index]].stop();
            }
            function shootLaser(index) {
                laserRow.children[playArea.columnsReversedMapping[index]].start();
            }

            anchors.bottom: judgeLine.bottom
            height: parent.height
            spacing: playArea.spacing
            z: 5

            Repeater {
                id: laserRowChildren

                model: playArea.columns

                // laser beam (animated)
                LaserBeam {
                    required property int index
                    required property string modelData
                    columnIndex: modelData
                    columnSizes: playArea.columnSizes
                    image: {
                        if (modelData === 7 || modelData === 15)
                            return root.iniImagesUrl + "keybeam/" + playArea.vars.keybeam + "/laser_s";
                        else if (modelData % 2 === 0)
                            return root.iniImagesUrl + "keybeam/" + playArea.vars.keybeam + "/laser_w";
                        else
                            return root.iniImagesUrl + "keybeam/" + playArea.vars.keybeam + "/laser_b";
                    }
                }
            }
        }
        Image {
            id: glow

            anchors.bottom: judgeLine.bottom
            opacity: (Math.abs(playArea.position % 1) > 0.5 ? Math.abs(playArea.position % 1) : 1 - Math.abs(playArea.position % 1)) * 0.2 + 0.1
            source: root.imagesUrl + "glow/" + playArea.vars.glow
            width: parent.width
            z: 1
        }
        Item {
            id: noteAnchor

            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height * playArea.globalVars.liftOn * playArea.globalVars.liftRatio
            width: parent.width
            z: 4
        }
    }
    Judgements {
        anchors.centerIn: parent

        score: playArea.score
        judge: playArea.vars.judge
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

        target: playArea.score
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
                    let mod = playArea.vars.laneBrightness;
                    if (playArea.vars.laneBrightness < 0) {
                        mod = base.hsvValue * playArea.vars.laneBrightness;
                    }
                    base.hslLightness = Math.max(0, Math.min(base.hslLightness + mod, 1));
                    return base;
                }
                height: parent.height
                width: playArea.spacing
                x: {
                    let cpos = 0;
                    for (let i = 0; i < index + 1; i++) {
                        cpos += playArea.columnSizes[playfield.columns[i]];
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
                    let mod = playArea.vars.laneBrightness;
                    if (playArea.vars.laneBrightness < 0) {
                        mod = Qt.color("#1e1e1e").hslLightness * playArea.vars.laneBrightness;
                    }
                    base.hslLightness = Math.max(0, Math.min(base.hslLightness + mod, 1));
                    return base;
                }
                height: parent.height
                width: playArea.columnSizes[playfield.columns[index]]
                x: {
                    let cpos = 0;
                    for (let i = 0; i < index; i++) {
                        cpos += playArea.columnSizes[playfield.columns[i]];
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
            anchors.bottomMargin: parent.height * playArea.globalVars.liftOn * playArea.globalVars.liftRatio
            width: playArea.columnSizes[playfield.columns[index]]
            x: {
                let cpos = 0;
                for (let i = 0; i < index; i++) {
                    cpos += playArea.columnSizes[playfield.columns[i]];
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
                source: root.imagesUrl + "bomb/" + playArea.vars.bomb
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
                if (playArea.notes[playArea.columnsReversedMapping[tap.column]][tap.noteIndex].type === Note.Type.LongNoteBegin) {
                    item.ln = true;
                }
                item.restart();
            }
        }

        target: playArea.score
    }
    // to get the sourceSize of the bomb image
    Image {
        id: bombSize

        source: root.imagesUrl + "bomb/" + playArea.vars.bomb
        visible: false
    }
}


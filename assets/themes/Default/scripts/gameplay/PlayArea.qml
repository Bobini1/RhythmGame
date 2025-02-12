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
    required property var columnStates
    readonly property int spacing: playArea.vars.spacing
    readonly property var vars: profile.vars.themeVars[chartFocusScope.screen]
    readonly property var globalVars: profile.vars.globalVars
    readonly property list<real> columnSizes: root.getColumnSizes(vars)

    height: playArea.vars.playAreaHeight
    width: playfield.width

    Item {
        id: playObjectContainer

        anchors.fill: parent
        //clip: true
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
            barLines: chart.state1.barLinesState
            heightMultiplier: playArea.heightMultiplier
            width: parent.width
            anchors.bottom: parent.bottom
            y: -playArea.vars.thickness / 2 + chart.position * playArea.heightMultiplier + parent.height * (1 - playArea.globalVars.liftOn * playArea.globalVars.liftRatio)
            z: 2
        }
        Playfield {
            id: playfield

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            columns: playArea.columns
            spacing: playArea.spacing
            notes: playArea.notes
            columnStates: playArea.columnStates
            noteThickness: playArea.vars.thickness
            heightMultiplier: playArea.heightMultiplier
            columnSizes: playArea.columnSizes
            noteImage: playArea.vars.notes
            mineImage: playArea.vars.mine
            notesStay: playArea.vars.notesStay
            z: 4
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
            z: 3

            Repeater {
                id: laserRowChildren

                model: playArea.columnStates

                LaserBeam {
                    required property int index
                    required property var modelData
                    columnIndex: playArea.columns[index]
                    width: playArea.columnSizes[columnIndex]
                    active: modelData.pressed
                    image: {
                        if (columnIndex === 7 || columnIndex === 15)
                            return root.iniImagesUrl + "keybeam/" + playArea.vars.keybeam + "/laser_s";
                        else if (columnIndex % 2 === 0)
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
            opacity: (Math.abs(chart.position % 1) > 0.5 ? Math.abs(chart.position % 1) : 1 - Math.abs(chart.position % 1)) * 0.2 + 0.1
            source: root.imagesUrl + "glow/" + playArea.vars.glow
            width: parent.width
            z: 1
        }
        Item {
            id: noteAnchor

            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height * playArea.globalVars.liftOn * playArea.globalVars.liftRatio
            width: parent.width
            z: 5
        }
    }
    Judgements {
        anchors.centerIn: parent

        score: playArea.score
        judge: playArea.vars.judge
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
    // to get the sourceSize of the bomb image
    Image {
        id: bombSize

        source: root.imagesUrl + "bomb/" + playArea.vars.bomb
        visible: false
    }
}


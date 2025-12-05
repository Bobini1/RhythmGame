pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import RhythmGameQml
import "../common/TaoQuickCustom"

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
        let bpmMode = profile.vars.generalVars.hiSpeedFix;
        const bpm = (() => {
            switch (bpmMode) {
                case HiSpeedFix.Off:
                    return 120;
                case HiSpeedFix.Main:
                    return chartData.mainBpm;
                case HiSpeedFix.Start:
                    return chartData.initialBpm;
                case HiSpeedFix.Min:
                    return chartData.minBpm;
                case HiSpeedFix.Max:
                    return chartData.maxBpm;
                case HiSpeedFix.Avg:
                    return chartData.avgBpm;
                default:
                    console.error("Invalid HiSpeedFix mode: " + bpmMode);
                    return 120;
            }
        })();
        let baseSpeed = ((1 / profile.vars.generalVars.noteScreenTimeMillis) || 0) * 60000 * vars.playAreaHeight / bpm;
        let laneCoverMod = profile.vars.generalVars.laneCoverOn * profile.vars.generalVars.laneCoverRatio;
        let liftMod = profile.vars.generalVars.liftOn * profile.vars.generalVars.liftRatio;
        return baseSpeed * Math.max(0, Math.min(1 - laneCoverMod - liftMod, 1));
    }
    required property Player player
    required property ChartData chartData
    readonly property Profile profile: player.profile
    readonly property var score: player.score
    readonly property var barLinesState: player.state.barLinesState
    readonly property var notes: columns.map(function (column) {
        return side.notes.notes[column];
    })
    readonly property var columnStates: columns.map(function (column) {
        return side.columnStates[column];
    })
    readonly property int spacing: playArea.vars.spacing
    readonly property var vars: profile.vars.themeVars[root.screen][root.themeName]
    readonly property var generalVars: profile.vars.generalVars
    readonly property list<real> columnSizes: root.getColumnSizes(vars)
    property real position
    property var pointTarget
    FrameAnimation {
        running: true
        onTriggered: {
            playArea.position = playArea.player.position;
        }
    }

    Item {
        id: playObjectContainer

        anchors.fill: parent
        layer.enabled: true
        layer.smooth: true

        Image {
            id: laneCover

            height: parent.height
            source: root.imagesUrl + "lanecover/" + playArea.vars.lanecover
            visible: playArea.generalVars.laneCoverOn
            width: parent.width
            y: height * (-1 + playArea.generalVars.laneCoverRatio)
            z: 7
        }
        Image {
            id: liftCover

            fillMode: Image.PreserveAspectCrop
            height: parent.height * Math.min(1, playArea.generalVars.liftOn * playArea.generalVars.liftRatio + playArea.generalVars.hiddenOn * playArea.generalVars.hiddenRatio)
            source: root.imagesUrl + "liftcover/" + playArea.vars.liftcover
            visible: playArea.generalVars.liftOn || playArea.generalVars.hiddenOn
            width: parent.width
            y: parent.height - height
            z: 6
        }
        Rectangle {
            id: judgeLine

            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height * playArea.generalVars.liftOn * playArea.generalVars.liftRatio
            color: playArea.vars.judgeLineColor
            height: playArea.vars.judgeLineThickness
            width: parent.width
            z: 0
        }
        BarLinePositioner {
            model: playArea.barLinesState
            heightMultiplier: playArea.heightMultiplier
            position: playArea.position
            anchors.fill: parent
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
            hideLnEnds: playArea.vars.hideLnEnds
            position: playArea.position
            z: 4
        }
        Row {
            id: laserRow

            anchors.bottom: judgeLine.bottom
            height: parent.height
            spacing: playArea.spacing
            z: 3

            Repeater {
                id: laserRowChildren

                model: playArea.columnStates

                LaserBeam {
                    id: beam
                    required property int index
                    required property var modelData
                    columnIndex: playArea.columns[index]
                    width: playArea.columnSizes[columnIndex]
                    height: parent.height
                    Binding {
                        when: beam.columnIndex !== 7 && beam.columnIndex !== 15
                        beam.active: modelData.pressed
                    }
                    Connections {
                        target: modelData
                        enabled: beam.columnIndex === 7 || beam.columnIndex === 15
                        function onPressedChanged() {
                            beam.active = modelData.pressed;
                            beam.active = false;
                        }
                    }
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
            opacity: {
                let pos = Math.abs(playArea.player.position % 1);
                return (pos > 0.5 ? pos : 1 - pos) * 0.2 + 0.1;
            }
            source: root.imagesUrl + "glow/" + playArea.vars.glow
            width: parent.width
            z: 1
        }
    }
    Judgements {
        id: judgements
        anchors.centerIn: parent
        anchors.verticalCenterOffset: (-playArea.vars.judgementsOffset * 2 + 1) * (parent.height - height) / 2
        height: parent.height * playArea.vars.judgementsHeight

        score: playArea.score
        judge: playArea.vars.judge
        columns: playArea.columns
    }
    Item {
        id: judgementsPositioner
        Binding {
            delayed: true
            judgementsPositioner.x: judgements.x
            judgementsPositioner.width: judgements.width
            judgementsPositioner.height: judgements.height
            judgementsPositioner.y: playArea.height / 2 + judgements.anchors.verticalCenterOffset - judgementsPositioner.height / 2
        }
        z: 11
        Component.onCompleted: {
            x = judgements.x;
            y = judgements.y;
            width = judgements.width;
            height = judgements.height;
        }
        onYChanged: {
            let center = parent.height / 2;
            let offset = y + height / 2 - center;
            playArea.vars.judgementsOffset = -offset / (center - height / 2) / 2 + 0.5;
        }
        onHeightChanged: {
            playArea.vars.judgementsHeight = height / parent.height;
        }

        TemplateDragBorder {
            id: judgementsTemplate

            anchors.fill: parent
            anchors.margins: -borderMargin
            color: "transparent"
            visible: root.customizeMode
            dragAxis: Drag.YAxis
            keepAspectRatio: true

            MouseArea {
                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                z: -1

                onClicked: mouse => {
                    let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                    let popup;
                    if (side.mirrored) {
                        popup = judgementsPopupP2;
                    } else {
                        popup = judgementsPopup;
                    }
                    popup.setPosition(point);
                    popup.open();
                    root.popup = popup;
                }
            }
        }
    }
    Item {
        id: fastslow
        property bool fast: false
        property bool shouldShow: false

        Binding {
            delayed: true
            fastslow.x: judgements.x + judgements.width / 2 + playArea.vars.fastslowX * judgements.height - fastslow.width / 2
            fastslow.y: judgements.y + playArea.vars.fastslowY * judgements.height - fastslow.height
            fastslow.height: playArea.vars.fastslowHeight * judgements.height
        }
        Binding {
            target: fastslow
            property: "width"
            delayed: true
            when: fastslowImage.sourceSize.height > 0
            value: fastslowImage.sourceSize.width * (fastslow.height / fastslowImage.sourceSize.height)
        }
        Component.onCompleted: {
            height = playArea.vars.fastslowHeight * judgements.height;
            x = judgements.x + judgements.width / 2 + playArea.vars.fastslowX * judgements.height - fastslow.width / 2;
            y = judgements.y + playArea.vars.fastslowY * judgements.height - fastslow.height;
            if (fastslowImage.sourceSize.height > 0)
                width = fastslowImage.sourceSize.width * (height / fastslowImage.sourceSize.height);
        }
        z: 13
        onHeightChanged: playArea.vars.fastslowHeight = height / judgements.height;
        onXChanged: playArea.vars.fastslowX = (x + width / 2 - judgements.x - judgements.width / 2) / judgements.height;
        onYChanged: playArea.vars.fastslowY = (y - judgements.y + height) / judgements.height;

        Image {
            id: fastslowImage
            anchors.fill: parent
            source: root.iniImagesUrl + "fastslow/" + playArea.vars.fastslow + (fastslow.fast ? "/fast" : "/slow")
            visible: playArea.vars.fastslowEnabled && fastslow.shouldShow && judgements.visible
        }
        TemplateDragBorder {
            id: fastslowTemplate
            anchors.fill: parent
            anchors.margins: -borderMargin
            color: "transparent"
            visible: root.customizeMode
            keepAspectRatio: true
            anchorXCenter: true
            anchorYBottom: true

            MouseArea {
                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                z: -1

                onClicked: mouse => {
                    let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                    let popup;
                    if (side.mirrored) {
                        popup = fastslowPopupP2;
                    } else {
                        popup = fastslowPopup;
                    }
                    popup.setPosition(point);
                    popup.open();
                    root.popup = popup;
                }
            }
        }
        Connections {
            target: playArea.score
            function onHit(tap) {
                if (!tap.points || !playArea.columns.includes(tap.column))
                    return;
                let fast = tap.points.deviation < 0;
                fastslow.fast = fast;
                switch (tap.points.judgement) {
                case Judgement.Great:
                case Judgement.Good:
                case Judgement.Bad:
                    fastslow.shouldShow = true;
                    break;
                case Judgement.Perfect:
                case Judgement.Poor:
                case Judgement.EmptyPoor:
                default:
                    fastslow.shouldShow = false;
                }
            }
        }
    }
    GhostScore {
        id: ghostScore

        color: {
            if (!playArea.vars.ghostScoreEnabled || !judgements.visible) {
                return "transparent";
            }
            return playArea.score.points >= playArea.pointTarget ? "white" : "red";
        }

        FrameAnimation {
            running: true
            onTriggered: {
                ghostScore.points = playArea.score.points - playArea.pointTarget;
            }
        }

        TextMetrics {
            id: fontInfo
            font.pixelSize: ghostScore.fontInfo.pixelSize
            font.family: ghostScore.fontInfo.family
            text: ghostScore.text
        }

        Binding {
            delayed: true
            ghostScore.height: playArea.vars.ghostScoreHeight * judgements.height
            ghostScore.x: judgements.x + judgements.width / 2 + playArea.vars.ghostScoreX * judgements.height - ghostScore.width / 2
            ghostScore.y: judgements.y + playArea.vars.ghostScoreY * judgements.height - ghostScore.height
        }
        Binding {
            target: ghostScore
            property: "width"
            delayed: true
            when: fontInfo.width > 0
            value: fontInfo.width
        }
        Component.onCompleted: {
            height = playArea.vars.ghostScoreHeight * judgements.height;
            x = judgements.x + judgements.width / 2 + playArea.vars.ghostScoreX * judgements.height - ghostScore.width / 2;
            y = judgements.y + playArea.vars.ghostScoreY * judgements.height - ghostScore.height;
            width = fontInfo.width;
        }
        z: 12

        onHeightChanged: {
            playArea.vars.ghostScoreHeight = height / judgements.height;
        }
        onXChanged: {
            playArea.vars.ghostScoreX = (x + width / 2 - judgements.x - judgements.width / 2) / judgements.height;
        }
        onYChanged: {
            playArea.vars.ghostScoreY = (y - judgements.y + height) / judgements.height;
        }

        TemplateDragBorder {
            id: ghostScoreTemplate
            anchors.fill: parent
            anchors.margins: -borderMargin
            color: "transparent"
            visible: root.customizeMode
            keepAspectRatio: true
            anchorXCenter: true
            anchorYBottom: true

            MouseArea {
                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                z: -1

                onClicked: mouse => {
                    let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                    let popup;
                    if (side.mirrored) {
                        popup = ghostScorePopupP2;
                    } else {
                        popup = ghostScorePopup;
                    }
                    popup.setPosition(point);
                    popup.open();
                    root.popup = popup;
                }
            }
        }
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
                lnBomb.restart();
            }

            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height * playArea.generalVars.liftOn * playArea.generalVars.liftRatio
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
                frameCount: 16
                frameDuration: 25
                frameHeight: bombSize.sourceSize.height / 4
                frameWidth: bombSize.sourceSize.width / 4
                frameY: frameHeight
                height: frameHeight / 2
                loops: 1
                running: false
                source: root.imagesUrl + "bomb/" + playArea.vars.bomb
                opacity: (running && !bombWrapper.ln ? 1 : 0)
                width: frameWidth / 2
            }

            AnimatedSprite {
                id: lnBomb

                anchors.centerIn: parent
                finishBehavior: AnimatedSprite.FinishAtFinalFrame
                frameCount: 8
                frameDuration: 25
                frameHeight: bombSize.sourceSize.height / 4
                frameWidth: bombSize.sourceSize.width / 4
                frameY: 0
                height: frameHeight / 2
                loops: AnimatedSprite.Infinite
                running: false
                source: root.imagesUrl + "bomb/" + playArea.vars.bomb
                opacity: (running && bombWrapper.ln ? 1 : 0)
                width: frameWidth / 2
            }
        }
    }
    Connections {
        target: playArea.score
        function onHit(hit) {
            function handleBomb(index, isLongNote, restart = true) {
                if (index === undefined) return;
                let bomb = explosions.itemAt(index);
                bomb.ln = isLongNote;
                if (restart) bomb.restart();
            }

            if (!playArea.columns.includes(hit.column)) {
                return;
            }

            if (hit.noteRemoved) {
                let index = columnsReversedMapping[hit.column];
                let n = playArea.notes[index][hit.noteIndex];

                if (hit.action === hitEvent.Press) {
                    if (n.type === note.Type.Normal) {
                        handleBomb(index, false);
                    } else if (n.type === note.Type.LongNoteBegin) {
                        handleBomb(index, true);
                    }
                } else if (n.type === note.Type.LongNoteEnd) {
                    handleBomb(index, false, false);
                }
            }
        }
    }
    // to get the sourceSize of the bomb image
    Image {
        id: bombSize

        source: root.imagesUrl + "bomb/" + playArea.vars.bomb
        opacity: 0
    }
    
    Repeater {
        id: notePreloader
        model: ["red", "black", "white"]
        delegate: Repeater {
            id: colors
            property string color: modelData
            model: [`notes/${playArea.vars.notes}/note_`, `notes/${playArea.vars.notes}/ln_start_`, `notes/${playArea.vars.notes}/ln_end_`, `mine/${playArea.vars.mine}/mine_`]
            delegate: Image {
                source: root.iniImagesUrl + modelData + colors.color
                opacity: 0
            }
        }
    }
}

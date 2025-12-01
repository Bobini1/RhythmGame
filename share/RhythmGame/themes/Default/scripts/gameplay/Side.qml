import QtQuick
import QtQml
import RhythmGameQml
import QtQuick.Controls.Basic
import "../common/TaoQuickCustom"

Item {
    id: side
    required property Player player
    required property string dpSuffix
    required property var columns
    property bool mirrored: false
    required property int index

    readonly property Profile profile: player.profile
    readonly property BmsLiveScore score: player.score
    readonly property BmsNotes notes: player.notes
    readonly property var columnStates: player.state.columnStates
    readonly property var profileVars: profile.vars.themeVars[root.screen][root.themeName]

    property bool start: Input[`start${index+1}`] || (dpSuffix && (Input.start1 || Input.start2))
    property bool select: Input[`select${index+1}`] || (dpSuffix && (Input.select1 || Input.select2))

    property bool lastDirectionUp: false
    function modifyGnWn(number, amount) {
        if (amount > 0 && lastDirectionUp || amount < 0 && !lastDirectionUp) {
            return;
        }
        let mult = 1 * amount;
        if (number > 50 || side.index === 0 && Input.col1sUp && Input.col1sDown || side.index === 1 && Input.col2sUp && Input.col2sDown) {
            mult = 10 * amount;
        }
        let vars = side.profile.vars.generalVars;
        if (side.start) {
            vars.noteScreenTimeMillis += vars.noteScreenTimeMillis / 1000 * mult;
        } else if (side.select) {
            if (vars.laneCoverOn) {
                vars.laneCoverRatio += 0.0005 * mult;
            } else if (vars.liftOn) {
                vars.liftRatio += 0.0005 * mult;
            }
        }
    }
    Input.onCol1sUpTicked: (number, type) => {
        if (side.index === 0) side.modifyGnWn(number, -1);
    }
    Input.onCol1sDownTicked: (number, type) => {
        if (side.index === 0) side.modifyGnWn(number, 1);
    }
    Input.onCol2sUpTicked: (number, type) => {
        if (side.index === 1) side.modifyGnWn(number, -1)
    }
    Input.onCol2sDownTicked: (number, type) => {
        if (side.index === 1) side.modifyGnWn(number, 1)
    }
    Input.onCol1sUpPressed: {
        if (side.index === 0) side.lastDirectionUp = true;
    }
    Input.onCol1sUpReleased: {
        if (side.index === 0) side.lastDirectionUp = false;
    }
    Input.onCol1sDownPressed: {
        if (side.index === 0) side.lastDirectionUp = false;
    }
    Input.onCol1sDownReleased: {
        if (side.index === 0) side.lastDirectionUp = true;
    }
    Input.onCol2sUpPressed: {
        if (side.index === 1) side.lastDirectionUp = true;
    }
    Input.onCol2sUpReleased: {
        if (side.index === 1) side.lastDirectionUp = false;
    }
    Input.onCol2sDownPressed: {
        if (side.index === 1) side.lastDirectionUp = false;
    }
    Input.onCol2sDownReleased: {
        if (side.index === 1) side.lastDirectionUp = true;
    }

    transform: Scale {
        xScale: side.mirrored ? -1 : 1; origin.x: side.width / 2
    }
    PlayArea {
        id: playArea

        readonly property list<real> columnSizes: root.getColumnSizes(vars)
        player: side.player
        columns: side.columns
        chartData: root.chartData
        transform: Scale {
            xScale: side.mirrored ? -1 : 1; origin.x: playArea.width / 2
        }
        z: side.profileVars.playAreaZ

        height: side.profileVars.playAreaHeight
        width: side.columns.reduce((a, b) => a + playArea.columnSizes[b], 0) + (side.columns.length - 1) * playArea.vars.spacing
        x: side.profileVars["playAreaX" + side.dpSuffix]
        y: side.profileVars["playAreaY" + side.dpSuffix]

        onHeightChanged: {
            side.profileVars.playAreaHeight = height;
        }
        onWidthChanged: {
            let spacing = side.profileVars.spacing;
            let oldWithoutSpacing = side.columns.reduce((a, b) => a + playArea.columnSizes[b], 0);
            let newWithoutSpacing = width - spacing * 7;
            let newWidths = [];
            for (let i = 5; i < 8; i++) {
                newWidths.push(newWithoutSpacing / oldWithoutSpacing * playArea.columnSizes[i]);
            }
            side.profileVars.blackWidth = newWidths[0];
            side.profileVars.whiteWidth = newWidths[1];
            side.profileVars.scratchWidth = newWidths[2];
        }
        onXChanged: {
            side.profileVars["playAreaX" + side.dpSuffix] = x;
        }
        onYChanged: {
            side.profileVars["playAreaY" + side.dpSuffix] = y;
        }
        Binding {
            delayed: true
            playArea.width: {
                return side.columns.reduce((a, b) => a + playArea.columnSizes[b], 0) + (side.columns.length - 1) * side.profileVars.spacing;
            }
        }
        Binding {
            delayed: true
            playArea.height: side.profileVars.playAreaHeight
        }

        TemplateDragBorder {
            visible: root.customizeMode
            z: 10
            
            anchors.fill: parent
            anchors.margins: -borderMargin
            color: "transparent"

            MouseArea {
                id: playAreaTemplateMouseArea

                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                z: -1

                onClicked: mouse => {
                    let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                    let popup;
                    if (side.mirrored) {
                        popup = playAreaPopupP2;
                    } else {
                        popup = playAreaPopup;
                    }
                    popup.setPosition(point);
                    popup.open();
                    root.popup = popup;
                }
            }
        }

        Row {
            id: gnwnText
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            visible: side.start || side.select
            Text {
                width: Math.max(100, implicitWidth)
                font.pixelSize: 24
                color: "green"
                text: ((side.profile.vars.generalVars.noteScreenTimeMillis) * 3 / 5).toFixed(0)
            }
            Text {
                width: Math.max(100, implicitWidth)
                font.pixelSize: 24
                color: "white"
                visible: side.profile.vars.generalVars.laneCoverOn || side.profile.vars.generalVars.liftOn
                property real wn: {
                    let laneCoverMod = profile.vars.generalVars.laneCoverOn * profile.vars.generalVars.laneCoverRatio;
                    let liftMod = profile.vars.generalVars.liftOn * profile.vars.generalVars.liftRatio;
                    return Math.max(0, Math.min(1 - laneCoverMod - liftMod, 1));
                }
                text: ((1 - wn) * 1000).toFixed(0)
            }
        }
    }
    LifeBar {
        id: lifeBar

        verticalGauge: side.profileVars.verticalGauge
        gaugeImage: side.profileVars.gauge
        score: side.score
        transform: Scale {
            xScale: side.mirrored ? -1 : 1; origin.x: lifeBar.width / 2
        }

        height: side.profileVars.lifeBarHeight
        width: side.profileVars.lifeBarWidth
        x: side.profileVars.lifeBarX
        y: side.profileVars.lifeBarY
        z: side.profileVars.lifeBarZ

        onHeightChanged: {
            side.profileVars.lifeBarHeight = height;
        }
        onWidthChanged: {
            side.profileVars.lifeBarWidth = width;
        }
        onXChanged: {
            side.profileVars.lifeBarX = x;
        }
        onYChanged: {
            side.profileVars.lifeBarY = y;
        }

        TemplateDragBorder {
            id: lifeBarTemplate

            anchors.fill: parent
            anchors.margins: -borderMargin
            color: "transparent"
            visible: root.customizeMode

            MouseArea {
                id: lifeBarMouseArea

                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                z: -1

                onClicked: mouse => {
                    let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                    let popup;
                    if (side.mirrored) {
                        popup = gaugePopupP2;
                    } else {
                        popup = gaugePopup;
                    }
                    popup.setPosition(point);
                    popup.open();
                    root.popup = popup;
                }
            }
        }
    }
    JudgementCounts {
        id: judgementCountsContainer

        height: side.profileVars.judgementCountsHeight
        width: side.profileVars.judgementCountsWidth
        x: side.profileVars.judgementCountsX
        y: side.profileVars.judgementCountsY
        z: side.profileVars.judgementCountsZ
        transform: Scale {
            xScale: side.mirrored ? -1 : 1; origin.x: judgementCountsContainer.width / 2
        }
        score: side.score

        onHeightChanged: {
            side.profileVars.judgementCountsHeight = height;
        }
        onWidthChanged: {
            side.profileVars.judgementCountsWidth = width;
        }
        onXChanged: {
            side.profileVars.judgementCountsX = x;
        }
        onYChanged: {
            side.profileVars.judgementCountsY = y;
        }

        TemplateDragBorder {
            id: judgementCountsTemplate

            anchors.fill: parent
            anchors.margins: -borderMargin
            color: "transparent"
            visible: root.customizeMode
        }

        MouseArea {
            id: judgementCountsMouseArea

            acceptedButtons: Qt.RightButton
            anchors.fill: parent
            z: -1

            onClicked: mouse => {
                let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                let popup;
                if (side.mirrored) {
                    popup = judgementCountsPopupP2;
                } else {
                    popup = judgementCountsPopup;
                }
                popup.setPosition(point);
                popup.open();
                root.popup = popup;
            }
        }

        Connections {
            function onHit(tap) {
                if (!tap.points) {
                    return;
                }
                let judgement = tap.points.judgement;
                if (judgement === Judgement.Poor || judgement === Judgement.Bad) {
                    bga.poorVisible = true;
                    poorLayerTimer.restart();
                }
            }

            target: side.score
        }
    }
}
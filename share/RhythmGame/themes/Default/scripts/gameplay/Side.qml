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

    function modifyGnWn(number, amount) {
        let mult = 1 * amount;
        if (number > 50) {
            mult = 5 * amount;
        }
        let vars = side.profile.vars.generalVars;
        if (side.start) {
            vars.noteScreenTimeMillis += vars.noteScreenTimeMillis / 1000 * mult;
        } else if (side.select) {
            if (vars.laneCoverOn) {
                vars.laneCoverRatio += 0.0003 * mult;
            } else if (vars.liftOn) {
                vars.liftRatio += 0.0003 * mult;
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

    transform: Scale {
        xScale: side.mirrored ? -1 : 1; origin.x: side.width / 2
    }
    PlayAreaTemplate {
        id: playAreaTemplate

        columns: playArea.columns
        vars: side.profileVars
        dpSuffix: side.dpSuffix
        visible: root.customizeMode
        z: playArea.z + 1

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
    PlayArea {
        id: playArea

        player: side.player
        columns: side.columns
        chartData: root.chartData
        transform: Scale {
            xScale: side.mirrored ? -1 : 1; origin.x: playArea.width / 2
        }
        x: side.profileVars["playAreaX" + side.dpSuffix]
        y: side.profileVars["playAreaY" + side.dpSuffix]
        z: side.profileVars.playAreaZ


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
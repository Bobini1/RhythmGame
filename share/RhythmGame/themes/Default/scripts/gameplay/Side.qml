import QtQuick
import QtQml
import RhythmGameQml
import QtQuick.Controls
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
    readonly property real greenNumber: Math.max(1, profile.vars.generalVars.noteScreenTimeMillis) * 3 / 5
    readonly property real laneCoverNumber: Math.max(0, Math.min(1, profile.vars.generalVars.laneCoverRatio)) * 1000
    readonly property real liftNumber: Math.max(0, Math.min(1, profile.vars.generalVars.liftRatio)) * 1000

    property bool start: Input[`start${index+1}`] || (dpSuffix && (Input.start1 || Input.start2))
    property bool select: Input[`select${index+1}`] || (dpSuffix && (Input.select1 || Input.select2))
    property bool coverChangeLift: true
    property bool startSelectHeld: false
    property var pointTarget
    property bool pointTargetAvailable: true
    property real bestFinalPoints: 0
    property real bestMaxPoints: 0
    property real bestPoints: 0
    property real targetFinalPoints: 0

    readonly property int laneCoverDoubleClickInterval: 500
    property double lastStartPressTime: -1

    property bool lastDirectionUp: false
    function handleStartPress(time) {
        if (side.lastStartPressTime >= 0
                && time >= side.lastStartPressTime
                && time - side.lastStartPressTime <= side.laneCoverDoubleClickInterval) {
            side.lastStartPressTime = -1;
            let vars = side.profile.vars.generalVars;
            let visibleRatio = Math.max(0.001, 1 - Math.max(0, Math.min(1, vars.laneCoverRatio)));
            let currentDuration = vars.noteScreenTimeMillis > 0 ? vars.noteScreenTimeMillis : 1000;
            vars.noteScreenTimeMillis = Math.max(1, Math.min(10000,
                vars.laneCoverOn ? currentDuration / visibleRatio : currentDuration * visibleRatio));
            vars.laneCoverOn = !vars.laneCoverOn;
        } else {
            side.lastStartPressTime = time;
        }
    }
    function updateCoverChangeTarget() {
        let held = side.start && side.select;
        if (held && !side.startSelectHeld) {
            side.coverChangeLift = !side.coverChangeLift;
        }
        side.startSelectHeld = held;
    }
    onStartChanged: updateCoverChangeTarget()
    onSelectChanged: updateCoverChangeTarget()
    function modifyCoverValue(value) {
        let vars = side.profile.vars.generalVars;
        if (vars.laneCoverOn || (!vars.liftOn && !vars.hiddenOn)) {
            vars.laneCoverRatio = Math.max(0, Math.min(1, vars.laneCoverRatio + value));
        } else if (vars.liftOn && (!vars.hiddenOn || side.coverChangeLift)) {
            vars.liftRatio = Math.max(0, Math.min(1, vars.liftRatio + value));
        } else {
            vars.hiddenRatio = Math.max(0, Math.min(1, vars.hiddenRatio + value));
        }
    }
    function modifyGnWn(number, amount) {
        if (amount > 0 && lastDirectionUp || amount < 0 && !lastDirectionUp) {
            return;
        }
        let mult = 1 * amount;
        if (number > 50 || side.index === 0 && Input.col1sUp && Input.col1sDown || side.index === 1 && Input.col2sUp && Input.col2sDown) {
            mult = 10 * amount;
        }
        let vars = side.profile.vars.generalVars;
        if (side.start && side.select) {
            return;
        } else if (side.start) {
            vars.noteScreenTimeMillis += vars.noteScreenTimeMillis / 1000 * mult;
        } else if (side.select) {
            side.modifyCoverValue(0.0005 * mult);
        }
    }
    Input.onStart1Pressed: time => {
        if (side.index === 0) side.handleStartPress(time);
    }
    Input.onStart2Pressed: time => {
        if (side.index === 1) side.handleStartPress(time);
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

    component CoverNumbers: Row {
        required property real greenNumber
        required property real whiteNumber

        Text {
            width: Math.max(100, implicitWidth)
            font.pixelSize: 32
            color: "green"
            style: Text.Outline
            styleColor: "black"
            text: Math.round(parent.greenNumber).toString()
        }
        Text {
            width: Math.max(100, implicitWidth)
            font.pixelSize: 32
            color: "white"
            style: Text.Outline
            styleColor: "black"
            text: Math.round(parent.whiteNumber).toString()
        }
    }

    PlayArea {
        id: playArea

        readonly property list<real> columnSizes: root.getColumnSizes(vars)
        player: side.player
        columns: side.columns
        chartData: root.chartData
        pointTarget: side.pointTarget
        pointTargetAvailable: side.pointTargetAvailable
        transform: Scale {
            xScale: side.mirrored ? -1 : 1; origin.x: playArea.width / 2
        }
        z: side.profileVars.playAreaZ

        height: side.profileVars.playAreaHeight
        width: side.columns.reduce((a, b) => a + playArea.columnSizes[b], 0) + (side.columns.length - 1) * playArea.vars.spacing
        x: side.profileVars["playAreaX" + side.dpSuffix]
        y: side.profileVars["playAreaY" + side.dpSuffix]

        TemplateDragBorder {
            visible: root.customizeMode
            z: 10
            mirrored: side.mirrored
            anchors.fill: parent
            anchors.margins: -borderMargin
            color: "transparent"

            onGeometryEdited: geometry => {
                if (geometry.width !== playArea.width) {
                    let oldLaneWidth = side.columns.reduce((sum, column) => sum + playArea.columnSizes[column], 0);
                    let newLaneWidth = geometry.width - (side.columns.length - 1) * side.profileVars.spacing;
                    if (oldLaneWidth <= 0 || newLaneWidth <= 0)
                        return;
                    let ratio = newLaneWidth / oldLaneWidth;
                    let widths = [side.profileVars.blackWidth, side.profileVars.whiteWidth, side.profileVars.scratchWidth].map(width => width * ratio);
                    side.profileVars.blackWidth = widths[0];
                    side.profileVars.whiteWidth = widths[1];
                    side.profileVars.scratchWidth = widths[2];
                }
                side.profileVars["playAreaX" + side.dpSuffix] = geometry.x;
                side.profileVars["playAreaY" + side.dpSuffix] = geometry.y;
                side.profileVars.playAreaHeight = geometry.height;
            }

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

        CoverNumbers {
            id: laneCoverNumbers

            anchors.horizontalCenter: parent.horizontalCenter
            visible: side.start || side.select
            greenNumber: side.greenNumber
            whiteNumber: side.laneCoverNumber
            y: Math.max(0, Math.min(parent.height - height,
                parent.height * playArea.laneCoverHeightRatio - height))
        }

        CoverNumbers {
            id: liftNumbers

            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height * playArea.activeLiftRatio
            anchors.horizontalCenter: parent.horizontalCenter
            visible: (side.start || side.select) && side.profile.vars.generalVars.liftOn
            greenNumber: side.greenNumber
            whiteNumber: side.liftNumber
        }
    }
    LifeBar {
        id: lifeBar

        verticalGauge: side.profileVars.verticalGauge
        gaugeImage: side.profileVars.gauge
        score: side.score
        fontFile: side.profileVars.lifeNumberFont
        contentVisible: side.profileVars.lifeBarEnabled
        transform: Scale {
            xScale: side.mirrored ? -1 : 1; origin.x: lifeBar.width / 2
        }

        height: side.profileVars.lifeBarHeight
        width: side.profileVars.lifeBarWidth
        x: side.profileVars.lifeBarX
        y: side.profileVars.lifeBarY
        z: side.profileVars.lifeBarZ

        TemplateDragBorder {
            id: lifeBarTemplate

            anchors.fill: parent
            anchors.margins: -borderMargin
            color: "transparent"
            visible: root.customizeMode
            mirrored: side.mirrored

            onGeometryEdited: geometry => {
                side.profileVars.lifeBarX = geometry.x;
                side.profileVars.lifeBarY = geometry.y;
                side.profileVars.lifeBarWidth = geometry.width;
                side.profileVars.lifeBarHeight = geometry.height;
            }

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
    ScoreGraph {
        id: sideScoreGraph

        x: side.profileVars.scoreGraphX
        y: side.profileVars.scoreGraphY
        width: side.profileVars.scoreGraphWidth
        height: side.profileVars.scoreGraphHeight
        z: side.profileVars.scoreGraphZ
        transform: Scale {
            xScale: side.mirrored ? -1 : 1; origin.x: sideScoreGraph.width / 2
        }

        contentVisible: side.profile.vars.generalVars.scoreGraphEnabled
        graphBackground: side.profileVars.scoregraph || ""
        barWidthRatio: side.profileVars.scoreGraphBarWidth
        fontFile: side.profileVars.scoreGraphFont

        maxPoints: side.score.maxPoints
        currentPoints: side.score.points
        bestFinalPoints: side.bestFinalPoints
        bestMaxPoints: side.bestMaxPoints
        bestPoints: side.bestPoints
        targetPoints: side.pointTarget
        targetFinalPoints: side.targetFinalPoints

        TemplateDragBorder {
            anchors.fill: parent
            anchors.margins: -borderMargin
            color: "transparent"
            visible: root.customizeMode
            mirrored: side.mirrored

            onGeometryEdited: geometry => {
                side.profileVars.scoreGraphX = geometry.x;
                side.profileVars.scoreGraphY = geometry.y;
                side.profileVars.scoreGraphWidth = geometry.width;
                side.profileVars.scoreGraphHeight = geometry.height;
            }
        }

        MouseArea {
            acceptedButtons: Qt.RightButton
            anchors.fill: parent
            z: -1
            enabled: root.customizeMode

            onClicked: mouse => {
                let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                let popup = side.mirrored ? scoreGraphPopupP2 : scoreGraphPopup;
                popup.setPosition(point);
                popup.open();
                root.popup = popup;
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
        fontFile: side.profileVars.judgementCountsFont

        TemplateDragBorder {
            id: judgementCountsTemplate

            anchors.fill: parent
            anchors.margins: -borderMargin
            color: "transparent"
            visible: root.customizeMode
            mirrored: side.mirrored

            onGeometryEdited: geometry => {
                side.profileVars.judgementCountsX = geometry.x;
                side.profileVars.judgementCountsY = geometry.y;
                side.profileVars.judgementCountsWidth = geometry.width;
                side.profileVars.judgementCountsHeight = geometry.height;
            }
        }

        MouseArea {
            id: judgementCountsMouseArea

            acceptedButtons: Qt.RightButton
            anchors.fill: parent
            z: -1
            enabled: root.customizeMode

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

    // ── Hit Distribution ────────────────────────────────────────────────────
    // Only shown for SP and battle modes; DP uses a single centred component
    // in Gameplay.qml instead.
    HitDistribution {
        id: hitDistributionItem

        // Suppress in DP (dpSuffix is "" for SP/battle, "1"/"2" for DP).
        contentVisible: side.dpSuffix === "" && side.profileVars.hitDistributionEnabled
        visible: side.dpSuffix === ""

        x: side.profileVars.hitDistributionX
        y: side.profileVars.hitDistributionY
        width: side.profileVars.hitDistributionWidth
        height: side.profileVars.hitDistributionHeight
        z: side.profileVars.hitDistributionZ

        transform: Scale {
            xScale: side.mirrored ? -1 : 1
            origin.x: hitDistributionItem.width / 2
        }

        ewmaMode: side.profileVars.hitDistributionEwmaMode
        ewmaAlpha: side.profileVars.hitDistributionEwmaAlpha
        maxTrail: side.profileVars.hitDistributionMaxTrail
        vertical: side.profileVars.hitDistributionVertical
        lineColor: side.profileVars.hitDistributionLineColor
        centerLineColor: side.profileVars.hitDistributionCenterLineColor
        lineWidth: side.profileVars.hitDistributionLineWidth
        centerLineWidth: side.profileVars.hitDistributionCenterLineWidth
        backgroundOpacity: side.profileVars.hitDistributionBackgroundOpacity

        timingWindows: root.chartData.timingWindows
        score: side.score

        TemplateDragBorder {
            anchors.fill: parent
            anchors.margins: -borderMargin
            color: "transparent"
            visible: root.customizeMode
            mirrored: side.mirrored

            onGeometryEdited: geometry => {
                side.profileVars.hitDistributionX = geometry.x;
                side.profileVars.hitDistributionY = geometry.y;
                side.profileVars.hitDistributionWidth = geometry.width;
                side.profileVars.hitDistributionHeight = geometry.height;
            }
        }

        MouseArea {
            acceptedButtons: Qt.RightButton
            anchors.fill: parent
            z: -1
            enabled: root.customizeMode

            onClicked: mouse => {
                let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                let popup = side.mirrored ? hitDistributionPopupP2 : hitDistributionPopup;
                popup.setPosition(point);
                popup.open();
                root.popup = popup;
            }
        }
    }
}

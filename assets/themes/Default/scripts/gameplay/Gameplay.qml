import QtQuick
import QtQuick.Window
import QtQml
import QtQuick.Layouts
import RhythmGameQml
import QtQuick.Controls.Basic
import QtMultimedia
import "../common/TaoQuickCustom"
import "../common/helpers.js" as Helpers

Rectangle {
    id: root

    property bool customizeMode: false
    readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    readonly property Profile mainProfile: Rg.profileList.mainProfile
    readonly property var mainProfileVars: mainProfile.vars.themeVars[screen][themeName]
    property string rootUrl: QmlUtils.fileName.slice(0, QmlUtils.fileName.lastIndexOf("/") + 1)
    property ChartData chartData: chart instanceof ChartRunner ? chart.chartData : chart.chartDatas[chart.currentChartIndex];
    readonly property string screen: {
        let keys = chart.keymode;
        let battle = chart.player1 && chart.player2;
        return "k" + keys + (battle ? "battle" : "");
    }
    property var popup: null
    readonly property bool isDp: screen === "k14"
    readonly property bool isBattle: screen === "k7battle"
    readonly property bool isCourse: chart instanceof CourseRunner
    property var chart
    readonly property string themeName: QmlUtils.themeName

    property bool showedCourseResult: false
    StackView.onActivated: {
        escapeShortcut.nothingWasHit = true;
        if (chart.status === ChartRunner.Finished) {
            if (isCourse && !showedCourseResult) {
                showedCourseResult = true;
                let profiles = [chart.player1.profile, chart.player2 ? chart.player2.profile : null];
                Qt.callLater(() => globalRoot.openCourseResult(chart.finish(), profiles, chart.chartDatas, chart.course));
            } else {
                Qt.callLater(() => sceneStack.pop());
            }
        } else {
            chart.start();
        }
    }

    function getColumnSizes(vars) {
        let sizes = [];
        for (let i = 0; i < 16; i++) {
            if (i === 7 || i === 15) {
                sizes.push(vars.scratchWidth);
            } else if (i % 2 === 0)
                sizes.push(vars.whiteWidth);
            else {
                sizes.push(vars.blackWidth);
            }
        }
        return sizes;
    }

    color: "black"

    // stops all sounds when leaving the screen
    Component.onDestruction: {
        chart.destroy();
    }

    Timer {
        id: poorLayerTimer

        interval: 400

        onTriggered: {
            bga.poorVisible = false;
        }
    }
    Connections {
        function onStatusChanged() {
            if (root.chart.status === ChartRunner.Ready || root.chart.status === ChartRunner.Running) {
                bga.clearOutput();
                chart.bga.layers[0].videoSink = bga.baseSink;
                chart.bga.layers[1].videoSink = bga.layerSink;
                chart.bga.layers[2].videoSink = bga.layer2Sink;
                chart.bga.layers[3].videoSink = bga.poorSink;
            } else if (root.chart.status === ChartRunner.Finished) {
                bga.clearOutput();
                if (root.popup !== null) {
                    root.popup.close();
                    root.popup = null;
                }
                if (escapeShortcut.used) {
                    return;
                }
                chart.bga.layers[0].videoSink = bga.baseSink;
                chart.bga.layers[1].videoSink = bga.layerSink;
                chart.bga.layers[2].videoSink = bga.layer2Sink;
                chart.bga.layers[3].videoSink = bga.poorSink;
                let chartData = root.chartData;
                let profile1 = chart.player1.profile;
                let profile2 = chart.player2 ? chart.player2.profile : null;
                let scores = chart instanceof ChartRunner ? chart.finish() : chart.proceed();
                globalRoot.openResult(scores, [profile1, profile2], chartData);
            }
        }

        target: chart
    }
    PlayAreaPopup {
        id: playAreaPopup

        property Profile profile: chart.player1.profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]
        generalVars: profile.vars.generalVars
        dp: root.isDp

        onClosed: {
            root.popup = null;
        }
    }
    PlayAreaPopup {
        id: playAreaPopupP2

        property Profile profile: (chart.player2 || chart.player1).profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]
        generalVars: profile.vars.generalVars
        dp: root.isDp

        onClosed: {
            root.popup = null;
        }
    }
    GaugePopup {
        id: gaugePopup

        property Profile profile: chart.player1.profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    GaugePopup {
        id: gaugePopupP2

        property Profile profile: (chart.player2 || chart.player1).profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }

    Item {
        id: scaledRoot

        anchors.centerIn: parent
        width: 1920
        height: 1080
        scale: Math.min(globalRoot.width / 1920, globalRoot.height / 1080)
        transformOrigin: Item.Center

        Text {
            color: "white"
            text: "fps: " + frameAnimation.fps.toFixed(0)
            anchors.top: parent.top
            anchors.right: parent.right
        }

        FrameAnimation {
            id: frameAnimation
            property real fps: smoothFrameTime > 0 ? (1.0 / smoothFrameTime) : 0
            running: true
        }

        BgaRenderer {
            id: bga

            readonly property Profile profile: chart.player2 ? Rg.profileList.mainProfile : chart.player1.profile
            readonly property var profileVars: profile.vars.themeVars[root.screen][root.themeName]

            height: profileVars.bgaSize
            visible: profile.vars.generalVars.bgaOn
            width: profileVars.bgaSize
            x: profileVars.bgaX
            y: profileVars.bgaY
            z: profileVars.bgaZ

            onHeightChanged: {
                profileVars.bgaSize = height;
            }
            onXChanged: {
                profileVars.bgaX = x;
            }
            onYChanged: {
                profileVars.bgaY = y;
            }

            TemplateDragBorder {
                id: bgaTemplate

                anchors.fill: parent
                anchors.margins: -borderMargin
                color: "transparent"
                keepAspectRatio: true
                visible: root.customizeMode
            }
        }

        Side {
            anchors.fill: parent
            player: chart.player1
            dpSuffix: root.isDp ? "1" : ""
            index: 0
            columns: root.isDp || !profileVars.scratchOnRightSide ? [7, 0, 1, 2, 3, 4, 5, 6] : [0, 1, 2, 3, 4, 5, 6, 7]
        }
        Loader {
            id: p2SideLoader
            active: chart.player2 !== null || root.isDp
            anchors.fill: parent
            sourceComponent: Side {
                id: side2
                player: root.isDp ? chart.player1 : chart.player2
                dpSuffix: root.isDp ? "2" : ""
                mirrored: !root.isDp
                index: 1
                columns: {
                    if (root.isDp) {
                        return [8, 9, 10, 11, 12, 13, 14, 15];
                    } else {
                        return profileVars.scratchOnRightSide ? [0, 1, 2, 3, 4, 5, 6, 7] : [7, 0, 1, 2, 3, 4, 5, 6];
                    }
                }
            }
        }
        component Side : Item {
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

            property bool startAndUp: start && Input[`col${index+1}sUp`]
            property bool startAndDown: start && Input[`col${index+1}sDown`]
            property bool selectAndUp: select && Input[`col${index+1}sUp`] && !startAndUp
            property bool selectAndDown: select && Input[`col${index+1}sDown`] && !startAndDown

            Timer {
                id: gnWnTimer
                interval: 2
                repeat: true
                running: side.startAndDown || side.selectAndDown || side.selectAndUp || side.startAndUp
                property int count: 0

                onTriggered: {
                    let mult = 1;
                    if (count++ > 250) {
                        mult = 5;
                    }
                    let vars = side.profile.vars.generalVars;
                    if (side.startAndDown) {
                        vars.noteScreenTimeMillis -= vars.noteScreenTimeMillis / 1000 * mult;
                    } else if (side.startAndUp) {
                        vars.noteScreenTimeMillis += vars.noteScreenTimeMillis / 1000 * mult;
                    } else if (side.selectAndDown) {
                        if (vars.laneCoverOn) {
                            vars.laneCoverRatio -= 0.0003 * mult;
                        } else if (vars.liftOn) {
                            vars.liftRatio -= 0.0003 * mult;
                        }
                    } else if (side.selectAndUp) {
                        if (vars.laneCoverOn) {
                            vars.laneCoverRatio += 0.0003 * mult;
                        } else if (vars.liftOn) {
                            vars.liftRatio += 0.0003 * mult;
                        }
                    }
                }
                onRunningChanged: {
                    count = 0;
                }
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
            Rectangle {
                id: judgementCountsContainer

                color: "darkslategray"
                height: side.profileVars.judgementCountsHeight
                width: side.profileVars.judgementCountsWidth
                x: side.profileVars.judgementCountsX
                y: side.profileVars.judgementCountsY
                z: side.profileVars.judgementCountsZ
                transform: Scale {
                    xScale: side.mirrored ? -1 : 1; origin.x: judgementCountsContainer.width / 2
                }

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
                Repeater {
                    id: judgementCountsModel

                    model: side.score.judgementCounts
                    delegate: Item {
                        required property var judgement
                        required property int count
                    }
                }
                Column {
                    Repeater {
                        id: judgementCounts

                        anchors.fill: parent
                        anchors.margins: 8

                        model: side.score.judgementCounts.rowCount()
                        readonly property var judgements: ["Empty Poor", "Poor", "Bad", "Good", "Great", "Perfect"]

                        delegate: Text {
                            required property int index
                            color: "white"
                            fontSizeMode: Text.Fit
                            textFormat: Text.PlainText
                            text: {
                                let row = (judgementCountsModel.count, judgementCountsModel.itemAt(side.score.judgementCounts.rowCount() - index - 1));
                                // ignore judgements like MineHit
                                if (!row || row.judgement > Judgement.Perfect) {
                                    return "";
                                }
                                return judgementCounts.judgements[row.judgement] + ": " + row.count;
                            }
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
            }
        }
    }
    Connections {
        target: chart.player1.score
        function onHit(tap) {
            let ignoreJudgements = [Judgement.Poor, Judgement.EmptyPoor, Judgement.MineHit, Judgement.MineAvoided];
            if (!tap.points || ignoreJudgements.includes(tap.points?.judgement)) {
                return;
            }
            escapeShortcut.nothingWasHit = false;
        }
    }
    Connections {
        target: chart.player2?.score || null
        ignoreUnknownSignals: true
        function onHit(tap) {
            let ignoreJudgements = [Judgement.Poor, Judgement.EmptyPoor, Judgement.MineHit, Judgement.MineAvoided];
            if (!tap.points || ignoreJudgements.includes(tap.points?.judgement)) {
                return;
            }
            escapeShortcut.nothingWasHit = false;
        }
    }
    Shortcut {
        id: escapeShortcut
        enabled: root.enabled
        sequence: "Esc"
        property bool nothingWasHit: true
        property bool used: false

        onActivated: {
            if (nothingWasHit) {
                sceneStack.pop();
            } else {
                used = true;
                let chartData = root.chartData;
                let profile1 = chart.player1.profile;
                let profile2 = chart.player2 ? chart.player2.profile : null;
                let scores = chart instanceof ChartRunner ? chart.finish() : chart.proceed();
                globalRoot.openResult(scores, [profile1, profile2], chartData);
            }
        }
    }
    Shortcut {
        sequence: "F2"
        enabled: root.enabled

        onActivated: {
            root.customizeMode = !root.customizeMode;
            if (root.popup !== null) {
                root.popup.close();
                root.popup = null;
            }
        }
    }
}
import QtQuick
import QtQuick.Window
import QtQml
import QtQuick.Layouts
import RhythmGameQml
import QtQuick.Controls.Basic
import QtMultimedia
import "../common/TaoQuickCustom"
import "../common/helpers.js" as Helpers
import "popups"

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

        readonly property Profile profile: chart.player1.profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]
        generalVars: profile.vars.generalVars
        dp: root.isDp

        onClosed: {
            root.popup = null;
        }
    }
    PlayAreaPopup {
        id: playAreaPopupP2

        readonly property Profile profile: (chart.player2 || chart.player1).profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]
        generalVars: profile.vars.generalVars
        dp: root.isDp

        onClosed: {
            root.popup = null;
        }
    }
    GaugePopup {
        id: gaugePopup

        readonly property Profile profile: chart.player1.profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    GaugePopup {
        id: gaugePopupP2

        readonly property Profile profile: (chart.player2 || chart.player1).profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    JudgementCountsPopup {
        id: judgementCountsPopup

        readonly property Profile profile: chart.player1.profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    JudgementCountsPopup {
        id: judgementCountsPopupP2

        readonly property Profile profile: (chart.player2 || chart.player1).profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    BgaPopup {
        id: bgaPopup

        readonly property Profile profile: Rg.profileList.mainProfile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]
        generalVars: profile.vars.generalVars

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

            MouseArea {
                id: bgaMouseArea

                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                z: -1

                onClicked: mouse => {
                    let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                    bgaPopup.setPosition(point);
                    bgaPopup.open();
                    root.popup = bgaPopup;
                }
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
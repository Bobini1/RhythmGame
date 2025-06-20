import QtQuick
import QtQuick.Window
import QtQml
import QtQuick.Layouts
import RhythmGameQml
import QtQuick.Controls.Basic
import QtMultimedia
import Qt5Compat.GraphicalEffects
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
    readonly property string screen: {
        let keys = chart.chartData.keymode;
        let battle = chart.player1 && chart.player2;
        return "k" + keys + (battle ? "battle" : "");
    }
    property var popup: null
    readonly property bool isDp: screen === "k14"
    readonly property bool isBattle: screen === "k7battle"
    property Chart chart
    readonly property string themeName: QmlUtils.themeName

    property bool completed: false
    StackView.onActivated: {
        if (completed) {
            Qt.callLater(() => sceneStack.pop());
        } else {
            completed = true;
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

    // destroy chart when this component is unloaded
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
            if (chart.status === Chart.Ready) {
                chart.bga.layers[0].videoSink = bga.baseSink;
                chart.bga.layers[1].videoSink = bga.layerSink;
                chart.bga.layers[2].videoSink = bga.layer2Sink;
                chart.bga.layers[3].videoSink = bga.poorSink;
                chart.start();
            } else if (chart.status === Chart.Finished) {
                if (root.popup !== null) {
                    root.popup.close();
                    root.popup = null;
                }
                globalRoot.openResult(chart.finish(), [chart.player1.profile, chart.player2?.profile], chart.chartData);
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

            readonly property Profile profile: player.profile
            readonly property BmsLiveScore score: player.score
            readonly property BmsNotes notes: player.notes
            readonly property var columnStates: player.state.columnStates
            readonly property var profileVars: profile.vars.themeVars[root.screen][root.themeName]

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
                        let point = mapToGlobal(mouse.x, mouse.y);
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
                transform: Scale {
                    xScale: side.mirrored ? -1 : 1; origin.x: playArea.width / 2
                }
                x: side.profileVars["playAreaX" + side.dpSuffix]
                y: side.profileVars["playAreaY" + side.dpSuffix]
                z: side.profileVars.playAreaZ
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
                            let point = mapToGlobal(mouse.x, mouse.y);
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
                Column {
                    Repeater {
                        id: judgementCounts

                        anchors.fill: parent
                        anchors.margins: 8

                        model: side.score.judgementCounts
                        readonly property var judgements: ["Poor", "Empty Poor", "Bad", "Good", "Great", "Perfect"]

                        delegate: Text {
                            required property var judgement
                            required property int count
                            color: "white"
                            fontSizeMode: Text.Fit
                            textFormat: Text.PlainText
                            text: {
                                // ignore judgements like MineHit
                                if (judgement > Judgement.Perfect) {
                                    return "";
                                }
                                return judgementCounts.judgements[judgement] + ": " + count;
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

        onActivated: {
            if (nothingWasHit) {
                sceneStack.pop();
            } else {
                globalRoot.openResult(chart.finish(), [chart.player1.profile, chart.player2?.profile], chart.chartData);
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
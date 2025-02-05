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
    readonly property Profile mainProfile: ProfileList.mainProfile
    readonly property var mainProfileVars: mainProfile.vars.themeVars[chartFocusScope.screen]
    property var popup: null
    property string rootUrl: globalRoot.urlToPath(Qt.resolvedUrl(".").toString())
    property bool screen: chartFocusScope.screen
    property bool isDp: chartFocusScope.screen === "k14"
    property bool isBattle: chartFocusScope.screen === "k7battle"

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
        function onLoaded() {
            chart.bga.layers[0].videoSink = bga.baseSink;
            chart.bga.layers[1].videoSink = bga.layerSink;
            chart.bga.layers[2].videoSink = bga.layer2Sink;
            chart.bga.layers[3].videoSink = bga.poorSink;
            chart.start();
        }

        function onOver() {
            if (root.popup !== null) {
                root.popup.close();
                root.popup = null;
            }
            globalRoot.openResult(chart.finish(), chart.chartData);
        }

        target: chart
    }
    PlayAreaPopup {
        id: playAreaPopup

        property Profile profile: chart.profile1
        themeVars: profile.vars.themeVars[chartFocusScope.screen]
        globalVars: profile.vars.globalVars
        dp: root.isDp

        onClosed: {
            root.popup = null;
        }
    }
    PlayAreaPopup {
        id: playAreaPopupP2

        property Profile profile: chart.profile2 || chart.profile1
        themeVars: profile.vars.themeVars[chartFocusScope.screen]
        globalVars: profile.vars.globalVars

        onClosed: {
            root.popup = null;
        }
    }
    GaugePopup {
        id: gaugePopup

        property Profile profile: chart.profile1
        themeVars: profile.vars.themeVars[chartFocusScope.screen]

        onClosed: {
            root.popup = null;
        }
    }
    GaugePopup {
        id: gaugePopupP2

        property Profile profile: chart.profile2 || chart.profile1
        themeVars: profile.vars.themeVars[chartFocusScope.screen]

        onClosed: {
            root.popup = null;
        }
    }
    Item {
        id: scaledRoot

        anchors.horizontalCenter: parent.horizontalCenter
        height: 1080
        scale: Math.min(globalRoot.width / 1920, globalRoot.height / 1080)
        width: 1920

        BgaRenderer {
            id: bga

            readonly property Profile profile: chart.profile2 ? ProfileList.mainProfile : chart.profile1
            readonly property var profileVars: profile.vars.themeVars[chartFocusScope.screen]

            height: profileVars.bgaSize
            visible: profile.vars.globalVars.bgaOn
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
            profile: chart.profile1
            score: chart.score1
            notes: chart.notes1
            dpSuffix: root.isDp ? "1" : ""
            columns: root.isDp || !profileVars.scratchOnRightSide ? [7, 0, 1, 2, 3, 4, 5, 6] : [0, 1, 2, 3, 4, 5, 6, 7]
        }
        Loader {
            id: p2SideLoader
            active: chart.profile2 !== null || root.isDp
            anchors.fill: parent
            sourceComponent: Side {
                id: side2
                profile: root.isDp ? chart.profile1 : chart.profile2
                score: root.isDp ? chart.score1 : chart.score2
                notes: root.isDp ? chart.notes1 : chart.notes2
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
            required property Profile profile
            required property BmsScore score
            required property BmsNotes notes
            required property string dpSuffix
            required property var columns
            property bool mirrored: false
            readonly property var profileVars: profile.vars.themeVars[chartFocusScope.screen]
            transform: Scale{ xScale: side.mirrored ? -1 : 1; origin.x: side.width / 2 }
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

                columns: side.columns
                profile: side.profile
                score: side.score
                notes: columns.map(function (column) {
                    return side.notes.visibleNotes[column];
                })
                transform: Scale{ xScale: side.mirrored ? -1 : 1; origin.x: playArea.width / 2 }
                x: side.profileVars["playAreaX" + side.dpSuffix]
                y: side.profileVars["playAreaY" + side.dpSuffix]
                z: side.profileVars.playAreaZ
            }
            LifeBar {
                id: lifeBar

                verticalGauge: side.profileVars.verticalGauge
                gaugeImage: side.profileVars.gauge
                score: side.score
                transform: Scale{ xScale: side.mirrored ? -1 : 1; origin.x: lifeBar.width / 2 }

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
                transform: Scale{ xScale: side.mirrored ? -1 : 1; origin.x: judgementCountsContainer.width / 2 }

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
                Text {
                    id: judgementCounts

                    property int bad: 0
                    property int emptyPoor: 0
                    property int good: 0
                    property int great: 0
                    property int perfect: 0
                    property int poor: 0

                    anchors.fill: parent
                    anchors.margins: 8
                    color: "white"
                    font.pixelSize: 300
                    fontSizeMode: Text.Fit
                    renderType: Text.NativeRendering
                    text: {
                        let txt = "";
                        for (let judgement of ["perfect", "great", "good", "bad", "poor", "emptyPoor"]) {
                            txt += Helpers.capitalizeFirstLetter(judgement) + ": " + judgementCounts[judgement] + "\n";
                        }
                        return txt;
                    }
                    textFormat: Text.PlainText
                }
                Connections {
                    function onMissed() {
                        judgementCounts.poor++;
                        bga.poorVisible = true;
                        poorLayerTimer.restart();
                    }

                    function onNoteHit(tap) {
                        switch (tap.points.judgement) {
                            case Judgement.Perfect:
                                judgementCounts.perfect++;
                                break;
                            case Judgement.Great:
                                judgementCounts.great++;
                                break;
                            case Judgement.Good:
                                judgementCounts.good++;
                                break;
                            case Judgement.Bad:
                                judgementCounts.bad++;
                                bga.poorVisible = true;
                                poorLayerTimer.restart();
                                break;
                            case Judgement.EmptyPoor:
                                judgementCounts.emptyPoor++;
                                bga.poorVisible = true;
                                poorLayerTimer.restart();
                                break;
                        }
                    }

                    target: side.score
                }
            }
        }
    }
    Shortcut {
        enabled: root.enabled
        sequence: "Esc"

        onActivated: {
            globalRoot.openResult(chart.finish(), chart.chartData);
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
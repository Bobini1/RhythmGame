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
    property list<string> laserImages: {
        let images = [];
        for (let i = 0; i < 16; i++) {
            if (i === 7 || i === 15)
                images.push(iniImagesUrl + "keybeam/" + mainProfileVars.keybeam + "/laser_s");
            else if (i % 2 === 0)
                images.push(iniImagesUrl + "keybeam/" + mainProfileVars.keybeam + "/laser_w");
            else
                images.push(iniImagesUrl + "keybeam/" + mainProfileVars.keybeam + "/laser_b");
        }
        return images;
    }
    readonly property Profile mainProfile: ProfileList.mainProfile
    readonly property var mainProfileVars: mainProfile.vars.themeVars.gameplay
    property list<string> noteColors: {
        let images = [];
        for (let i = 0; i < 16; i++) {
            if (i === 7 || i === 15)
                images.push("red");
            else if (i % 2 === 0)
                images.push("white");
            else
                images.push("black");
        }
        return images;
    }
    property double playfieldHeight: 800
    property var popup: null
    property string rootUrl: globalRoot.urlToPath(Qt.resolvedUrl(".").toString())
    // copying visibleNotes to js array is faster than accessing it directly
    readonly property var visibleNotes: chart.notes.visibleNotes

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

        themeVars: ProfileList.mainProfile.vars.themeVars.gameplay
        globalVars: ProfileList.mainProfile.vars.globalVars

        onClosed: {
            root.popup = null;
        }
    }
    GaugePopup {
        id: gaugePopup

        themeVars: ProfileList.mainProfile.vars.themeVars.gameplay

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

        PlayAreaTemplate {
            id: playAreaTemplate

            columns: playArea.columns
            vars: ProfileList.mainProfile.vars.themeVars.gameplay
            visible: root.customizeMode
            z: playArea.z + 1

            MouseArea {
                id: playAreaTemplateMouseArea

                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                z: -1

                onClicked: mouse => {
                    let point = mapToGlobal(mouse.x, mouse.y);
                    playAreaPopup.setPosition(point);
                    playAreaPopup.open();
                    root.popup = playAreaPopup;
                }
            }
        }
        PlayArea {
            id: playArea

            columns: root.mainProfileVars.scratchOnRightSide ? [0, 1, 2, 3, 4, 5, 6, 7] : [7, 0, 1, 2, 3, 4, 5, 6]
            profile: ProfileList.mainProfile
            score: chart.scores[0]
            notes: columns.map(function (column) {
                return chart.notes[0].visibleNotes[column];
            })
            x: root.mainProfileVars.playAreaX
            y: root.mainProfileVars.playAreaY
            z: root.mainProfileVars.playAreaZ
        }
        BgaRenderer {
            id: bga

            height: root.mainProfileVars.bgaSize
            visible: ProfileList.mainProfile.vars.globalVars.bgaOn
            width: root.mainProfileVars.bgaSize
            x: root.mainProfileVars.bgaX
            y: root.mainProfileVars.bgaY
            z: root.mainProfileVars.bgaZ

            onHeightChanged: {
                root.mainProfileVars.bgaSize = height;
                height = Qt.binding(() => root.mainProfileVars.bgaSize);
            }
            onWidthChanged: {
                width = Qt.binding(() => root.mainProfileVars.bgaSize);
            }
            onXChanged: {
                root.mainProfileVars.bgaX = x;
                x = Qt.binding(() => root.mainProfileVars.bgaX);
            }
            onYChanged: {
                root.mainProfileVars.bgaY = y;
                y = Qt.binding(() => root.mainProfileVars.bgaY);
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
        LifeBar {
            id: lifeBar

            verticalGauge: root.mainProfileVars.verticalGauge
            gaugeImage: root.mainProfileVars.gauge
            score: chart.scores[0]

            height: root.mainProfileVars.lifeBarHeight
            width: root.mainProfileVars.lifeBarWidth
            x: root.mainProfileVars.lifeBarX
            y: root.mainProfileVars.lifeBarY
            z: root.mainProfileVars.lifeBarZ

            onHeightChanged: {
                root.mainProfileVars.lifeBarHeight = height;
                height = Qt.binding(() => root.mainProfileVars.lifeBarHeight);
            }
            onWidthChanged: {
                root.mainProfileVars.lifeBarWidth = width;
                width = Qt.binding(() => root.mainProfileVars.lifeBarWidth);
            }
            onXChanged: {
                root.mainProfileVars.lifeBarX = x;
                x = Qt.binding(() => root.mainProfileVars.lifeBarX);
            }
            onYChanged: {
                root.mainProfileVars.lifeBarY = y;
                y = Qt.binding(() => root.mainProfileVars.lifeBarY);
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
                        gaugePopup.setPosition(point);
                        gaugePopup.open();
                        root.popup = gaugePopup;
                    }
                }
            }
        }
        Rectangle {
            id: judgementCountsContainer

            color: "darkslategray"
            height: root.mainProfileVars.judgementCountsHeight
            width: root.mainProfileVars.judgementCountsWidth
            x: root.mainProfileVars.judgementCountsX
            y: root.mainProfileVars.judgementCountsY
            z: root.mainProfileVars.judgementCountsZ

            onHeightChanged: {
                root.mainProfileVars.judgementCountsHeight = height;
                height = Qt.binding(() => root.mainProfileVars.judgementCountsHeight);
            }
            onWidthChanged: {
                root.mainProfileVars.judgementCountsWidth = width;
                width = Qt.binding(() => root.mainProfileVars.judgementCountsWidth);
            }
            onXChanged: {
                root.mainProfileVars.judgementCountsX = x;
                x = Qt.binding(() => root.mainProfileVars.judgementCountsX);
            }
            onYChanged: {
                root.mainProfileVars.judgementCountsY = y;
                y = Qt.binding(() => root.mainProfileVars.judgementCountsY);
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

                target: chart.scores[0]
            }
        }
    }
    Shortcut {
        enabled: chartFocusScope.active
        sequence: "Esc"

        onActivated: {
            globalRoot.openResult(chart.finish(), chart.chartData);
        }
    }
    Shortcut {
        sequence: "F2"

        onActivated: {
            root.customizeMode = !root.customizeMode;
            if (root.popup !== null) {
                root.popup.close();
                root.popup = null;
            }
        }
    }
}
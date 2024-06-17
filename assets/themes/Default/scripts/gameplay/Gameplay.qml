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

    property list<real> columnSizes: {
        let sizes = [];
        for (let i = 0; i < 16; i++) {
            if (i === 7 || i === 15) {
                sizes.push(root.vars.scratchWidth);
            } else if (i % 2 === 0)
                sizes.push(root.vars.whiteWidth);
            else {
                sizes.push(root.vars.blackWidth);
            }
        }
        return sizes;
    }
    property bool customizeMode: false
    property double greenNumber: ProfileList.currentProfile.vars.globalVars.noteScreenTimeSeconds * 400
    readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    readonly property var vars: ProfileList.currentProfile.vars.themeVars.gameplay
    property list<string> laserImages: {
        let images = [];
        for (let i = 0; i < 16; i++) {
            if (i === 7 || i === 15)
                images.push(iniImagesUrl + "keybeam/" + root.vars.keybeam + "/laser_s");
            else if (i % 2 === 0)
                images.push(iniImagesUrl + "keybeam/" + root.vars.keybeam + "/laser_w");
            else
                images.push(iniImagesUrl + "keybeam/" + root.vars.keybeam + "/laser_b");
        }
        return images;
    }
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
    property string rootUrl: globalRoot.urlToPath(Qt.resolvedUrl(".").toString())
    // copying visibleNotes to js array is faster than accessing it directly
    readonly property var visibleNotes: chart.notes.visibleNotes
    property var popup: null

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

        onClosed: {
            root.popup = null;
        }
    }
    GaugePopup {
        id: gaugePopup

        onClosed: {
            root.popup = null;
        }
    }
    Rectangle {
        id: scaledRoot
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        color: "black"
        height: 1080
        scale: Math.min(globalRoot.width / 1920, globalRoot.height / 1080)
        width: 1920

        FpsCounter {
            anchors.right: parent.right
            anchors.top: parent.top
            z: 3
        }
        PlayAreaTemplate {
            id: playAreaTemplate

            anchors.top: parent.top
            columns: playArea.columns
            visible: root.customizeMode
            z: 2

            MouseArea {
                id: playAreaTemplateMouseArea

                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                z: -1

                onClicked: mouse => {
                    let point = mapToGlobal(mouse.x, mouse.y);
                    playAreaPopup.x = point.x;
                    playAreaPopup.y = point.y;
                    playAreaPopup.open();
                    root.popup = playAreaPopup;
                }
            }
        }
        PlayArea {
            id: playArea

            anchors.top: parent.top
            columns: [7, 0, 1, 2, 3, 4, 5, 6]
            x: root.vars.playAreaX
        }
        LifeBar {
            id: lifeBar

            height: root.vars.lifeBarHeight
            width: root.vars.lifeBarWidth
            x: root.vars.lifeBarX
            y: root.vars.lifeBarY
            z: 2

            onHeightChanged: {
                root.vars.lifeBarHeight = height;
                height = Qt.binding(() => root.vars.lifeBarHeight);
            }
            onWidthChanged: {
                root.vars.lifeBarWidth = width;
                width = Qt.binding(() => root.vars.lifeBarWidth);
            }
            onXChanged: {
                root.vars.lifeBarX = x;
                x = Qt.binding(() => root.vars.lifeBarX);
            }
            onYChanged: {
                root.vars.lifeBarY = y;
                y = Qt.binding(() => root.vars.lifeBarY);
            }

            TemplateDragBorder {
                id: lifeBarTemplate

                anchors.fill: parent
                anchors.margins: -borderMargin
                color: "transparent"
                rotationEnabled: false
                visible: root.customizeMode

                MouseArea {
                    id: lifeBarMouseArea

                    acceptedButtons: Qt.RightButton
                    anchors.fill: parent
                    z: -1

                    onClicked: mouse => {
                        let point = mapToGlobal(mouse.x, mouse.y);
                        gaugePopup.x = point.x;
                        gaugePopup.y = point.y;
                        gaugePopup.open();
                        root.popup = gaugePopup;
                    }
                }
            }
        }
        Rectangle {
            id: judgementCountsContainer

            color: "darkslategray"
            height: root.vars.judgementCountsHeight
            width: root.vars.judgementCountsWidth
            x: root.vars.judgementCountsX
            y: root.vars.judgementCountsY
            z: 2

            onHeightChanged: {
                root.vars.judgementCountsHeight = height;
                height = Qt.binding(() => root.vars.judgementCountsHeight);
            }
            onWidthChanged: {
                root.vars.judgementCountsWidth = width;
                width = Qt.binding(() => root.vars.judgementCountsWidth);
            }
            onXChanged: {
                root.vars.judgementCountsX = x;
                x = Qt.binding(() => root.vars.judgementCountsX);
            }
            onYChanged: {
                root.vars.judgementCountsY = y;
                y = Qt.binding(() => root.vars.judgementCountsY);
            }

            TemplateDragBorder {
                id: judgementCountsTemplate

                anchors.fill: parent
                anchors.margins: -borderMargin
                color: "transparent"
                rotationEnabled: false
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

                target: chart.score
            }
        }
        BgaRenderer {
            id: bga

            height: root.vars.bgaSize
            visible: ProfileList.currentProfile.vars.globalVars.bgaOn
            width: root.vars.bgaSize
            x: root.vars.bgaX
            y: root.vars.bgaY

            onHeightChanged: {
                root.vars.bgaSize = height;
                height = Qt.binding(() => root.vars.bgaSize);
            }
            onWidthChanged: {
                width = Qt.binding(() => root.vars.bgaSize);
            }
            onXChanged: {
                root.vars.bgaX = x;
                x = Qt.binding(() => root.vars.bgaX);
            }
            onYChanged: {
                root.vars.bgaY = y;
                y = Qt.binding(() => root.vars.bgaY);
            }

            TemplateDragBorder {
                id: bgaTemplate

                anchors.fill: parent
                anchors.margins: -borderMargin
                color: "transparent"
                keepAspectRatio: true
                rotationEnabled: false
                visible: root.customizeMode
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
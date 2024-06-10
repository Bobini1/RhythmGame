import QtQuick
import QtQuick.Window
import QtQml
import QtQuick.Layouts
import RhythmGameQml
import QtQuick.Controls.Basic
import QtMultimedia
import Qt5Compat.GraphicalEffects
import "../common/TaoQuickCustom"

Rectangle {
    id: root

    property bool customizeMode: false
    property list<real> columnSizes: {
        let sizes = [];
        for (let i = 0; i < 16; i++) {
            if (i === 7 || i === 15) {
                sizes.push(ProfileList.currentProfile.vars.themeVars["gameplay"].scratchWidth);
            } else if (i % 2 === 0)
                sizes.push(ProfileList.currentProfile.vars.themeVars["gameplay"].whiteWidth);
            else {
                sizes.push(ProfileList.currentProfile.vars.themeVars["gameplay"].blackWidth);
            }
        }
        return sizes;
    }
    property double greenNumber: ProfileList.currentProfile.vars.globalVars.noteScreenTimeSeconds * 400
    readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    property list<string> laserImages: {
        let images = [];
        for (let i = 0; i < 16; i++) {
            if (i === 7 || i === 15)
                images.push(iniImagesUrl + "laser.png/laser_s");
            else if (i % 2 === 0)
                images.push(iniImagesUrl + "laser.png/laser_w");
            else
                images.push(iniImagesUrl + "laser.png/laser_b");
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
            globalRoot.openResult(chart.finish(), chart.chartData);
        }

        target: chart
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
            z: 2
            columns: playArea.columns
        }
        PlayArea {
            id: playArea

            anchors.top: parent.top
            columns: [7, 0, 1, 2, 3, 4, 5, 6]
            x: ProfileList.currentProfile.vars.themeVars["gameplay"].playAreaX
        }
        Row {
            anchors.horizontalCenter: playArea.horizontalCenter
            anchors.top: playArea.bottom
            anchors.topMargin: 8

            Gauge {
                id: gauge

            }
            Item {
                id: lifeNumberContainer

            }
            LifeNumber {
                anchors.verticalCenter: gauge.verticalCenter
                width: 80
            }
        }
        Rectangle {
            id: judgementCountsContainer

            anchors.bottom: playArea.bottom
            anchors.left: playArea.right
            color: "darkslategray"
            height: childrenRect.height
            width: 120

            Column {
                anchors.left: parent.left
                anchors.leftMargin: 8

                Repeater {
                    id: judgementCounts

                    model: ["Perfect", "Great", "Good", "Bad", "Poor", "EmptyPoor"]

                    delegate: Text {
                        property int num: 0

                        color: "white"
                        font.pixelSize: 16
                        text: modelData + ": " + num
                        textFormat: Text.PlainText
                    }
                }
            }
            Connections {
                function onMissed() {
                    judgementCounts.itemAt(4).num++;
                    bga.poorVisible = true;
                    poorLayerTimer.restart();
                }
                function onNoteHit(tap) {
                    switch (tap.points.judgement) {
                    case Judgement.Perfect:
                        judgementCounts.itemAt(0).num++;
                        break;
                    case Judgement.Great:
                        judgementCounts.itemAt(1).num++;
                        break;
                    case Judgement.Good:
                        judgementCounts.itemAt(2).num++;
                        break;
                    case Judgement.Bad:
                        judgementCounts.itemAt(3).num++;
                        bga.poorVisible = true;
                        poorLayerTimer.restart();
                        break;
                    case Judgement.EmptyPoor:
                        judgementCounts.itemAt(5).num++;
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
            visible: ProfileList.currentProfile.vars.globalVars.bgaOn

            x: ProfileList.currentProfile.vars.themeVars["gameplay"].bgaX
            y: ProfileList.currentProfile.vars.themeVars["gameplay"].bgaY
            height: ProfileList.currentProfile.vars.themeVars["gameplay"].bgaSize
            width: ProfileList.currentProfile.vars.themeVars["gameplay"].bgaSize
            onHeightChanged: {
                ProfileList.currentProfile.vars.themeVars["gameplay"].bgaSize = height;
                height = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].bgaSize);
            }
            onWidthChanged: {
                width = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].bgaSize);
            }
            onXChanged: {
                ProfileList.currentProfile.vars.themeVars["gameplay"].bgaX = x;
                x = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].bgaX);
            }
            onYChanged: {
                ProfileList.currentProfile.vars.themeVars["gameplay"].bgaY = y;
                y = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].bgaY);
            }

            TemplateDragBorder {
                id: bgaTemplate
                visible: root.customizeMode
                keepAspectRatio: true
                color: "transparent"

                anchors.margins: -borderMargin
                anchors.fill: parent
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
        enabled: chartFocusScope.active
        sequence: "F2"

        onActivated: {
            root.customizeMode = !root.customizeMode;
        }
    }
}
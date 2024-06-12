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
                sizes.push(ProfileList.currentProfile.vars.themeVars["gameplay"].scratchWidth);
            } else if (i % 2 === 0)
                sizes.push(ProfileList.currentProfile.vars.themeVars["gameplay"].whiteWidth);
            else {
                sizes.push(ProfileList.currentProfile.vars.themeVars["gameplay"].blackWidth);
            }
        }
        return sizes;
    }
    property bool customizeMode: false
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
            columns: playArea.columns
            z: 2
        }
        PlayArea {
            id: playArea

            anchors.top: parent.top
            columns: [7, 0, 1, 2, 3, 4, 5, 6]
            x: ProfileList.currentProfile.vars.themeVars["gameplay"].playAreaX
        }
        LifeBar {
            id: lifeBar

            height: ProfileList.currentProfile.vars.themeVars["gameplay"].lifeBarHeight
            width: ProfileList.currentProfile.vars.themeVars["gameplay"].lifeBarWidth
            x: ProfileList.currentProfile.vars.themeVars["gameplay"].lifeBarX
            y: ProfileList.currentProfile.vars.themeVars["gameplay"].lifeBarY
            z: 2

            onHeightChanged: {
                ProfileList.currentProfile.vars.themeVars["gameplay"].lifeBarHeight = height;
                height = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].lifeBarHeight);
            }
            onWidthChanged: {
                ProfileList.currentProfile.vars.themeVars["gameplay"].lifeBarWidth = width;
                width = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].lifeBarWidth);
            }
            onXChanged: {
                ProfileList.currentProfile.vars.themeVars["gameplay"].lifeBarX = x;
                x = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].lifeBarX);
            }
            onYChanged: {
                ProfileList.currentProfile.vars.themeVars["gameplay"].lifeBarY = y;
                y = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].lifeBarY);
            }

            TemplateDragBorder {
                id: lifeBarTemplate

                anchors.fill: parent
                anchors.margins: -borderMargin
                color: "transparent"
                visible: root.customizeMode
                rotationEnabled: false
            }
        }
        Rectangle {
            id: judgementCountsContainer

            color: "darkslategray"
            height: ProfileList.currentProfile.vars.themeVars["gameplay"].judgementCountsHeight
            width: ProfileList.currentProfile.vars.themeVars["gameplay"].judgementCountsWidth
            x: ProfileList.currentProfile.vars.themeVars["gameplay"].judgementCountsX
            y: ProfileList.currentProfile.vars.themeVars["gameplay"].judgementCountsY
            z: 2

            onHeightChanged: {
                ProfileList.currentProfile.vars.themeVars["gameplay"].judgementCountsHeight = height;
                height = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].judgementCountsHeight);
            }
            onWidthChanged: {
                ProfileList.currentProfile.vars.themeVars["gameplay"].judgementCountsWidth = width;
                width = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].judgementCountsWidth);
            }
            onXChanged: {
                ProfileList.currentProfile.vars.themeVars["gameplay"].judgementCountsX = x;
                x = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].judgementCountsX);
            }
            onYChanged: {
                ProfileList.currentProfile.vars.themeVars["gameplay"].judgementCountsY = y;
                y = Qt.binding(() => ProfileList.currentProfile.vars.themeVars["gameplay"].judgementCountsY);
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
                property int perfect: 0
                property int great: 0
                property int good: 0
                property int bad: 0
                property int poor: 0
                property int emptyPoor: 0


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

            height: ProfileList.currentProfile.vars.themeVars["gameplay"].bgaSize
            visible: ProfileList.currentProfile.vars.globalVars.bgaOn
            width: ProfileList.currentProfile.vars.themeVars["gameplay"].bgaSize
            x: ProfileList.currentProfile.vars.themeVars["gameplay"].bgaX
            y: ProfileList.currentProfile.vars.themeVars["gameplay"].bgaY

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
        enabled: chartFocusScope.active
        sequence: "F2"

        onActivated: {
            root.customizeMode = !root.customizeMode;
        }
    }
}
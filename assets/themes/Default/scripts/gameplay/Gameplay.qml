import QtQuick
import QtQuick.Window
import QtQml
import QtQuick.Layouts
import RhythmGameQml
import QtQuick.Controls.Basic
import QtMultimedia
import Qt5Compat.GraphicalEffects

Rectangle {
    id: root

    property list<int> columnSizes: {
        let sizes = [];
        for (let i = 0; i < 16; i++) {
            if (i == 7 || i == 15) {
                sizes.push(ProfileList.currentProfile.vars.themeVars[screen].scratchColumnWidth);
            } else if (i % 2 == 0)
                sizes.push(ProfileList.currentProfile.vars.themeVars[screen].whiteColumnWidth);
            else {
                sizes.push(ProfileList.currentProfile.vars.themeVars[screen].blackColumnWidth);
            }
        }
        return sizes;
    }
    property double greenNumber: 400
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

        anchors.centerIn: parent
        color: "black"
        height: 1080
        scale: Math.min(globalRoot.width / 1920, globalRoot.height / 1080)
        width: 1920

        FpsCounter {
            anchors.right: parent.right
            anchors.top: parent.top
        }
        Rectangle {
            id: playAreaBorder

            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height - height
            color: "white"
            height: root.playfieldHeight + 2
            width: playArea.width + 2
            z: 1

            PlayArea {
                id: playArea

                anchors.bottomMargin: 1
                anchors.left: parent.left
                anchors.leftMargin: 1
                columns: [7, 0, 1, 2, 3, 4, 5, 6]
                height: root.playfieldHeight
            }
        }
        Row {
            anchors.horizontalCenter: playAreaBorder.horizontalCenter
            anchors.top: playAreaBorder.bottom

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

            anchors.bottom: playAreaBorder.bottom
            anchors.left: playAreaBorder.right
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

            anchors.left: judgementCountsContainer.right
            anchors.top: parent.top
            height: 800
            width: 800
        }
    }
    Shortcut {
        enabled: chartFocusScope.active
        sequence: "Esc"

        onActivated: {
            globalRoot.openResult(chart.finish(), chart.chartData);
        }
    }
}
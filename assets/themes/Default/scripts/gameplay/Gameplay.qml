import QtQuick
import QtQuick.Window
import QtQml
import QtQuick.Layouts
import RhythmGameQml
import QtQuick.Controls.Basic
import QtMultimedia

Rectangle {
    id: root

    property list<int> columnSizes: {
        let sizes = [];
        for (let i = 0; i < 16; i++) {
            if (i == 7 || i == 15) {
                sizes.push(108);
            } else if (i % 2 == 0)
                sizes.push(60);
            else {
                sizes.push(48);
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
    property list<string> noteImages: {
        let images = [];
        for (let i = 0; i < 16; i++) {
            if (i === 7 || i === 15)
                images.push(iniImagesUrl + "default.png/note_red");
            else if (i % 2 === 0)
                images.push(iniImagesUrl + "default.png/note_white");
            else
                images.push(iniImagesUrl + "default.png/note_black");
        }
        return images;
    }
    property double playfieldHeight: 800
    property string rootUrl: globalRoot.urlToPath(Qt.resolvedUrl(".").toString())

    color: "black"

    // destroy chart when this component is unloaded
    Component.onDestruction: {
        chart.destroy();
        gc();
    }

    Timer {
        id: poorLayerTimer

        interval: 400

        onTriggered: {
            poorLayer.visible = false;
        }
    }
    Connections {
        function onLoaded() {
            chart.bga.layers[0].videoSink = videoOutput.videoSink;
            chart.bga.layers[1].videoSink = videoLayer.videoSink;
            chart.bga.layers[2].videoSink = videoLayer2.videoSink;
            chart.bga.layers[3].videoSink = poorLayer.videoSink;
            chart.start();
        }
        function onOver() {
            globalRoot.openResult(chart.finish());
        }

        target: chart
    }
    Rectangle {
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
                function onHit(tap) {
                    if (!tap.points)
                        return;
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
                        poorLayer.visible = true;
                        poorLayerTimer.restart();
                        break;
                    case Judgement.EmptyPoor:
                        judgementCounts.itemAt(5).num++;
                        poorLayer.visible = true;
                        poorLayerTimer.restart();
                        break;
                    }
                }
                function onMissed() {
                    judgementCounts.itemAt(4).num++;
                    poorLayer.visible = true;
                    poorLayerTimer.restart();
                }

                target: chart.score
            }
        }
        VideoOutput {
            id: videoOutput

            anchors.left: judgementCountsContainer.right
            anchors.top: parent.top
            fillMode: VideoOutput.PreserveAspectCrop
            height: 800
            width: 800
        }
        VideoOutput {
            id: videoLayer

            anchors.fill: videoOutput
            fillMode: VideoOutput.PreserveAspectCrop
            z: videoOutput.z + 1
        }
        VideoOutput {
            id: videoLayer2

            anchors.fill: videoOutput
            fillMode: VideoOutput.PreserveAspectCrop
            z: videoLayer.z + 1
        }
        ColorChanger {
            anchors.fill: videoLayer
            from: "black"
            to: "transparent"
            z: videoLayer.z

            source: ShaderEffectSource {
                hideSource: true
                sourceItem: videoLayer
            }
        }
        ColorChanger {
            anchors.fill: videoLayer2
            from: "black"
            to: "transparent"
            z: videoLayer2.z

            source: ShaderEffectSource {
                hideSource: true
                sourceItem: videoLayer
            }
        }
        VideoOutput {
            id: poorLayer

            anchors.fill: videoOutput
            fillMode: VideoOutput.PreserveAspectCrop
            visible: false
            z: videoLayer2.z + 1
        }
    }
    Shortcut {
        enabled: chartFocusScope.active
        sequence: "Esc"

        onActivated: {
            if (chart.score.points === 0) {
                sceneStack.pop();
            } else {
                globalRoot.openResult(chart.finish());
            }
        }
    }
}
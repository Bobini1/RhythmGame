import QtQuick
import QtQuick.Window
import QtQml
import QtQuick.Layouts
import RhythmGameQml

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
    readonly property string imagesUrl: rootUrl + "images/"
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

    anchors.fill: parent
    color: "black"

    Component.onCompleted: {
        chart.start();
    }

    // destroy chart when this component is unloaded
    Component.onDestruction: {
        chart.destroy();
    }

    Connections {
        function onOver() {
            globalRoot.openResult(chart.score);
        }

        target: chart
    }
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
                    text: modelData[0] + ": " + num
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
                    break;
                case Judgement.EmptyPoor:
                    judgementCounts.itemAt(5).num++;
                    break;
                }
            }
            function onMissed() {
                judgementCounts.itemAt(4).num++;
            }

            target: chart.score
        }
    }
}

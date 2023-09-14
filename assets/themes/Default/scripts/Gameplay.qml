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
    readonly property string imagesUrl: rootUrl + "../Images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "../Images/"
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
    property string rootUrl: {
        let thisFile = SceneUrls.gameplaySceneUrl;
        // convert to string
        thisFile = thisFile.toString();
        console.log(thisFile);
        let url = thisFile.substring(0, thisFile.lastIndexOf("/") + 1);
        let rootUrl = url.substring(0, url.lastIndexOf("/") + 1);
        return rootUrl;
    }

    anchors.fill: parent
    color: "black"

    Component.onCompleted: {
        chart.start();
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
                    color: "white"
                    font.pixelSize: 16
                    text: modelData + ": " + ((chart.score.judgementCounts[modelData] !== undefined) ? chart.score.judgementCounts[modelData] : 0)
                }
            }
        }
    }
}

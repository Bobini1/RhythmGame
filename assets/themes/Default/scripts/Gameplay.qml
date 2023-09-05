import QtQuick
import QtQuick.Window
import QtQml
import QtQuick.Layouts

Rectangle {
    id: root

    property double bpm
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
    property url imagesUrl: rootUrl + "../Images/"
    property list<string> laserImages: {
        let images = [];
        for (let i = 0; i < 16; i++) {
            if (i === 7 || i === 15)
                images.push(imagesUrl + "laser.png/laser_s");
            else if (i % 2 === 0)
                images.push(imagesUrl + "laser.png/laser_w");
            else
                images.push(imagesUrl + "laser.png/laser_b");
        }
        return images;
    }
    property list<string> noteImages: {
        let images = [];
        for (let i = 0; i < 16; i++) {
            if (i === 7 || i === 15)
                images.push(imagesUrl + "default.png/note_red");
            else if (i % 2 === 0)
                images.push(imagesUrl + "default.png/note_white");
            else
                images.push(imagesUrl + "default.png/note_black");
        }
        return images;
    }
    property url rootUrl: Qt.resolvedUrl(".").toString().replace("file://", "image://ini/")

    anchors.fill: parent
    border.color: "darkslategray"
    border.width: 10
    color: "black"

    Component.onCompleted: {
        chart.start();
    }

    // FpsCounter {
    //     anchors.right: parent.right
    //     anchors.top: parent.top
    // }
    PlayArea {
        anchors.left: parent.left
        columns: [7, 0, 1, 2, 3, 4, 5, 6]
    }
}

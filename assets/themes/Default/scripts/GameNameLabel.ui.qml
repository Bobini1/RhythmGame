import QtQuick 2.15
import QtQuick.Controls 2.15
import RhythmGameQml

Item {
    id: screen

    height: 1080
    width: 1920

    Image {
        id: image

        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        source: "lataia.jpg"

        Rectangle {
            id: rectangle

            anchors.fill: parent
            color: "#323b7c"
            layer.enabled: false
            z: -1
        }
        Column {
            id: column

            height: 435
            width: 477
            x: 241
            y: 288

            Button {
                id: button

                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 0
                text: qsTr("Button")
            }
            Button {
                id: button1

                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 0
                text: qsTr("Button")
            }
            Button {
                id: button2

                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 0
                text: qsTr("Button")
            }
        }
    }
}

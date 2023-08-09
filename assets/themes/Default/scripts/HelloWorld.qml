import QtQuick
import QtQuick.Window

Window {
    height: 480
    title: qsTr("Hello World")
    visible: true
    width: 320

    Rectangle {
        id: page

        color: "lightgray"
        height: parent.height
        width: parent.width

        Text {
            id: helloText

            anchors.horizontalCenter: page.horizontalCenter
            font.bold: true
            font.pointSize: 24
            text: "Hello world!"
            y: 30
        }
        FpsCounter {
            id: fpsCounter

            // upper right corner
            anchors.right: page.right
            anchors.rightMargin: 4
            anchors.top: page.top
            anchors.topMargin: 4
        }
        Grid {
            id: colorPicker

            anchors.bottom: page.bottom
            anchors.bottomMargin: 4
            columns: 3
            rows: 2
            spacing: 3
            x: 4

            Cell {
                cellColor: "red"

                onClicked: function (cellColor) {
                    helloText.color = cellColor;
                }
            }
            Cell {
                cellColor: "green"

                onClicked: function (cellColor) {
                    helloText.color = cellColor;
                }
            }
            Cell {
                cellColor: "blue"

                onClicked: function (cellColor) {
                    helloText.color = cellColor;
                }
            }
            Cell {
                cellColor: "yellow"

                onClicked: function (cellColor) {
                    helloText.color = cellColor;
                }
            }
            Cell {
                cellColor: "steelblue"

                onClicked: function (cellColor) {
                    helloText.color = cellColor;
                }
            }
            Cell {
                cellColor: "black"

                onClicked: function (cellColor) {
                    helloText.color = cellColor;
                }
            }
        }
    }
}
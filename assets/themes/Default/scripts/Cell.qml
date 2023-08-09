import QtQuick

Item {
    id: container

    property alias cellColor: rectangle.color

    signal clicked(color cellColor)

    height: 25
    width: 40

    Rectangle {
        id: rectangle

        anchors.fill: parent
        border.color: "white"
    }
    MouseArea {
        anchors.fill: parent

        onClicked: container.clicked(container.cellColor)
    }
}
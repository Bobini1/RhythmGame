import QtQuick 2.9
import QtQuick.Controls 2.2
import "../../third_party/TaoQuick"
import "../../third_party/TaoQuick/Qml"
import "../../third_party/TaoQuick/Qml/Misc"

ResizeBorder {
    id: cusBorder

    property color borderColor: CusConfig.controlBorderColor
    readonly property int borderMargin: 6
    property color color: CusConfig.themeColor
    property var controller: parent
    property alias dragEnabled: dragItem.enabled
    property alias dragged: dragItem.drag.active
    property color rotateHandleColor: "lightgreen"
    readonly property int rotateHandleDistance: 25
    property bool rotationEnabled: true

    signal clicked(var mouse)
    signal doubleClicked(var mouse)

    height: parent.height
    width: parent.width
    x: 0
    y: 0

    //big
    Rectangle {
        anchors.fill: parent
        anchors.margins: borderMargin + 1
        border.color: cusBorder.borderColor
        border.width: 1
        color: cusBorder.color
        radius: borderMargin
    }
    //line to rotateHandle and Border
    Rectangle {
        color: rotateHandleColor
        height: rotateHandleDistance
        visible: rotationEnabled
        width: 2

        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: -rotateHandleDistance
        }
    }
    //top
    Rectangle {
        border.color: CusConfig.controlBorderColor
        border.width: 1
        color: CusConfig.backgroundColor
        height: width
        radius: width / 2
        visible: cusBorder.topBorderResizable && !cusBorder.keepAspectRatio
        width: borderMargin * 2

        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
        }
    }
    //bottom
    Rectangle {
        border.color: CusConfig.controlBorderColor
        border.width: 1
        color: CusConfig.backgroundColor
        height: width
        radius: width / 2
        visible: cusBorder.bottomBorderResizable && !cusBorder.keepAspectRatio
        width: borderMargin * 2

        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }
    }
    //left
    Rectangle {
        border.color: CusConfig.controlBorderColor
        border.width: 1
        color: CusConfig.backgroundColor
        height: width
        radius: width / 2
        visible: cusBorder.leftBorderResizable && !cusBorder.keepAspectRatio
        width: borderMargin * 2

        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
        }
    }
    //right
    Rectangle {
        border.color: CusConfig.controlBorderColor
        border.width: 1
        color: CusConfig.backgroundColor
        height: width
        radius: width / 2
        visible: cusBorder.rightBorderResizable && !cusBorder.keepAspectRatio
        width: borderMargin * 2

        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
        }
    }
    //top left
    Rectangle {
        border.color: CusConfig.controlBorderColor
        border.width: 1
        color: CusConfig.backgroundColor
        height: width
        radius: width / 2
        visible: cusBorder.topBorderResizable && cusBorder.leftBorderResizable
        width: borderMargin * 2

        anchors {
            left: parent.left
            top: parent.top
        }
    }
    Rectangle {
        border.color: CusConfig.controlBorderColor
        border.width: 1
        color: CusConfig.backgroundColor
        height: width
        radius: width / 2
        visible: cusBorder.topBorderResizable && cusBorder.rightBorderResizable
        width: borderMargin * 2

        anchors {
            right: parent.right
            top: parent.top
        }
    }
    Rectangle {
        border.color: CusConfig.controlBorderColor
        border.width: 1
        color: CusConfig.backgroundColor
        height: width
        radius: width / 2
        visible: cusBorder.bottomBorderResizable && cusBorder.leftBorderResizable
        width: borderMargin * 2

        anchors {
            bottom: parent.bottom
            left: parent.left
        }
    }
    Rectangle {
        border.color: CusConfig.controlBorderColor
        border.width: 1
        color: CusConfig.backgroundColor
        height: width
        radius: width / 2
        visible: cusBorder.bottomBorderResizable && cusBorder.rightBorderResizable
        width: borderMargin * 2

        anchors {
            bottom: parent.bottom
            right: parent.right
        }
    }
    Rectangle {
        color: rotateHandleColor
        height: width
        radius: width / 2
        visible: rotationEnabled
        width: borderMargin * 2

        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: -rotateHandleDistance
        }
        CusImage {
            id: rotateCursor

            source: "../../third_party/TaoQuick/Images/rotate.png"
            visible: rotateArea.containsMouse | rotateArea.pressed
            x: rotateArea.mouseX - width / 2
            y: rotateArea.mouseY - height / 2
        }
        MouseArea {
            id: rotateArea

            property int lastX: 0

            anchors.centerIn: parent
            height: parent.height * 2
            hoverEnabled: true
            width: parent.width * 2

            onContainsMouseChanged: {
                if (containsMouse) {
                    cursorShape = Qt.BlankCursor;
                } else {
                    cursorShape = Qt.ArrowCursor;
                }
            }
            onPositionChanged: {
                if (pressed) {
                    var t = controller.rotation + (mouseX - lastX) / 5;
                    t = t % 360;
                    controller.rotation = t;
                }
            }
            onPressedChanged: {
                if (containsPress) {
                    lastX = mouseX;
                }
            }
        }
        BasicTooltip {
            id: toolTip

            text: parseInt(controller.rotation) + "°"
            visible: rotateArea.pressed
            x: rotateArea.mouseX + 30
            y: rotateArea.mouseY
        }
    }
    MouseArea {
        id: dragItem

        acceptedButtons: Qt.LeftButton
        anchors.fill: parent
        anchors.margins: borderMargin * 2
        cursorShape: Qt.PointingHandCursor
        drag.target: controller

        onClicked: mouse => cusBorder.clicked(mouse)
        onDoubleClicked: mouse => cusBorder.doubleClicked(mouse)
    }
}

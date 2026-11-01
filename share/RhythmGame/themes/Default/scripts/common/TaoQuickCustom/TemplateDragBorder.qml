import QtQuick

ResizeBorder {
    id: cusBorder

    property color borderColor: "#cbcbcb"
    readonly property int borderMargin: 6
    property color color: "#ec4141"
    property color backgroundColor: "#ffffff"
    property alias dragEnabled: dragItem.enabled
    property alias dragged: dragItem.drag.active
    property color rotateHandleColor: "lightgreen"
    readonly property int rotateHandleDistance: 25
    property alias dragAxis: dragItem.drag.axis

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
    //top
    Rectangle {
        border.color: cusBorder.borderColor
        border.width: 1
        color: cusBorder.backgroundColor
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
        border.color: cusBorder.borderColor
        border.width: 1
        color: cusBorder.backgroundColor
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
        border.color: cusBorder.borderColor
        border.width: 1
        color: cusBorder.backgroundColor
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
        border.color: cusBorder.borderColor
        border.width: 1
        color: cusBorder.backgroundColor
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
        border.color: cusBorder.borderColor
        border.width: 1
        color: cusBorder.backgroundColor
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
        border.color: cusBorder.borderColor
        border.width: 1
        color: cusBorder.backgroundColor
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
        border.color: cusBorder.borderColor
        border.width: 1
        color: cusBorder.backgroundColor
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
        border.color: cusBorder.borderColor
        border.width: 1
        color: cusBorder.backgroundColor
        height: width
        radius: width / 2
        visible: cusBorder.bottomBorderResizable && cusBorder.rightBorderResizable
        width: borderMargin * 2

        anchors {
            bottom: parent.bottom
            right: parent.right
        }
    }
    // Drag a temporary item so MouseArea never replaces the control's bindings.
    Item {
        id: dragPosition
        parent: cusBorder.control.parent
        visible: false

        onXChanged: {
            if (dragItem.drag.active)
                cusBorder.geometryEdited(Qt.rect(x, y, cusBorder.control.width, cusBorder.control.height));
        }
        onYChanged: {
            if (dragItem.drag.active)
                cusBorder.geometryEdited(Qt.rect(x, y, cusBorder.control.width, cusBorder.control.height));
        }
    }
    MouseArea {
        id: dragItem

        acceptedButtons: Qt.LeftButton
        anchors.fill: parent
        anchors.margins: borderMargin * 2
        cursorShape: Qt.PointingHandCursor
        drag.target: dragPosition

        onPressed: {
            dragPosition.x = cusBorder.control.x;
            dragPosition.y = cusBorder.control.y;
        }

        onClicked: mouse => cusBorder.clicked(mouse)
        onDoubleClicked: mouse => cusBorder.doubleClicked(mouse)
    }
}

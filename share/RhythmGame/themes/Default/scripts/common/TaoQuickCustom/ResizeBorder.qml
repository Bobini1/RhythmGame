import QtQuick

Item {
    id: root

    property bool borderPressed: leftTopHandle.pressed || rightTopHandle.pressed || leftBottomHandle.pressed || rightBottomHandle.pressed || posTopItem.pressed || posLeftItem.pressed || posRightItem.pressed || posBottomItem.pressed
    property int borderWidth: 12
    property Item control: parent
    property bool keepAspectRatio: false
    property bool leftBorderResizable: true
    property bool rightBorderResizable: true
    property bool topBorderResizable: true
    property bool bottomBorderResizable: true
    property bool anchorXCenter: false
    property bool anchorYBottom: false
    // When true the item lives in a horizontally-mirrored coordinate space
    // (e.g. inside a Scale{xScale:-1} parent).  The resize math is adjusted
    // so that dragging a visual edge moves that edge, not the opposite one.
    property bool mirrored: false

    // Request an edit; the owner stores it and its geometry bindings stay intact.
    signal geometryEdited(rect geometry)

    function resize(xOffset, yOffset, horizontalEdge, verticalEdge) {
        let oldWidth = control.width;
        let oldHeight = control.height;
        if (oldWidth <= 0 || oldHeight <= 0)
            return;

        let width = oldWidth + horizontalEdge * xOffset * (anchorXCenter ? 2 : 1);
        let height = oldHeight + verticalEdge * yOffset;
        if (keepAspectRatio) {
            let aspectRatio = oldWidth / oldHeight;
            if (anchorYBottom || Math.abs(xOffset) <= Math.abs(yOffset))
                height = width / aspectRatio;
            else
                width = height * aspectRatio;
        }
        if (width <= 0 || height <= 0)
            return;

        let x = control.x;
        if (anchorXCenter)
            x += (oldWidth - width) / 2;
        else if (mirrored ? horizontalEdge > 0 : horizontalEdge < 0)
            x += oldWidth - width;
        let y = control.y;
        if (anchorYBottom || verticalEdge < 0)
            y += oldHeight - height;
        geometryEdited(Qt.rect(x, y, width, height));
    }

    DragItem {
        id: leftTopHandle

        enabled: topBorderResizable && leftBorderResizable
        height: borderWidth
        posType: posLeftTop
        width: borderWidth

        onPosChange: (xOffset, yOffset) => root.resize(xOffset, yOffset, -1, -1)
    }
    DragItem {
        id: rightTopHandle

        enabled: topBorderResizable && rightBorderResizable
        height: borderWidth
        posType: posRightTop
        width: borderWidth
        x: parent.width - width

        onPosChange: (xOffset, yOffset) => root.resize(xOffset, yOffset, 1, -1)
    }
    DragItem {
        id: leftBottomHandle

        enabled: leftBorderResizable && bottomBorderResizable
        height: borderWidth
        posType: posLeftBottom
        width: borderWidth
        y: parent.height - height

        onPosChange: (xOffset, yOffset) => root.resize(xOffset, yOffset, -1, 1)
    }
    DragItem {
        id: rightBottomHandle

        enabled: rightBorderResizable && bottomBorderResizable
        height: borderWidth
        posType: posRightBottom
        width: borderWidth
        x: parent.width - width
        y: parent.height - height

        onPosChange: (xOffset, yOffset) => root.resize(xOffset, yOffset, 1, 1)
    }
    DragItem {
        id: posTopItem

        enabled: topBorderResizable && !root.keepAspectRatio
        height: borderWidth
        posType: posTop
        width: parent.width - leftTopHandle.width - rightTopHandle.width
        x: leftBottomHandle.width

        onPosChange: (xOffset, yOffset) => root.resize(xOffset, yOffset, 0, -1)
    }

    DragItem {
        id: posLeftItem

        enabled: leftBorderResizable && !root.keepAspectRatio
        height: parent.height - leftTopHandle.height - leftBottomHandle.height
        posType: posLeft
        width: borderWidth
        y: leftTopHandle.height

        onPosChange: (xOffset, yOffset) => root.resize(xOffset, yOffset, -1, 0)
    }
    DragItem {
        id: posRightItem

        enabled: rightBorderResizable && !root.keepAspectRatio
        height: parent.height - rightTopHandle.height - rightBottomHandle.height
        posType: posRight
        width: borderWidth
        x: parent.width - width
        y: rightTopHandle.height

        onPosChange: (xOffset, yOffset) => root.resize(xOffset, yOffset, 1, 0)
    }
    DragItem {
        id: posBottomItem

        enabled: bottomBorderResizable && !root.keepAspectRatio
        height: borderWidth
        posType: posBottom
        width: parent.width - leftBottomHandle.width - rightBottomHandle.width
        x: leftBottomHandle.width
        y: parent.height - height

        onPosChange: (xOffset, yOffset) => root.resize(xOffset, yOffset, 0, 1)
    }
}

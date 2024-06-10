import QtQuick 2.9
import QtQuick.Controls 2.2
import "../../third_party/TaoQuick/Qml/Misc"

Item {
    id: root

    property bool borderPressed: leftTopHandle.pressed || rightTopHandle.pressed || leftBottomHandle.pressed || rightBottomHandle.pressed || posTopItem.pressed || posLeftItem.pressed || posRightItem.pressed || posBottomItem.pressed
    property int borderWidth: 12
    //controller 要控制大小的目标，可以是Item，也可以是view，只要提供x、y、width、height等属性的修改
    //默认值为parent
    property var control: parent
    property bool keepAspectRatio: false
    property bool leftBorderResizable: true
    property bool rightBorderResizable: true
    property bool topBorderResizable: true
    property bool bottomBorderResizable: true

    //左上角的拖拽
    DragItem {
        id: leftTopHandle

        enabled: topBorderResizable && leftBorderResizable
        height: borderWidth
        posType: posLeftTop
        width: borderWidth

        onPosChange: function (xOffset, yOffset) {
            if (root.keepAspectRatio) {
                let aspectRatio = control.width / control.height;
                if (Math.abs(xOffset) > Math.abs(yOffset)) {
                    xOffset = yOffset / aspectRatio;
                } else {
                    yOffset = xOffset * aspectRatio;
                }
            }

            //不要简化这个判断条件，化简之后不容易看懂. Qml引擎会自动简化
            if (control.x + xOffset < control.x + control.width)
                control.x += xOffset;
            if (control.y + yOffset < control.y + control.height)
                control.y += yOffset;
            if (control.width - xOffset > 0)
                control.width -= xOffset;
            if (control.height - yOffset > 0)
                control.height -= yOffset;
        }
    }
    //右上角拖拽
    DragItem {
        id: rightTopHandle

        enabled: topBorderResizable && rightBorderResizable
        height: borderWidth
        posType: posRightTop
        width: borderWidth
        x: parent.width - width

        onPosChange: function (xOffset, yOffset) {
            if (root.keepAspectRatio) {
                let aspectRatio = control.width / control.height;
                if (Math.abs(xOffset) > Math.abs(yOffset)) {
                    xOffset = yOffset * aspectRatio * -1;
                } else {
                    yOffset = xOffset / aspectRatio * -1;
                }
            }

            //向左拖动时，xOffset为负数
            if (control.width + xOffset > 0)
                control.width += xOffset;
            if (control.height - yOffset > 0)
                control.height -= yOffset;
            if (control.y + yOffset < control.y + control.height)
                control.y += yOffset;
        }
    }
    //左下角拖拽
    DragItem {
        id: leftBottomHandle

        enabled: leftBorderResizable && bottomBorderResizable
        height: borderWidth
        posType: posLeftBottom
        width: borderWidth
        y: parent.height - height

        onPosChange: function (xOffset, yOffset) {
            if (root.keepAspectRatio) {
                let aspectRatio = control.width / control.height;
                if (Math.abs(xOffset) > Math.abs(yOffset)) {
                    xOffset = yOffset * aspectRatio * -1;
                } else {
                    yOffset = xOffset / aspectRatio * -1;
                }
            }
            if (control.x + xOffset < control.x + control.width)
                control.x += xOffset;
            if (control.width - xOffset > 0)
                control.width -= xOffset;
            if (control.height + yOffset > 0)
                control.height += yOffset;
        }
    }
    //右下角拖拽
    DragItem {
        id: rightBottomHandle

        enabled: rightBorderResizable && bottomBorderResizable
        height: borderWidth
        posType: posRightBottom
        width: borderWidth
        x: parent.width - width
        y: parent.height - height

        onPosChange: function (xOffset, yOffset) {
            if (root.keepAspectRatio) {
                let aspectRatio = control.width / control.height;
                if (Math.abs(xOffset) > Math.abs(yOffset)) {
                    xOffset = yOffset * aspectRatio;
                } else {
                    yOffset = xOffset / aspectRatio;
                }
            }
            if (control.width + xOffset > 0)
                control.width += xOffset;
            if (control.height + yOffset > 0)
                control.height += yOffset;
        }
    }
    //上边拖拽
    DragItem {
        id: posTopItem

        enabled: topBorderResizable && !root.keepAspectRatio
        height: borderWidth
        posType: posTop
        width: parent.width - leftTopHandle.width - rightTopHandle.width
        x: leftBottomHandle.width

        onPosChange: function (xOffset, yOffset) {
            if (control.y + yOffset < control.y + control.height)
                control.y += yOffset;
            if (control.height - yOffset > 0)
                control.height -= yOffset;
        }
    }

    //左边拖拽
    DragItem {
        id: posLeftItem

        enabled: leftBorderResizable && !root.keepAspectRatio
        height: parent.height - leftTopHandle.height - leftBottomHandle.height
        posType: posLeft
        width: borderWidth
        y: leftTopHandle.height

        onPosChange: function (xOffset, yOffset) {
            if (control.x + xOffset < control.x + control.width)
                control.x += xOffset;
            if (control.width - xOffset > 0)
                control.width -= xOffset;
        }
    }
    //右边拖拽
    DragItem {
        id: posRightItem

        enabled: rightBorderResizable && !root.keepAspectRatio
        height: parent.height - rightTopHandle.height - rightBottomHandle.height
        posType: posRight
        width: borderWidth
        x: parent.width - width
        y: rightTopHandle.height

        onPosChange: function (xOffset, yOffset) {
            if (control.width + xOffset > 0)
                control.width += xOffset;
        }
    }
    //下边拖拽
    DragItem {
        id: posBottomItem

        enabled: bottomBorderResizable && !root.keepAspectRatio
        height: borderWidth
        posType: posBottom
        width: parent.width - leftBottomHandle.width - rightBottomHandle.width
        x: leftBottomHandle.width
        y: parent.height - height

        onPosChange: function (xOffset, yOffset) {
            if (control.height + yOffset > 0)
                control.height += yOffset;
        }
    }
}

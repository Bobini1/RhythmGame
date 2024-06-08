import QtQuick 2.9
import QtQuick.Controls 2.2
import "../../third_party/TaoQuick/Qml/Misc"

Item {
    id: root
    //controller 要控制大小的目标，可以是Item，也可以是view，只要提供x、y、width、height等属性的修改
    //默认值为parent
    property var control: parent
    property int borderWidth: 12

    // Added for RhythmGame
    property bool topBorderResizable: true
    property bool leftBorderResizable: true
    property bool rightBorderResizable: true
    property bool bottomBorderResizable: true
    
    property bool keepAspectRatio: false

    property bool borderPressed: leftTopHandle.pressed || rightTopHandle.pressed || leftBottomHandle.pressed || rightBottomHandle.pressed || posTopItem.pressed || posLeftItem.pressed || posRightItem.pressed || posBottomItem.pressed

    //左上角的拖拽
    DragItem {
        id: leftTopHandle
        posType: posLeftTop
        width: borderWidth
        height: borderWidth
        enabled: topBorderResizable && leftBorderResizable
        onPosChange: function(xOffset, yOffset){
            if (root.keepAspectRatio) {
                let aspectRatio = control.width / control.height;
                if (xOffset > yOffset) {
                    xOffset = yOffset * aspectRatio;
                } else {
                    yOffset = xOffset / aspectRatio;
                }
            }

            //不要简化这个判断条件，化简之后不容易看懂. Qml引擎会自动简化
            if (control.x + xOffset < control.x + control.width)
                control.x += xOffset;
            if (control.y + yOffset < control.y + control.height)
                control.y += yOffset;
            if (control.width - xOffset > 0)
                control.width-= xOffset;
            if (control.height -yOffset > 0)
                control.height -= yOffset;
        }
    }
    //右上角拖拽
    DragItem {
        id: rightTopHandle
        posType: posRightTop
        x: parent.width - width
        width: borderWidth
        height: borderWidth
        enabled: topBorderResizable && rightBorderResizable
        onPosChange: function(xOffset, yOffset){
            if (root.keepAspectRatio) {
                let aspectRatio = control.width / control.height;
                if (xOffset > yOffset) {
                    xOffset = yOffset * aspectRatio;
                } else {
                    yOffset = xOffset / aspectRatio;
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
        posType: posLeftBottom
        y: parent.height - height
        width: borderWidth
        height: borderWidth
        enabled: leftBorderResizable && bottomBorderResizable
        onPosChange:function(xOffset, yOffset){
            if (root.keepAspectRatio) {
                let aspectRatio = control.width / control.height;
                if (xOffset > yOffset) {
                    xOffset = yOffset * aspectRatio;
                } else {
                    yOffset = xOffset / aspectRatio;
                }
            }

            if (control.x + xOffset < control.x + control.width)
                control.x += xOffset;
            if (control.width - xOffset > 0)
                control.width-= xOffset;
            if (control.height + yOffset > 0)
                control.height += yOffset;
        }
    }
    //右下角拖拽
    DragItem {
        id: rightBottomHandle
        posType: posRightBottom
        x: parent.width - width
        y: parent.height - height
        width: borderWidth
        height: borderWidth
        enabled: rightBorderResizable && bottomBorderResizable
        onPosChange: function(xOffset, yOffset) {
            if (root.keepAspectRatio) {
                let aspectRatio = control.width / control.height;
                if (xOffset > yOffset) {
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
        posType: posTop
        width: parent.width - leftTopHandle.width - rightTopHandle.width
        height: borderWidth
        x: leftBottomHandle.width
        enabled: topBorderResizable && !root.keepAspectRatio
        onPosChange: function(xOffset, yOffset){
            if (control.y + yOffset < control.y + control.height)
                control.y += yOffset;
            if (control.height - yOffset > 0)
                control.height -= yOffset;
        }
    }

    //左边拖拽
    DragItem {
        id: posLeftItem
        posType: posLeft
        height: parent.height - leftTopHandle.height - leftBottomHandle.height
        width: borderWidth

        y: leftTopHandle.height
        enabled: leftBorderResizable && !root.keepAspectRatio
        onPosChange: function(xOffset, yOffset){
            if (control.x + xOffset < control.x + control.width)
                control.x += xOffset;
            if (control.width - xOffset > 0)
                control.width-= xOffset;
        }
    }
    //右边拖拽
    DragItem {
        id: posRightItem
        posType: posRight
        x: parent.width - width
        height: parent.height - rightTopHandle.height - rightBottomHandle.height
        width: borderWidth

        y: rightTopHandle.height
        enabled: rightBorderResizable && !root.keepAspectRatio
        onPosChange: function(xOffset, yOffset) {
            if (control.width + xOffset > 0)
                control.width += xOffset;
        }
    }
    //下边拖拽
    DragItem {
        id: posBottomItem
        posType: posBottom
        x: leftBottomHandle.width
        y: parent.height - height
        width: parent.width - leftBottomHandle.width - rightBottomHandle.width
        height: borderWidth
        enabled: bottomBorderResizable && !root.keepAspectRatio
        onPosChange: function(xOffset, yOffset){
            if (control.height + yOffset > 0)
                control.height += yOffset;
        }
    }
}

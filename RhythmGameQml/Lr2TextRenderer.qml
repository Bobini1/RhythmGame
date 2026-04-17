import QtQuick
import RhythmGameQml 1.0
import "Lr2Timeline.js" as Lr2Timeline

// Root fills the Loader (skin canvas); inner `textBox` does the real
// animated positioning/sizing so that QQuickLoader's size-sync can't
// clobber our width/height bindings.
Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property var chart
    property real scaleOverride: 1.0
    // Bound externally by the screen wrapper with the actual text for this st index
    property string resolvedText: ""

    readonly property var currentState: Lr2Timeline.getCurrentState(dsts, skinTime, timers, activeOptions)
    readonly property bool isLr2Font: srcData && srcData.fontPath && srcData.fontPath.toLowerCase().endsWith(".lr2font")

    Item {
        id: textBox
        x: root.currentState ? root.currentState.x * root.scaleOverride : 0
        y: root.currentState ? root.currentState.y * root.scaleOverride : 0
        width: root.currentState ? root.currentState.w * root.scaleOverride : 0
        height: root.currentState ? root.currentState.h * root.scaleOverride : 0
        visible: root.currentState && root.currentState.a > 0 && width > 0 && height > 0
        opacity: root.currentState ? root.currentState.a / 255.0 : 0

        Lr2BitmapFontText {
            visible: root.isLr2Font
            anchors.fill: parent
            fontPath: root.srcData ? root.srcData.fontPath : ""
            text: root.resolvedText
            alignment: root.srcData ? root.srcData.align : 0
        }

        Text {
            visible: !root.isLr2Font
            anchors.fill: parent
            text: root.resolvedText || ("ST_" + (root.srcData ? root.srcData.st : "?"))
            color: root.currentState
                   ? Qt.rgba(root.currentState.r / 255.0, root.currentState.g / 255.0, root.currentState.b / 255.0, 1.0)
                   : "white"
            font.pixelSize: Math.max(1, Math.round(parent.height))
            font.family: root.srcData && root.srcData.fontPath && !root.isLr2Font ? root.srcData.fontPath : ""
            horizontalAlignment: {
                if (!root.srcData) return Text.AlignLeft;
                if (root.srcData.align === 1) return Text.AlignRight;
                if (root.srcData.align === 2) return Text.AlignHCenter;
                return Text.AlignLeft;
            }
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }
}

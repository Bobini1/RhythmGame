import QtQuick
import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property var chart
    property real scaleOverride: 1.0
    property real value: 0

    readonly property var currentState: Lr2Timeline.getCurrentState(dsts, skinTime, timers, activeOptions)
    readonly property real clampedValue: Math.max(0, Math.min(1, value))
    readonly property int frameIndex: {
        if (!srcData) return 0;
        let timerIdx = srcData.timer || 0;
        let fire = (timers && timers[timerIdx] !== undefined) ? timers[timerIdx] : -1;
        return Lr2Timeline.getAnimationFrame(srcData, skinTime, fire);
    }
    readonly property string resolvedSource: {
        if (!srcData) return "";
        if (srcData.specialType === 1 || srcData.specialType === 3 || srcData.specialType === 4) {
            let chartData = root.chart ? root.chart.chartData : null;
            let fileName = "";
            if (srcData.specialType === 1) {
                fileName = chartData ? chartData.stageFile : "";
            } else if (srcData.specialType === 3) {
                fileName = chartData ? chartData.backBmp : "";
            } else {
                fileName = chartData ? chartData.banner : "";
            }
            if (!chartData || !fileName || !chartData.chartDirectory) {
                return "";
            }
            let dir = chartData.chartDirectory;
            if (dir[0] !== "/") {
                dir = "/" + dir;
            }
            return "file://" + dir + fileName.replace(/\.[^/.]+$/, "");
        }
        if (!srcData.source) return "";
        let absPath = srcData.source.replace(/\\/g, "/");
        if (/^[A-Za-z]:\//.test(absPath)) {
            return "file:///" + absPath;
        }
        if (absPath.startsWith("/")) {
            return "file://" + absPath;
        }
        return absPath;
    }

    Item {
        id: graph
        readonly property real fullW: root.currentState ? root.currentState.w * root.scaleOverride : 0
        readonly property real fullH: root.currentState ? root.currentState.h * root.scaleOverride : 0
        readonly property bool horizontal: root.srcData ? (root.srcData.direction === 0 || root.srcData.direction === 2) : true
        readonly property bool reverse: root.srcData ? (root.srcData.direction === 2 || root.srcData.direction === 3) : false

        x: root.currentState ? root.currentState.x * root.scaleOverride + (horizontal && reverse ? fullW * (1 - root.clampedValue) : 0) : 0
        y: root.currentState ? root.currentState.y * root.scaleOverride + (!horizontal && reverse ? fullH * (1 - root.clampedValue) : 0) : 0
        width: horizontal ? fullW * root.clampedValue : fullW
        height: horizontal ? fullH : fullH * root.clampedValue
        visible: root.currentState && root.currentState.a > 0 && width > 0 && height > 0 && root.resolvedSource !== ""
        opacity: root.currentState ? root.currentState.a / 255.0 : 0

        Image {
            anchors.fill: parent
            source: root.resolvedSource
            fillMode: Image.Stretch
            sourceClipRect: {
                if (!root.srcData) return Qt.rect(0, 0, 0, 0);
                let sx = Math.max(0, root.srcData.x || 0);
                let sy = Math.max(0, root.srcData.y || 0);
                let sw = root.srcData.w || 0;
                let sh = root.srcData.h || 0;
                if (sw <= 0 || sh <= 0) {
                    return Qt.rect(0, 0, 0, 0);
                }
                let divX = Math.max(1, root.srcData.div_x || 1);
                let divY = Math.max(1, root.srcData.div_y || 1);
                let cellW = sw / divX;
                let cellH = sh / divY;
                let col = root.frameIndex % divX;
                let row = Math.floor(root.frameIndex / divX) % divY;
                return Qt.rect(sx + col * cellW, sy + row * cellH, cellW, cellH);
            }
        }
    }
}

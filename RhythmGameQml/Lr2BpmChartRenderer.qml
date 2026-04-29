import QtQuick
import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property var chart
    property real scaleOverride: 1.0

    readonly property bool hasStaticTimelineState: Lr2Timeline.canUseStaticState(dsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(dsts[0], dsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(dsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(dsts) ? activeOptions : []
    readonly property var currentState: staticTimelineState
        || Lr2Timeline.getCurrentStateWithOptionalTimerFire(
            dsts, skinTime, timelineTimers, timerFire, timelineActiveOptions)
    readonly property int fieldW: Math.max(1, srcData ? (srcData.fieldW || 1) : 1)
    readonly property int fieldH: Math.max(1, srcData ? (srcData.fieldH || 1) : 1)
    readonly property real stateW: currentState ? (currentState.w || 0) : 0
    readonly property real stateH: currentState ? (currentState.h || 0) : 0
    readonly property real drawW: Math.abs(stateW) > 0 ? Math.abs(stateW) : fieldW
    readonly property real drawH: Math.abs(stateH) > 0 ? Math.abs(stateH) : fieldH
    readonly property real drawX: currentState
        ? (currentState.x + (stateW < 0 ? stateW : 0)) * scaleOverride
        : 0
    readonly property real drawY: currentState
        ? (currentState.y + (Math.abs(stateH) > 0 && stateH < 0 ? stateH : 0)
            - (Math.abs(stateH) > 0 ? 0 : fieldH)) * scaleOverride
        : 0
    readonly property real reveal: {
        if (!srcData || (srcData.delay || 0) <= 0) {
            return 1;
        }
        return Math.max(0, Math.min(1, skinTime / Math.max(1, srcData.delay || 1)));
    }
    readonly property var chartData: {
        if (!chart) {
            return null;
        }
        if (chart.chartData) {
            return chart.chartData;
        }
        if (chart.bpmChanges) {
            return chart;
        }
        return null;
    }
    readonly property int dataRevision: chartData
        ? ((chartData.md5 ? String(chartData.md5) : "")
           + ":" + (chartData.length || 0)
           + ":" + (chartData.mainBpm || 0)
           + ":" + (chartData.minBpm || 0)
           + ":" + (chartData.maxBpm || 0)).length
        : 0

    function hexColor(value, fallback) {
        let raw = value === undefined || value === null ? "" : String(value);
        raw = raw.replace(/[^0-9a-fA-F]/g, "");
        if (raw.length < 6) {
            return fallback;
        }
        return "#" + raw.substring(0, 6);
    }

    function lineColor(speed, minSpeed, maxSpeed, mainSpeed) {
        if (speed <= 0) {
            return hexColor(srcData ? srcData.stopLineColor : "", "#ff00ff");
        }
        if (Math.abs(speed - mainSpeed) < 0.0001) {
            return hexColor(srcData ? srcData.mainBpmColor : "", "#00ff00");
        }
        if (Math.abs(speed - minSpeed) < 0.0001) {
            return hexColor(srcData ? srcData.minBpmColor : "", "#0000ff");
        }
        if (Math.abs(speed - maxSpeed) < 0.0001) {
            return hexColor(srcData ? srcData.maxBpmColor : "", "#ff0000");
        }
        return hexColor(srcData ? srcData.otherBpmColor : "", "#ffff00");
    }

    function speedAt(change) {
        let bpm = change && change.bpm !== undefined ? Number(change.bpm) : 0;
        let scroll = change && change.scroll !== undefined ? Number(change.scroll) : 1;
        return bpm * scroll;
    }

    function timeAt(change) {
        return change && change.time && change.time.timestamp !== undefined
            ? Number(change.time.timestamp)
            : 0;
    }

    function buildSpeedData(chartData) {
        let changes = chartData && chartData.bpmChanges ? chartData.bpmChanges : [];
        let result = [];
        for (let i = 0; i < changes.length; ++i) {
            result.push({ speed: speedAt(changes[i]), time: timeAt(changes[i]) });
        }
        if (result.length === 0 && chartData) {
            result.push({ speed: chartData.initialBpm || chartData.mainBpm || 0, time: 0 });
        }
        if (result.length > 0 && result[0].time !== 0) {
            result.unshift({ speed: result[0].speed, time: 0 });
        }
        return result;
    }

    function graphY(speed, mainSpeed, height, lineWidth) {
        let minValue = 1.0 / 8.0;
        let maxValue = 8.0;
        let minLog = Math.log(minValue) / Math.LN10;
        let maxLog = Math.log(maxValue) / Math.LN10;
        let ratio = mainSpeed > 0 ? speed / mainSpeed : 1;
        ratio = Math.max(minValue, Math.min(maxValue, ratio));
        let value = (Math.log(ratio) / Math.LN10 - minLog) / (maxLog - minLog);
        return Math.max(0, Math.min(height - lineWidth, (height - lineWidth) * (1 - value)));
    }

    visible: currentState && currentState.a > 0 && !!srcData && !!chartData

    Canvas {
        id: chartCanvas
        x: root.drawX
        y: root.drawY
        width: Math.max(1, root.drawW * root.scaleOverride)
        height: Math.max(1, root.drawH * root.scaleOverride)
        visible: root.visible
        opacity: root.currentState ? Math.max(0, Math.min(1, (root.currentState.a || 255) / 255.0)) : 0
        renderTarget: Canvas.Image
        antialiasing: false

        onPaint: {
            let ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            if (!root.chartData || !root.srcData || width <= 0 || height <= 0) {
                return;
            }

            let data = root.buildSpeedData(root.chartData);
            if (data.length === 0) {
                return;
            }

            let totalLength = Math.max(1, Number(root.chartData.length || 0));
            let lastTime = Math.max(totalLength, data[data.length - 1].time + 1000000000);
            let lineWidth = Math.max(1, Math.round((root.srcData.lineWidth || 2) * root.scaleOverride));
            let mainSpeed = root.chartData.mainBpm || root.chartData.initialBpm || data[0].speed || 1;
            let minSpeed = root.chartData.minBpm || mainSpeed;
            let maxSpeed = root.chartData.maxBpm || mainSpeed;

            ctx.save();
            ctx.beginPath();
            ctx.rect(0, 0, width * root.reveal, height);
            ctx.clip();
            for (let i = 1; i < data.length; ++i) {
                let x = Math.round(width * data[i].time / lastTime);
                let y1 = Math.round(root.graphY(data[i - 1].speed, mainSpeed, height, lineWidth));
                let y2 = Math.round(root.graphY(data[i].speed, mainSpeed, height, lineWidth));
                if (Math.abs(y2 - y1) - lineWidth > 0) {
                    ctx.fillStyle = root.hexColor(root.srcData.transitionLineColor, "#7f7f7f");
                    ctx.fillRect(x, Math.min(y1, y2) + lineWidth, lineWidth, Math.abs(y2 - y1) - lineWidth);
                }

                let x1 = Math.round(width * data[i - 1].time / lastTime);
                let x2 = Math.round(width * data[i].time / lastTime);
                let y = Math.round(root.graphY(data[i - 1].speed, mainSpeed, height, lineWidth));
                ctx.fillStyle = root.lineColor(data[i - 1].speed, minSpeed, maxSpeed, mainSpeed);
                ctx.fillRect(x1, y, Math.max(lineWidth, x2 - x1 + lineWidth), lineWidth);
            }

            let tail = data[data.length - 1];
            let tailX = Math.round(width * tail.time / lastTime);
            let tailY = Math.round(root.graphY(tail.speed, mainSpeed, height, lineWidth));
            ctx.fillStyle = root.lineColor(tail.speed, minSpeed, maxSpeed, mainSpeed);
            ctx.fillRect(tailX, tailY, Math.max(lineWidth, width - tailX), lineWidth);
            ctx.restore();
        }
    }

    function requestChartPaint() {
        if (chartCanvas.available) {
            chartCanvas.requestPaint();
        }
    }

    onChartChanged: requestChartPaint()
    onChartDataChanged: requestChartPaint()
    onDataRevisionChanged: requestChartPaint()
    onSrcDataChanged: requestChartPaint()
    onCurrentStateChanged: requestChartPaint()
    onRevealChanged: requestChartPaint()
    Component.onCompleted: requestChartPaint()
}

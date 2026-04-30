pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

Lr2NativeCursor {
    id: cursor

    required property var screenRoot
    required property var skinModel

    readonly property var root: screenRoot
    readonly property bool rootReady: root !== undefined && root !== null
    readonly property bool modelReady: skinModel !== undefined && skinModel !== null
    readonly property var cursorSrcData: modelReady && skinModel.mouseCursor && skinModel.mouseCursor.src
        ? skinModel.mouseCursor.src
        : null
    readonly property var cursorDsts: modelReady && skinModel.mouseCursor && skinModel.mouseCursor.dsts
        ? skinModel.mouseCursor.dsts
        : []
    readonly property int cursorDstTimer: cursorDsts && cursorDsts.length > 0
        ? (cursorDsts[0].timer || 0)
        : 0
    readonly property int cursorSrcTimer: cursorSrcData ? (cursorSrcData.timer || 0) : 0
    property Lr2TimelineState cursorTimelineState: Lr2TimelineState {
        enabled: cursor.rootReady && !!cursor.cursorSrcData
        dsts: cursor.cursorDsts
        skinTime: cursor.rootReady ? cursor.root.renderSkinTime : 0
        timers: null
        timerFire: cursor.rootReady ? cursor.root.skinTimerFireTime(cursor.cursorDstTimer) : -1
        activeOptions: cursor.rootReady ? cursor.root.runtimeActiveOptions : []
    }
    readonly property var cursorState: rootReady && cursorSrcData
        ? cursorTimelineState.state
        : null
    readonly property bool wholeTextureSource: cursorSrcData !== null
        && (cursorSrcData.x < 0 || cursorSrcData.y < 0
            || cursorSrcData.w < 0 || cursorSrcData.h < 0)
    readonly property bool croppedTextureSource: cursorSrcData !== null
        && cursorSrcData.w > 0 && cursorSrcData.h > 0
    readonly property string resolvedSource: {
        if (!cursorSrcData || !cursorSrcData.source) {
            return "";
        }
        let absPath = cursorSrcData.source.replace(/\\/g, "/");
        if (/^[A-Za-z]:\//.test(absPath)) {
            return "file:///" + absPath;
        }
        if (absPath.startsWith("/")) {
            return "file://" + absPath;
        }
        return absPath;
    }
    property Lr2AnimationFrameState animationFrameState: Lr2AnimationFrameState {
        enabled: cursor.rootReady && !!cursor.cursorSrcData
        sourceData: cursor.cursorSrcData
        skinTime: cursor.rootReady ? cursor.root.renderSkinTime : 0
        timers: null
        timerFire: cursor.rootReady ? cursor.root.skinTimerFireTime(cursor.cursorSrcTimer) : -1
    }
    readonly property int frameIndex: animationFrameState.frameIndex
    readonly property rect clipRect: {
        if (!cursorSrcData || wholeTextureSource || !croppedTextureSource) {
            return Qt.rect(0, 0, 0, 0);
        }

        let sx = Math.max(0, cursorSrcData.x || 0);
        let sy = Math.max(0, cursorSrcData.y || 0);
        let sw = cursorSrcData.w;
        let sh = cursorSrcData.h;
        let divX = Math.max(1, cursorSrcData.div_x || 1);
        let divY = Math.max(1, cursorSrcData.div_y || 1);
        let cellW = sw / divX;
        let cellH = sh / divY;
        let col = frameIndex % divX;
        let row = Math.floor(frameIndex / divX) % divY;

        return Qt.rect(sx + col * cellW, sy + row * cellH, cellW, cellH);
    }

    active: rootReady
        && root.screenUpdatesActive
        && root.effectiveScreenKey === "select"
        && resolvedSource !== ""
    source: resolvedSource
    sourceRect: clipRect
    targetSize: cursorState
        ? Qt.size(cursorState.w * root.skinVisualScaleX, cursorState.h * root.skinVisualScaleY)
        : Qt.size(0, 0)
}

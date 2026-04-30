pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: root

    required property var host
    property bool updatesActive: true
    property int skinHeight: 0
    property int skinTime: 0

    property string text: ""
    readonly property var lines: root.text.length > 0
        ? root.text.split(/\r\n|\n|\r/)
        : [""]
    property int mode: 0 // 0=hidden, 1=open, 2=closing
    property int startSkinTime: 0
    readonly property int elapsed: root.mode > 0
        ? Math.max(0, Math.min(500, root.skinTime - root.startSkinTime))
        : 0
    property real offsetX: 0
    property real offsetY: 0
    property int lineSpacing: 18
    property bool mouseHeld: false
    property real mouseX: 0
    property real mouseY: 0

    property Timer closeTimer: Timer {
        interval: 500
        repeat: false
        onTriggered: root.hideImmediately()
    }

    property FrameAnimation edgeScrollAnimation: FrameAnimation {
        running: root.updatesActive && root.mode === 1 && root.mouseHeld
        onTriggered: {
            const amount = frameTime * 600.0;
            let dx = 0;
            let dy = 0;
            if (root.mouseX < 200) {
                dx += amount;
            } else if (root.mouseX > 440) {
                dx -= amount;
            }
            if (root.mouseY < 150) {
                dy += amount;
            } else if (root.mouseY > 330) {
                dy -= amount;
            }
            if (dx !== 0 || dy !== 0) {
                root.scrollBy(dx, dy);
            }
        }
    }

    function openText(value) {
        root.text = value || "";
        root.startSkinTime = root.skinTime;
        root.mode = 1;
        root.offsetX = 0;
        root.offsetY = 0;
        root.lineSpacing = 18;
        root.mouseHeld = false;
        root.closeTimer.stop();
        if (root.host) {
            root.host.clearSelectSearchFocus();
        }
    }

    function openPath(path) {
        if (!path) {
            return false;
        }
        const fileText = Rg.fileQuery.readTextFile(path) || "";
        root.openText(fileText.length > 0
            ? fileText
            : "HELP FILE NOT FOUND\n\n" + path);
        return true;
    }

    function close() {
        if (root.mode === 0) {
            return;
        }
        root.startSkinTime = root.skinTime;
        root.mode = 2;
        root.mouseHeld = false;
        root.closeTimer.restart();
    }

    function hideImmediately() {
        root.closeTimer.stop();
        root.mode = 0;
        root.text = "";
        root.offsetX = 0;
        root.offsetY = 0;
        root.mouseHeld = false;
    }

    function pauseActivity() {
        root.closeTimer.stop();
        root.mouseHeld = false;
    }

    function linesForSource(src) {
        if (!src || !src.readme || root.mode === 0) {
            return [];
        }
        if ((root.mode === 1 && src.readmeId === 0)
                || (root.mode === 2 && src.readmeId === 1)) {
            return root.lines;
        }
        return [];
    }

    function contentHeight() {
        return root.lines.length * Math.max(1, root.lineSpacing);
    }

    function clampOffsets() {
        const minY = Math.min(0, root.skinHeight - root.contentHeight());
        root.offsetX = Math.min(0, root.offsetX);
        root.offsetY = Math.max(minY, Math.min(0, root.offsetY));
    }

    function scrollBy(dx, dy) {
        if (root.mode !== 1) {
            return false;
        }
        root.offsetX += dx;
        root.offsetY += dy;
        root.clampOffsets();
        return true;
    }
}

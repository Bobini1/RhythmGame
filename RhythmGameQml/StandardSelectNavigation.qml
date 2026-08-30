import QtQuick
import RhythmGameQml

// Reusable directional navigation. It owns analog accumulation and classic
// scratch repeat timing, then emits logical movement for a skin to present.
Item {
    id: root

    property int analogTicksPerStep: 3
    property int initialRepeatDelayMillis: 300
    property int repeatDelayMillis: 50
    property var lastKeys: []
    property int analogBuffer: 0
    property int repeatDirection: 0
    property double nextRepeatMillis: 0

    signal moveRequested(int steps, bool repeated, bool analog)

    function requestMove(steps, repeated, analog = false) {
        if (enabled && steps !== 0) {
            moveRequested(steps, repeated, analog);
        }
    }

    function pressDirection(key) {
        if (enabled) {
            lastKeys = lastKeys.concat([key]);
        }
    }

    function directionStillHeld(up) {
        return up
            ? Input.col1sUp || Input.col2sUp
            : Input.col1sDown || Input.col2sDown;
    }

    function releaseDirection(key, up) {
        lastKeys = lastKeys.filter(pressedKey => pressedKey !== key);
        if (!directionStillHeld(up) && repeatDirection === (up ? -1 : 1)) {
            repeatDirection = 0;
            nextRepeatMillis = 0;
        }
    }

    function navigate(tickNumber, tickType, up, key) {
        if (!enabled) {
            return;
        }
        let direction = up ? -1 : 1;
        if (tickType === InputTranslator.AnalogScratchTick) {
            analogBuffer += direction;
            let steps = Math.trunc(analogBuffer / Math.max(1, analogTicksPerStep));
            analogBuffer %= Math.max(1, analogTicksPerStep);
            requestMove(steps, false, true);
            return;
        }
        if (lastKeys[lastKeys.length - 1] !== key) {
            return;
        }
        if (tickType === InputTranslator.ButtonTick
                || tickType === InputTranslator.ClassicScratchTick) {
            let now = Date.now();
            let firstTick = tickNumber === 0
                || repeatDirection !== direction;
            if (!firstTick && now < nextRepeatMillis) {
                return;
            }
            repeatDirection = direction;
            nextRepeatMillis = now + (firstTick
                ? initialRepeatDelayMillis : repeatDelayMillis);
            requestMove(direction, !firstTick);
            return;
        }
        requestMove(direction, !!tickNumber);
    }

    function reset() {
        lastKeys = [];
        analogBuffer = 0;
        repeatDirection = 0;
        nextRepeatMillis = 0;
    }

    onEnabledChanged: {
        if (!enabled) {
            reset();
        }
    }
}

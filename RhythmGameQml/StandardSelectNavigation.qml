import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectNavigation
    \inqmlmodule RhythmGameQml
    \brief Converts directional keys, scratch and mouse wheel input into row movement.

    Handle \l moveRequested to move your list or wheel. Negative \c steps means
    up, and positive means down. \c repeated is true after the first movement
    from a held key or classic scratch. \c analog identifies movement accumulated
    from analog scratch ticks or the mouse wheel.

    The theme keeps the focused row and chooses whether to wrap, stop at an end
    or animate the movement. The component only tracks held input and repeat timing.

    Use StandardSelectInput to add the standard selection actions. Call
    \l pressDirection, \l navigate and \l releaseDirection directly when adapting
    another input source. Forward wheel events from your view to \l handleWheel.
    Disabling the component calls \l reset to clear held keys and accumulated
    scratch and wheel movement.
*/
Item {
    id: root

    /*! Number of analog scratch ticks required for one logical step. */
    property int analogTicksPerStep: 3
    /*! Delay before classic-scratch repeat begins, in milliseconds. */
    property int initialRepeatDelayMillis: 300
    /*! Delay between repeated classic-scratch steps, in milliseconds. */
    property int repeatDelayMillis: 50

    /*!
        Requests relative focus movement by \a steps. \a repeated identifies held input and \a
        analog identifies analog scratch or mouse wheel input.
    */
    signal moveRequested(int steps, bool repeated, bool analog)

    QtObject {
        id: navigationState

        property int analogBuffer: 0
        property real wheelRemainder: 0
        property var lastKeys: []
        property double nextRepeatMillis: 0
        property int repeatDirection: 0

        function requestMove(steps, repeated, analog = false) {
            if (root.enabled && steps !== 0) {
                root.moveRequested(steps, repeated, analog);
            }
        }

        function directionStillHeld(up) {
            return up
                ? Input.col1sUp || Input.col2sUp
                : Input.col1sDown || Input.col2sDown;
        }
    }

    /*! Records that directional \a key was pressed. */
    function pressDirection(key) {
        if (enabled) {
            navigationState.lastKeys = navigationState.lastKeys.concat([key]);
        }
    }

    /*! Records that directional \a key was released; \a up identifies its direction. */
    function releaseDirection(key, up) {
        navigationState.lastKeys = navigationState.lastKeys.filter(
            pressedKey => pressedKey !== key);
        if (!navigationState.directionStillHeld(up)
                && navigationState.repeatDirection === (up ? -1 : 1)) {
            navigationState.repeatDirection = 0;
            navigationState.nextRepeatMillis = 0;
        }
    }

    /*!
        Converts \a tickNumber and \a tickType for directional \a key into a movement request.
        \a up selects the movement direction.
    */
    function navigate(tickNumber, tickType, up, key) {
        if (!enabled) {
            return;
        }
        let direction = up ? -1 : 1;
        if (tickType === InputTranslator.AnalogScratchTick) {
            navigationState.analogBuffer += direction;
            let ticksPerStep = Math.max(1, analogTicksPerStep);
            let steps = Math.trunc(navigationState.analogBuffer / ticksPerStep);
            navigationState.analogBuffer %= ticksPerStep;
            navigationState.requestMove(steps, false, true);
            return;
        }
        if (navigationState.lastKeys[navigationState.lastKeys.length - 1]
                !== key) {
            return;
        }
        if (tickType === InputTranslator.ButtonTick
                || tickType === InputTranslator.ClassicScratchTick) {
            let now = Date.now();
            let firstTick = tickNumber === 0
                || navigationState.repeatDirection !== direction;
            if (!firstTick && now < navigationState.nextRepeatMillis) {
                return;
            }
            navigationState.repeatDirection = direction;
            navigationState.nextRepeatMillis = now + (firstTick
                ? initialRepeatDelayMillis : repeatDelayMillis);
            navigationState.requestMove(direction, !firstTick);
            return;
        }
        navigationState.requestMove(direction, !!tickNumber);
    }

    /*!
        Converts a vertical wheel \a event into row movement. Accumulates small
        deltas and moves one row per wheel notch. Returns true and accepts the
        event when handled, including a partial step. Returns false when disabled
        or when the event has no vertical movement.
    */
    function handleWheel(event) {
        if (!root.enabled) return false;
        const delta = event.angleDelta.y || event.pixelDelta.y;
        if (delta === 0) return false;
        navigationState.wheelRemainder += delta;
        const steps = Math.trunc(navigationState.wheelRemainder / 120);
        navigationState.wheelRemainder -= steps * 120;
        event.accepted = true;
        navigationState.requestMove(-steps, false, true);
        return true;
    }

    /*! Clears held keys, accumulated scratch and wheel movement, and repeat timing. */
    function reset() {
        navigationState.lastKeys = [];
        navigationState.analogBuffer = 0;
        navigationState.wheelRemainder = 0;
        navigationState.repeatDirection = 0;
        navigationState.nextRepeatMillis = 0;
    }

    onEnabledChanged: {
        if (!enabled) {
            reset();
        }
    }
}

import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectNavigation
    \inqmlmodule RhythmGameQml
    \brief Converts directional input into logical selection movement.

    The component owns analog accumulation and classic-scratch repeat timing,
    then emits \l moveRequested for a skin to present. It does not own the
    focused row or any visual list.

    A negative \c steps value moves upward and a positive value moves downward.
    \c repeated is true after the first classic-scratch or key-repeat movement;
    \c analog is true only for accumulated analog-scratch movement. The skin
    decides whether movement wraps, clamps, or animates.

    Use \l StandardSelectInput when the standard input mapping is wanted. Call
    \l pressDirection, \l navigate, and \l releaseDirection directly only when
    adapting another input source. Disabling the component calls \l reset and
    discards held-key and partial analog state.
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
        Requests relative focus movement by \a steps. \a repeated identifies
        held input and \a analog identifies analog-scratch input.
    */
    signal moveRequested(int steps, bool repeated, bool analog)

    QtObject {
        id: navigationState

        property int analogBuffer: 0
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
        Converts \a tickNumber and \a tickType for directional \a key into a
        movement request. \a up selects the movement direction.
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

    /*! Clears held keys, analog accumulation, and repeat timing. */
    function reset() {
        navigationState.lastKeys = [];
        navigationState.analogBuffer = 0;
        navigationState.repeatDirection = 0;
        navigationState.nextRepeatMillis = 0;
    }

    onEnabledChanged: {
        if (!enabled) {
            reset();
        }
    }
}

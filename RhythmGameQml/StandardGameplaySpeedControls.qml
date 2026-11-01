import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardGameplaySpeedControls
    \inqmlmodule RhythmGameQml
    \brief Adjusts note speed and covers with Start, Select and scratch input.

    StandardGameplayFlow and StandardGameplayInput already include this component.
    Use it separately only when supplying your own gameplay input handling.
    Bind \l gameplay to the current context and disable the item while inactive.

    Hold Start and turn the scratch to change noteScreenTimeMillis. Hold Select
    and turn the scratch to adjust lane cover, lift or hidden cover. Lane cover
    takes priority when enabled. Otherwise, the enabled lift or hidden cover is
    adjusted. When both are enabled, a short Start+Select press switches between
    them. Double-press Start within 500 milliseconds to toggle lane cover while
    preserving note speed where the duration limits allow it.

    Settings are saved in each player's profile. Battle uses a separate profile
    for each side. In double play, either side adjusts the same player's settings,
    and Start or Select on either side can modify either scratch. Holding both
    buttons suspends adjustments so the retry gesture can be used.
*/
Item {
    id: root

    /*! Supplies the current players and keymode, including course stage changes. */
    property GameplayContext gameplay: null

    QtObject {
        id: controlState
        readonly property bool doublePlay: root.gameplay?.keymode === 10
            || root.gameplay?.keymode === 14
    }

    Repeater {
        model: 2

        Item {
            id: side
            required property int index
            readonly property Player player: root.gameplay?.players[controlState.doublePlay ? 0 : index] || null
            readonly property var vars: player?.profile.vars.generalVars
            enabled: !!player

            // All held-state reads use this item's attached Input instance.
            readonly property bool start: controlState.doublePlay
                ? side.Input.start1 || side.Input.start2
                : (index === 0 ? side.Input.start1 : side.Input.start2)
            readonly property bool select: controlState.doublePlay
                ? side.Input.select1 || side.Input.select2
                : (index === 0 ? side.Input.select1 : side.Input.select2)
            readonly property bool bothHeld: start && select
            readonly property bool bothScratchDirections: index === 0
                ? side.Input.col1sUp && side.Input.col1sDown
                : side.Input.col2sUp && side.Input.col2sDown
            property bool changeLift: true
            property bool lastDirectionUp: false
            property double lastStartPressTime: -1

            onBothHeldChanged: if (enabled && bothHeld) changeLift = !changeLift
            onPlayerChanged: reset()
            onEnabledChanged: if (!enabled) reset()

            function reset() {
                lastStartPressTime = -1;
                lastDirectionUp = false;
                changeLift = true;
            }

            function startPressed(time) {
                if (lastStartPressTime >= 0 && time >= lastStartPressTime
                        && time - lastStartPressTime <= 500) {
                    lastStartPressTime = -1;
                    const visibleRatio = Math.max(0.001, 1 - Math.max(0, Math.min(1, vars.laneCoverRatio)));
                    vars.noteScreenTimeMillis = Math.max(1, Math.min(10000,
                        vars.laneCoverOn ? vars.noteScreenTimeMillis / visibleRatio
                                         : vars.noteScreenTimeMillis * visibleRatio));
                    vars.laneCoverOn = !vars.laneCoverOn;
                } else {
                    lastStartPressTime = time;
                }
            }

            function adjust(tick, amount) {
                // When both scratch directions are held, only the latest one acts.
                if (bothHeld || (bothScratchDirections
                        && ((amount > 0 && lastDirectionUp)
                            || (amount < 0 && !lastDirectionUp)))) return;
                const step = amount * (tick > 50 || bothScratchDirections ? 10 : 1);
                if (start) {
                    vars.noteScreenTimeMillis = Math.max(1, vars.noteScreenTimeMillis * (1 + step / 1000));
                } else if (select) {
                    const option = vars.laneCoverOn || (!vars.liftOn && !vars.hiddenOn)
                        ? "laneCoverRatio"
                        : (vars.liftOn && (!vars.hiddenOn || changeLift) ? "liftRatio" : "hiddenRatio");
                    vars[option] = Math.max(0, Math.min(1, vars[option] + 0.0005 * step));
                }
            }

            Input.onStart1Pressed: time => { if (side.index === 0) side.startPressed(time); }
            Input.onStart2Pressed: time => { if (side.index === 1) side.startPressed(time); }
            Input.onCol1sUpTicked: (tick, type) => { if (side.index === 0) side.adjust(tick, -1); }
            Input.onCol1sDownTicked: (tick, type) => { if (side.index === 0) side.adjust(tick, 1); }
            Input.onCol2sUpTicked: (tick, type) => { if (side.index === 1) side.adjust(tick, -1); }
            Input.onCol2sDownTicked: (tick, type) => { if (side.index === 1) side.adjust(tick, 1); }
            Input.onCol1sUpPressed: if (side.index === 0) side.lastDirectionUp = true
            Input.onCol1sUpReleased: if (side.index === 0) side.lastDirectionUp = false
            Input.onCol1sDownPressed: if (side.index === 0) side.lastDirectionUp = false
            Input.onCol1sDownReleased: if (side.index === 0) side.lastDirectionUp = true
            Input.onCol2sUpPressed: if (side.index === 1) side.lastDirectionUp = true
            Input.onCol2sUpReleased: if (side.index === 1) side.lastDirectionUp = false
            Input.onCol2sDownPressed: if (side.index === 1) side.lastDirectionUp = false
            Input.onCol2sDownReleased: if (side.index === 1) side.lastDirectionUp = true
        }
    }
}

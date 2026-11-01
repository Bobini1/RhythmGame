# 7. Draw notes during gameplay {#theme_tutorial_gameplay}

[Previous](@ref theme_tutorial_decide) | [Tutorial](@ref theme_tutorial) | [Next: Results](@ref theme_tutorial_results)

**Goal:** Draw notes and live scores using the player's speed and cover settings,
with the standard flow handling play, retry and courses.

Install `docs/examples/theme-tutorial/07-gameplay/` and choose it for the gameplay
screens you use in Settings > Themes. It supports 5k, 7k, their battle variants,
10k and 14k. Keep an existing result theme selected for now.

[Download 07-gameplay](examples/theme-tutorial/07-gameplay.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/07-gameplay).

The screen uses rectangles for notes, long notes, bar lines and mines. It also
shows each player's score, combo, latest judgement and life gauge, with the
chart's background animation (BGA) beside the lanes.

`Gameplay.qml` arranges the screen, `PlayerField.qml` draws one player's field,
and `NoteLane.qml` draws one lane. `GaugeBar.qml` displays the active gauge.

## Let the standard flow run the play

The root receives the same [GameplayContext](qml/qml-rhythmgameqml-gameplaycontext.html) used by decide.
Bind it to [StandardGameplayFlow](qml/qml-rhythmgameqml-standardgameplayflow.html).

\snippet 07-gameplay/Gameplay.qml flow

The flow handles startup, Escape, retry, speed controls and results. For a course, it also
continues after each stage result and opens the final course summary.
Don't add [StandardGameplayInput](qml/qml-rhythmgameqml-standardgameplayinput.html)
beside it or call the runner's start and finish methods yourself.

## Follow the current stage and players

All gameplay themes must support courses. Bind to `gameplay.chartData` and
`gameplay.players` so the display follows the stage that is currently playing.

\snippet 07-gameplay/Gameplay.qml stage

`stageIndex` starts at zero; add one for display. A single chart has index zero
and a count of one. The note renderer uses the same player interface for both.

`players` has one entry for SP or DP and two for local battle.

\snippet 07-gameplay/Gameplay.qml players

Each field receives the gameplay context and a player index. Its `player`
binding follows `gameplay.players[playerIndex]` when a course changes stages.

## Give the lanes half of the screen

The outer `RowLayout` gives its two children equal widths with
`uniformCellSizes: true`. The player fields occupy the left half, while the
right half holds BGA and leaves room for the Arena panel. Both halves share
the available height.

\snippet 07-gameplay/Gameplay.qml half-screen

The right half is an `Item` containing BGA, so it keeps its space when the
player turns BGA off. The lanes stay the same width. Local battle divides the
left half between two player playfields. Double play uses one playfield for all lanes.

## Show loading and chart progress

`gameplay.status` tells you whether the stage is loading, ready, running or
finished. In Arena, Ready can remain on screen while the game waits for the
shared start time.

\snippet 07-gameplay/Gameplay.qml play-status

The first player's `bpm`, `elapsed` and `chartLength` supply the chart display.
Divide the time values by one billion to convert nanoseconds to seconds. The
progress bar can use the original units. Elapsed time can be negative before
play starts, so the label clamps it to zero.

## Show the background animation

[StandardGameplayBga](qml/qml-rhythmgameqml-standardgameplaybga.html) draws the
current chart's BGA. Give it the gameplay context and a place in the layout.

\snippet 07-gameplay/Gameplay.qml bga

It handles the image layers, miss animation and course stage changes. Visibility
follows the player's `bgaOn` setting, or the main profile's setting in battle.
Override `visible` if needed. For a custom renderer, use `gameplay.bga.layers`.

## Show the player's score target

[StandardGameplayTarget](qml/qml-rhythmgameqml-standardgameplaytarget.html)
uses the same context and player index. The example displays its current
target points in a label.

\snippet 07-gameplay/PlayerField.qml score-target

The target follows the player's profile setting: a score fraction, best score,
latest score or next rank. It uses replay progress when available, or a score
rate otherwise, and updates for each course stage. In battle and Arena, it
follows an opponent. Check `available` before showing it.

## Draw each player's lanes

`player.columns` supplies the playable column indices for the current player.
They follow the default visual order, with the left scratch first and the right
scratch last. Five-key modes omit unused columns. `player.isScratchColumn(index)`
identifies scratch lanes for their colors.

Changing the visual column order does not change the input mapping.

\snippet 07-gameplay/PlayerField.qml columns

## Follow the player's note speed

[StandardGameplayViewport](qml/qml-rhythmgameqml-standardgameplayviewport.html)
reads `noteScreenTimeMillis` and `hiSpeedFix` from the player's profile. Give it
the current player, chart metadata and lane height. It calculates
`pixelsPerBeat` for drawing and keeps the filtered note models in sync,
including long notes whose bodies cross either edge of the visible area.

\snippet 07-gameplay/PlayerField.qml visible-range

`noteScreenTimeMillis` sets the travel time at the BPM chosen by `hiSpeedFix`.
The helper uses the same calculation as Default.

| `HiSpeedFix` value | BPM used for note speed |
|--------------------|-------------------------|
| `Off`              | 120                     |
| `Main`             | `chartData.mainBpm`     |
| `Start`            | `chartData.initialBpm`  |
| `Min`              | `chartData.minBpm`      |
| `Max`              | `chartData.maxBpm`      |
| `Avg`              | `chartData.avgBpm`      |

At a reference BPM of 120 and a travel time of 1000 milliseconds, two beats
fit in the field. Resizing the field changes their spacing in pixels. The
visible span still follows the saved travel time and BPM setting.
The conversion is `(noteScreenTimeMillis / 1000) * (referenceBpm / 60)`,
which multiplies seconds by beats per second.

The calculation updates with the settings and current stage. BPM and scroll
changes within the chart still affect how notes move.

The example also draws covers and lift from `player.profile.vars.generalVars`.
Lift raises the judgement line by shortening the lanes. Lane cover draws over
the top of those lanes. The helper's `noteHeight` is the shortened lane height,
and `visibleHeight` subtracts the lane cover from it. Hidden cover draws over
the bottom of the lanes without changing the speed calculation.

\snippet 07-gameplay/PlayerField.qml covers

For example, with a 20% lift and a 25% lane cover, the judgement line sits at
80% of the field height. The lane cover hides a quarter of that remaining area,
leaving 60% of the full field for the saved note travel time.

`NoteLane.qml` places a note relative to `player.position`. As the player
position approaches the note position, the note moves toward the line at the
bottom. It uses the helper's `pixelsPerBeat` for both note positions and long
note lengths. Both positions include the chart's scroll changes. Long note
bodies extend from their beginning toward their end position.

The file starts with `pragma ValueTypeBehavior: Addressable` so it can use
constants such as `note.Type.LongNoteBegin`. Qt requires this for the lowercase
`note` value type.

\snippet 07-gameplay/NoteLane.qml notes

The field also draws bar lines from `player.state.barLinesState`. Each row's
`display.time.position` gives its position with scroll changes applied. Use
the same `pixelsPerBeat` as the notes so both move together. The model already
filters the lines to the visible range.

\snippet 07-gameplay/PlayerField.qml bar-lines

The lane highlight reads `columnState.pressed`. Autoplay and replay update
that state too. [Input](@ref qml_components::InputAttached) reports physical input actions, so using it for the
highlight would miss presses supplied by autoplay or replay.

\snippet 07-gameplay/NoteLane.qml pressed

## Adjust speed and covers while playing

[StandardGameplayFlow](qml/qml-rhythmgameqml-standardgameplayflow.html) includes
[StandardGameplaySpeedControls](qml/qml-rhythmgameqml-standardgameplayspeedcontrols.html).
The example only needs a label to explain the controls.

\snippet 07-gameplay/Gameplay.qml speed-controls

Hold START and turn the scratch to change `noteScreenTimeMillis`. Hold SELECT
and turn the scratch to change the cover amount. Double-press START within
half a second to turn lane cover on or off. The toggle adjusts the saved travel
time to keep the same scrolling speed, within the allowed duration limits.

SELECT adjusts lane cover when it is on. Otherwise, it adjusts lift or hidden,
whichever is on. When both lift and hidden are on, briefly press START+SELECT
to switch between them. With all three off, SELECT still adjusts the saved
lane-cover amount for the next time you enable it.

Each battle player changes their own profile. In double play, controls on
either side use the same profile. Adjustments pause during START+SELECT and
retry choice, and stop when the screen is inactive.

Set `speedControlsEnabled: false` on the flow to supply your own controls.
The lane renderer still follows the saved settings.

## Show the current gauge

`player.score.gauges` holds the live gauges for that player. It may contain
several gauges, depending on the player's gauge mode. Course stages supply
course gauges through the same property.

\snippet 07-gameplay/PlayerField.qml gauge

`GaugeBar.qml` shows the first gauge above its clear threshold, or the last
gauge if none is above its threshold. The binding updates as the player hits
or misses notes.

\snippet 07-gameplay/GaugeBar.qml active-gauge

The bar uses `gaugeMax` for its range and shows the gauge's `name` beside its
value. A theme can draw every gauge or choose another display rule using this
same list.

## Show the latest judgement

Each player's [BmsLiveScore](@ref gameplay_logic::BmsLiveScore) emits `hit`
for gameplay events. The event's `judgement` identifies the outcome, or is
`-1` when no judgement was awarded. Listen to the score so the display also
works during autoplay and replay.

\snippet 07-gameplay/PlayerField.qml judgement-feedback

The example maps judgements to text and colors, ignoring events such as
"mine avoided". Each displayed judgement restarts a half-second timer. A new
course stage changes the connection's target and clears the previous judgement.

## Add retry controls

The Retry and Same random buttons call the flow's `retry(samePattern)` method.
Bind their enabled state to `retryAvailable` so the flow decides when retry
is allowed.

\snippet 07-gameplay/Gameplay.qml retry-buttons

For a manual single chart, hold START+SELECT to choose a retry. Release START
for the same random pattern, SELECT for a new pattern, or both to cancel.
The Exit button is disabled during this choice. Standard quick retry is
unavailable for courses, autoplay, replay, battle and Arena.

## Include Arena standings

The screen adds [StandardArenaGameplayOverlay](qml/qml-rhythmgameqml-standardarenagameplayoverlay.html) beside the
gameplay flow.
Both receive the same context. The overlay shows standings and chat for
Arena play and stays hidden during local play and courses.
Its default position is in the upper-right corner, outside the left half
used by the lanes. The player can move or resize it.

\snippet 07-gameplay/Gameplay.qml arena-gameplay

## Try play, retry and a course

Start with an easy chart or autoplay. Check long notes and mines, then resize
the window: all lanes should remain visible. Try the speed and cover controls
and confirm that the notes follow the saved settings. In manual single-chart
play, try both retry buttons.

Play a course past its first result. The next stage should change both the
title and notes. Closing the final course summary should return to selection.
The example's combo label covers the current stage; `coursePlayers` provides
the combo for the whole course if you want to display it.

For later customization, the [flow reference](qml/qml-rhythmgameqml-standardgameplayflow.html)
describes `startReady` and `finishReady` for delaying transitions around an
animation. Keep the standard score saving and navigation when adding such effects.

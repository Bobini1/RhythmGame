# 7. Draw notes during gameplay {#theme_tutorial_gameplay}

[Previous](06-decide.md) | [Tutorial](index.md) | [Next: Results](08-results.md)

Install `docs/examples/theme-tutorial/07-gameplay/` and choose it for the gameplay
screens you use in Settings > Themes. It provides 5k, 7k, their battle variants,
10k and 14k. Keep an existing result theme selected for now.

[Download 07-gameplay](examples/theme-tutorial/07-gameplay.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/07-gameplay).

The screen draws normal notes, long notes, bar lines and red mines, with the scratch
lanes tinted differently. It also shows each player's score, combo and latest
judgement, along with BGA and a life gauge. The lanes use plain rectangles and follow
the player's saved note speed, lane cover, lift and hidden settings. The standard
flow component supplies the same speed and cover controls as Default.

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

`stageIndex` starts at zero, while the label adds one for the reader. A single
chart has index zero and a count of one. Course details add context to the
display, but the note renderer uses the same player interface in both cases.

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
finished. Showing it helps the player distinguish loading from a chart that
has started. In Arena, Ready can remain on screen while the game waits for
the shared start time.

\snippet 07-gameplay/Gameplay.qml play-status

The first player's `bpm`, `elapsed` and `chartLength` supply the common chart
display. `bpm` follows BPM changes during play. The time values are in
nanoseconds, so the label divides them by one billion to show seconds. The
progress bar uses their original units because both values use the same scale.
Elapsed time can be negative during the lead-in, so the display clamps it to zero.
These bindings follow the new player when a course advances.

## Show the background animation

[StandardGameplayBga](qml/qml-rhythmgameqml-standardgameplaybga.html) draws the
current chart's BGA. Give it the gameplay
context and a place in the layout. This example puts it in the right half of
the screen, beside the lanes.

\snippet 07-gameplay/Gameplay.qml bga

The component connects the base image, two overlay layers and the miss layer
to video outputs. Black becomes transparent in the overlay layers. Bad and Poor
judgements show the miss layer for 400 milliseconds. These connections follow
`gameplay.bga` when a course changes stages.

Visibility follows the player's `bgaOn` setting. In battle, it follows the main
profile. You can override `visible`, or replace the component with your own
renderer using `gameplay.bga.layers`.

## Show the player's score target

[StandardGameplayTarget](qml/qml-rhythmgameqml-standardgameplaytarget.html)
uses the same context and player index. The example displays its current
target points in a label.

\snippet 07-gameplay/PlayerField.qml score-target

The target follows the player's profile setting: a score fraction, best run,
latest run or next rank. Saved recordings supply the target's progress through
the chart. A saved score without a recording uses its score rate instead.
The component loads new scores for each course stage and cancels old requests.

Battle targets follow the other player. Arena targets follow the opponent
chosen by the session. `available` tells the display when it has a target.
The helper also exposes `finalPoints` and personal-best values for graphs.

## Draw each player's lanes

`player.columns` supplies the playable column indices for the current player.
They follow the default visual order, with the left scratch first and the right
scratch last. Five-key modes omit unused columns. `player.isScratchColumn(index)`
identifies scratch lanes for their colors.

The example keeps the default input mapping. A theme can arrange the columns
differently, for example to flip the scratch lane in 5k mode.

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

The calculation updates when the settings or current stage change. BPM and
scroll changes within the chart still affect how notes move. The helper uses
the selected reference BPM, so a faster part of the chart can bring notes
through the field sooner.

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

Each battle player changes their own profile. In double play, either scratch
uses the same profile, and START or SELECT can be held on either side. Speed
and cover adjustments pause while both buttons are held or a retry is being
chosen. They also stop when the screen is inactive.

Set `speedControlsEnabled: false` on the flow if you want different controls.
The lane renderer still follows changes to the saved settings. For a custom
flow, [StandardGameplayInput](qml/qml-rhythmgameqml-standardgameplayinput.html)
also includes these controls, so do not add a second copy.

## Show the current gauge

`player.score.gauges` holds the live gauges for that player. It may contain
several gauges, depending on the player's gauge mode. Course stages supply
course gauges through the same property.

\snippet 07-gameplay/PlayerField.qml gauge

`GaugeBar.qml` shows the first gauge above its clear threshold, or the last
gauge if none is above its threshold. This follows Default's display choice.
Its binding reads each gauge's current value, so it updates as the player hits
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

The example maps the displayed judgements to text and colors. It ignores
events absent from the map, like "mine avoided" events.
A displayed judgement restarts a half-second timer,
even when it matches the previous one.

The label becomes transparent when the timer stops, so the lanes keep the
same height. Changing the player, including when a course advances, changes
the connection's target and stops the timer. The next stage starts with no
judgement showing.

## Include Arena standings

The screen adds [StandardArenaGameplayOverlay](qml/qml-rhythmgameqml-standardarenagameplayoverlay.html) beside the
gameplay flow.
Both receive the same context. The overlay shows standings and chat for
Arena play and stays hidden during local play and courses.
Its default position is in the upper-right corner, outside the left half
used by the lanes. The player can move or resize it.

\snippet 07-gameplay/Gameplay.qml arena-gameplay

Read the files together to follow the screen, player and lane structure.

\include{lineno} 07-gameplay/Gameplay.qml

\include{lineno} 07-gameplay/PlayerField.qml

\include{lineno} 07-gameplay/NoteLane.qml

\include{lineno} 07-gameplay/GaugeBar.qml

## Try play, retry and a course

Start with an easy chart or autoplay to inspect the notes. Check a long note
and a mine, then resize the window to make sure every lane remains visible.
Try double play or battle if you use those modes.
Turn BGA off and check that the lanes still occupy half of the screen. In an
Arena round, check that the panel's initial position leaves the lanes clear.

Change the saved note travel time and BPM mode, then play a chart with BPM
changes. Check that the example follows those settings, including when a
course advances to a stage with a different BPM.

Try START and SELECT with the scratch, then toggle lane cover with a double
press of START. Enable lift and hidden in your profile settings and check their
positions too. Play a chart with BGA and miss a note to see the miss layer.

For an ordinary manual single chart, hold START+SELECT to enter retry choice.
Release START for the same random, SELECT for a new random, or both to
cancel. The example disables its Exit button while the choice is active.
Courses, autoplay, replay, battle and Arena don't use the standard quick retry.

The example also provides Retry and Same random buttons. Their `retryAvailable`
binding uses the flow's eligibility checks. Both buttons call `retry(samePattern)`,
which is also used by the hold-and-release gesture.

\snippet 07-gameplay/Gameplay.qml retry-buttons

Play a course through a stage result and confirm that the next stage changes
both title and notes. The final course summary should return to selection
when closed. `coursePlayers` provides combo state for the whole course if
you want to add it, while the example's combo label describes the current stage.

For later customization, the [flow reference](qml/qml-rhythmgameqml-standardgameplayflow.html)
describes `startReady` and `finishReady` for delaying transitions around an
animation. Keep the standard score saving and navigation when adding such effects.

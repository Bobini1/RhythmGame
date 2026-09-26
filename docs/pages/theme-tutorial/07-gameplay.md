# 7. Draw notes during gameplay {#theme_tutorial_gameplay}

[Previous](06-decide.md) | [Tutorial](index.md) | [Next: Results](08-results.md)

Install `docs/examples/theme-tutorial/07-gameplay/` and choose it for the gameplay
roles you use in Settings > Themes. It provides 5k, 7k, their battle variants,
10k and 14k. Keep an existing result theme selected for now.

[Download 07-gameplay](examples/theme-tutorial/07-gameplay.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/07-gameplay).

The screen draws normal notes, long notes and red mines, with the scratch
lanes tinted differently. It also shows each player's score and combo. The
renderer uses plain rectangles and a fixed visible span of six beats. It
doesn't implement Default's BGA, lane covers, gauges or speed controls.

## Let the standard flow run the play

The root receives the same `GameplayContext` used by decide. Bind it to one
`StandardGameplayFlow` directly inside the root. The flow uses its parent as
the owning screen, so set `screen` explicitly if you nest it inside another item.

\snippet 07-gameplay/Gameplay.qml flow

The flow handles startup, Escape, retry and results. For a course, it also
continues after each stage result and opens the final course summary.
Don't add `StandardGameplayInput` beside it or call the runner's start and
finish methods yourself.

## Follow the current stage and players

All gameplay themes must support courses. Bind to `gameplay.chartData` and
`gameplay.players` so the display follows the stage that is currently playing.
Do not copy the first player into a JavaScript variable on startup and keep
using it after the course advances.

\snippet 07-gameplay/Gameplay.qml stage

`stageIndex` starts at zero, while the label adds one for the reader. A single
chart has index zero and a count of one. Course details add context to the
display, but the note renderer uses the same player interface in both cases.

`players` has one entry for solo or double play and two for local battle.
Double play is one player with two sets of lanes. There is no empty second
entry to handle in solo play.

\snippet 07-gameplay/Gameplay.qml players

## Draw each player's lanes

`PlayerField.qml` chooses column indices from the player's effective keymode.
Scratch uses column 7 on the left side and 15 on the right. The five-key modes
omit unused columns, and the example keeps the game's default input mapping.

\snippet 07-gameplay/PlayerField.qml columns

The gameplay state provides a filtered model for each column. Set its visible
span so it can keep the notes needed by the renderer, including long notes
that began below the visible area.

\snippet 07-gameplay/PlayerField.qml visible-range

`NoteLane.qml` places a note relative to `player.position`. As the player
position approaches the note position, the note moves toward the line at the
bottom. Both positions include the chart's scroll changes. Long note bodies
extend from their beginning toward their end position.

\snippet 07-gameplay/NoteLane.qml notes

The lane highlight reads `columnState.pressed`. Autoplay and replay update
that state too. `Input` reports physical input actions, so using it for the
highlight would miss presses supplied by autoplay or replay.

\snippet 07-gameplay/NoteLane.qml pressed

The screen adds `StandardArenaGameplayOverlay` beside the gameplay flow.
Both receive the same context. The overlay shows standings and chat for
Arena play and stays hidden during local play and courses.

\snippet 07-gameplay/Gameplay.qml arena-gameplay

Read the files together to follow the screen, player and lane structure.

\include{lineno} 07-gameplay/Gameplay.qml

\include{lineno} 07-gameplay/PlayerField.qml

\include{lineno} 07-gameplay/NoteLane.qml

## Try play, retry and a course

Start with an easy chart or autoplay to inspect the notes. Check a long note
and a mine, then resize the window to make sure every lane remains visible.
Try double play or battle if you use those modes.

For an ordinary manual single chart, hold START+SELECT to enter retry choice.
Release START for the same pattern, SELECT for a new pattern, or both to
cancel. The example disables its Exit button while the choice is active.
Courses, autoplay, replay, battle and Arena don't use the standard quick retry.

Play a course through a stage result and confirm that the next stage changes
both title and notes. The final course summary should return to selection
when closed. `coursePlayers` provides combo state for the whole course if
you want to add it, while the example's combo label describes the current stage.

For later customization, the [flow reference](qml/qml-rhythmgameqml-standardgameplayflow.html)
describes `startReady` and `finishReady` for delaying transitions around an
animation. Keep the standard score saving and navigation when adding such effects.

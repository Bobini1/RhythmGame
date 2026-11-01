# 6. Show what is about to play {#theme_tutorial_decide}

[Previous](@ref theme_tutorial_select) | [Tutorial](@ref theme_tutorial) | [Next: Gameplay](@ref theme_tutorial_gameplay)

**Goal:** Show the selected chart or course and let the player start or cancel.

Install `docs/examples/theme-tutorial/06-decide/` and choose it for Decide.

[Download 06-decide](examples/theme-tutorial/06-decide.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/06-decide).

## Receive the play data

The game supplies a [GameplayContext](qml/qml-rhythmgameqml-gameplaycontext.html)
as `gameplay`. Declare it as a required property on the root object.

\snippet 06-decide/Decide.qml context

`gameplay.chartData` describes the current chart. `gameplay.course` contains
the course definition when playing a course and is `undefined` for a single
chart. Check `if (gameplay.course)` before using its name or other course data.

\snippet 06-decide/Decide.qml title

Decide and gameplay receive the same context object.
In a course, the current chart and players change as stages advance.

## Accept or cancel with the standard flow

Pass the supplied context to [StandardDecideFlow](qml/qml-rhythmgameqml-standarddecideflow.html).
It starts gameplay after five seconds. Enter or a bound play key also starts,
while Escape or START+SELECT cancels.

\snippet 06-decide/Decide.qml decide-flow

`pointerEnabled: false` disables the flow's whole-screen mouse area because
the example has explicit buttons. The buttons call `start()` and `cancel()`,
so they share the flow's protection against duplicate transitions.

\include{lineno} 06-decide/Decide.qml

Set `timeoutMillis` to `0` to wait for input. You can change the layout while
keeping the flow. Override `startAction` or `cancelAction` only when you need
to replace the whole transition.

## Understand which screens remain open

The game keeps selection in memory while you play, which preserves its
current folder and focus. Starting play replaces decide with gameplay.
The game later shows a result above gameplay, then resumes gameplay after
the result closes so the flow can advance or return to selection.

Arena rounds open gameplay directly, without decide.
[StandardMultiplayerFlow](qml/qml-rhythmgameqml-standardmultiplayerflow.html)
handles that transition when the session prepares a round.

If you add shortcuts or audio players yourself, bind their behavior to the
screen's `enabled` state. They don't inherit it like child Items do.

## Try starting and cancelling

Try starting once by button and once by waiting for the timeout. Then select
another chart and cancel. Check a course too, if one is available in your
tables. The title should describe the course before its first stage starts.

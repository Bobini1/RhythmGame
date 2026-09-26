# 6. Show what is about to play {#theme_tutorial_decide}

[Previous](05-select.md) | [Tutorial](index.md) | [Next: Gameplay](07-gameplay.md)

Install `docs/examples/theme-tutorial/06-decide/` and choose it for Decide.
Selecting a chart now shows its title with Start and Back buttons. Selecting
a course shows the course name in the same screen.

[Download 06-decide](examples/theme-tutorial/06-decide.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/06-decide).

## Receive the play data

The game supplies `gameplay` when it creates the screen. Declare the required
property on the root object so QML checks that the value is present at creation.
You don't construct a `GameplayContext` yourself.

\snippet 06-decide/Decide.qml context

`gameplay.chartData` describes the current chart. `gameplay.isCourse` tells
you whether the play belongs to a course, and `gameplay.course` provides its
definition. Check `isCourse` before using the course data.

\snippet 06-decide/Decide.qml title

Decide and gameplay receive the same context object. The context gives each
screen access to the play data without exposing the object that runs a chart
or course. In a course, the current chart and players change as stages advance.

## Accept or cancel with the standard flow

Pass the supplied context to `StandardDecideFlow`. Its default timeout starts
play after five seconds. Enter or a bound play key also starts, while Escape
or START+SELECT cancels.

\snippet 06-decide/Decide.qml decide-flow

`pointerEnabled: false` disables the flow's whole-screen mouse area because
the example has explicit buttons. The buttons call `start()` and `cancel()`,
so they share the flow's protection against duplicate transitions.

\include{lineno} 06-decide/Decide.qml

Change `timeoutMillis` to `0` if you want to wait for input. For a different
appearance, edit the layout and keep the flow. A replacement `startAction`
or `cancelAction` takes responsibility for the whole transition, so it is
more work than changing how the screen looks.

## Understand which screens remain open

The game keeps selection in memory while you play, which preserves its
current folder and focus. Starting play replaces decide with gameplay.
The game later shows a result above gameplay, then resumes gameplay after
the result closes so the flow can advance or return to selection.

Arena rounds open gameplay directly, without decide. `StandardMultiplayerFlow`
handles that transition when the session prepares a round.

\image html theme-lifetime.svg "Screen lifetime during a single chart"

The game disables an inactive screen's input, but screen code must still
stop its own audio and disable its own shortcuts. `enabled` is supplied by
the game, so bind behavior to it rather than assigning it yourself. The
assets lesson demonstrates the same rule for preview audio.

Try starting once by button and once by waiting for the timeout. Then select
another chart and cancel. Check a course too, if one is available in your
tables. The title should describe the course before its first stage starts.

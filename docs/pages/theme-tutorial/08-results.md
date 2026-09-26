# 8. Show chart results and course summaries {#theme_tutorial_results}

[Previous](07-gameplay.md) | [Tutorial](index.md) | [Next: Assets and saved scores](09-assets.md)

Install `docs/examples/theme-tutorial/08-results/` and select it for both Result
and Course Result. The folder contains separate entry files for the two roles,
plus a shared score display. You can also enable just one role and keep another
theme for the other.

[Download 08-results](examples/theme-tutorial/08-results.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/08-results).

\include 08-results/theme.json

## Read a completed chart

`Result.qml` receives a `ResultContext`. It describes one completed chart,
including a chart played as a course stage. Its `chartData` and stage fields
stay fixed even if the gameplay context has already advanced to the next stage.

\snippet 08-results/Result.qml result-contract

Use `result.players` to show the participants. Each `ResultPlayer` pairs a
profile with a saved `BmsScore`, so you don't have to match separate profile
and score arrays. The list contains one or two entries, with no null padding.

\snippet 08-results/Result.qml score-delegate

The score's `result` property holds totals such as points and clear type.
The surrounding `BmsScore` also provides replay and gauge history. A completed
score has fixed totals, although its online submission state can still change.

## Read a completed course

`CourseResult.qml` receives `CourseResultContext` for the overall summary.
Its `course` describes the course, and `charts` contains the chart metadata in
stage order. Its players have `BmsScoreCourse` scores that aggregate the course.

\snippet 08-results/CourseResult.qml course-contract

\snippet 08-results/CourseResult.qml course-players

Keep the two root properties typed separately. The game doesn't send a course
summary to a normal result screen as a fallback. Default shares a private
view between its two result files, but another theme can draw them differently.

Both saved score types expose the totals used by `ScoreLine.qml`, so the
example shares that small helper. The roots still receive distinct types.

\include{lineno} 08-results/ScoreLine.qml

## Close a result through the same input handler

Pass either result context to `StandardResultInput`. It delays confirmation
briefly after the screen opens, then handles the standard keys. Your own
Continue button calls `confirm()` and uses the same delay.

\snippet 08-results/Result.qml result-input

\snippet 08-results/Result.qml result-button

For supported single charts, keys 5 and 7 retry with fresh randomization and
the same pattern respectively. The component gets the originating play from
the result context. A course summary has no single-chart retry.

## Include Arena standings

The normal result screen adds `StandardArenaResultOverlay`. Its context
identifies the completed Arena round, so the panel can display the matching
standings and chat. Local results stay unchanged. The result input above
disables confirmation while chat is open, so Enter only sends the message.
The condition uses `result.arenaActive`, which matches the session to this
result without depending on the overlay. Keep the condition when replacing
the panel with your own presentation.

\snippet 08-results/Result.qml arena-result

The course summary does not need an Arena overlay. See the
[complete example](10-shipping.md) for placement and chat options.

\include{lineno} 08-results/Result.qml

\include{lineno} 08-results/CourseResult.qml

Finish a chart and check the title, player name and score. Then play a course
and check that every stage shows a normal result. After closing the last stage
result, the course summary opens. If you use battle, check
that both players appear. The game owns the supplied data, so the theme must
not destroy the context or its scores when leaving the screen.

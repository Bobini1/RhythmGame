# 8. Show chart results and course summaries {#theme_tutorial_results}

[Previous](07-gameplay.md) | [Tutorial](index.md) | [Next: Assets and saved scores](09-assets.md)

Install `docs/examples/theme-tutorial/08-results/` and select it for both Result
and Course Result. The folder contains separate entry files for the two roles,
plus shared displays for scores, judgements and graphs. You can also enable just one role and keep another
theme for the other.

[Download 08-results](examples/theme-tutorial/08-results.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/08-results).

\include 08-results/theme.json

## Read a completed chart

`Result.qml` receives a [ResultContext](qml/qml-rhythmgameqml-resultcontext.html). It describes one completed chart,
including a chart played as a course stage. Its `chartData` and stage fields
stay fixed even if the gameplay context has already advanced to the next stage.

\snippet 08-results/Result.qml result-contract

Use `result.players` to show the participants. Each [ResultPlayer](qml/qml-rhythmgameqml-resultplayer.html) pairs a
profile with a saved [BmsScore](@ref gameplay_logic::BmsScore), so you don't have to match separate profile
and score arrays. The list contains one or two entries, with no null padding.

\snippet 08-results/Result.qml score-delegate

The score's `result` property holds totals such as points and clear type.
The surrounding [BmsScore](@ref gameplay_logic::BmsScore) also provides replay and gauge history. A completed
score has fixed totals, although its online submission state can still change.

## Show judgement counts

`ResultStats.qml` displays the judgement counts and both graphs for one player.
The normal result and course summary pass their scores to the same component.

\snippet 08-results/Result.qml result-stats

Read the counts from `score.result.judgementCounts`, using a `Judgement` value
as the index. The example chooses a label and color for each displayed judgement.
PGREAT uses `Judgement.Perfect`, while EMPTY POOR counts presses that were too
far from a note. MINE counts mines the player hit. Internal events such as
`LnEndSkip` don't need rows in the list.

\snippet 08-results/ResultStats.qml judgement-counts

The totals come from the saved result, so they remain available when a score
has no replay or gauge history. The graph components handle their own missing data.

## Group hits by timing

`HitDistribution.qml` draws a histogram, which groups hits with similar timing
into bars. Early hits appear on the left, and late hits appear on the right.
The middle bar contains hits close to zero deviation. Taller bars contain
more hits.

The graph reads `score.replayData.hitEvents`. It includes Perfect, Great, Good
and Bad judgements. Poor and Empty Poor events don't measure a successful note
hit, so the graph excludes them, along with mines and internal long-note events.
The `deviation` value is in nanoseconds, and dividing it by one million gives
milliseconds.

\snippet 08-results/HitDistribution.qml timing-buckets

The binding builds 41 buckets, with one centered on zero. The outer bucket
centers start at 100 milliseconds early and late. If a hit falls farther away,
the range expands in steps of 50 milliseconds. The label shows the bucket width,
and all included hits fit in the graph.

\snippet 08-results/HitDistribution.qml timing-bars

The graph scales bar heights against the largest count. Its buckets are
calculated when the replay data changes, so resizing only changes the rectangles.
A missing replay shows "Replay data unavailable". A replay with no included
hits shows "No replay data".

## Draw gauge history

`LifeGraph.qml` reads `score.gaugeHistory.gaugeInfo`. Each entry describes one
gauge, including its name, maximum value, clear threshold and history. The
dropdown lets the player inspect each gauge. It starts with the gauge whose
name matches the clear type, or the last gauge in the list if none matches.

\snippet 08-results/LifeGraph.qml gauge-selection

Each history entry records a gauge value and its `offsetFromStart` in
nanoseconds. The value stays constant until the next change, so the graph
uses horizontal segments followed by vertical steps. The time axis covers
the score's recorded length and any later gauge change.

\snippet 08-results/LifeGraph.qml gauge-points

The points use coordinates from zero to one. A
[Canvas](https://doc.qt.io/qt-6/qml-qtquick-canvas.html) draws the line at the
current width and height. It repaints when the points or dimensions change,
while resizing keeps the same list of points. The horizontal reference line
marks the clear threshold,
and the label gives the gauge's range. A score without gauge history shows
"Gauge history unavailable".

\snippet 08-results/LifeGraph.qml gauge-path

## Keep the result usable in a small window

The statistics use three columns when there is enough width, and stack
vertically in a narrow window. A `ScrollView` holds the player displays, so
battle results can show both players without shrinking the graphs. Retry and
Continue stay outside the scroll area.

During Arena play, the normal result uses the left half of the screen, leaving
room for the Arena panel's default position on the right.

## Show each player's online rank

The result uses the same
[StandardRanking](qml/qml-rhythmgameqml-standardranking.html) that the select
controller supplies. Here, each player's display creates its own instance
with the result's chart and that player's profile and score.

\snippet 08-results/Result.qml result-ranking

The dropdown beside the rank shows the provider's name and lets the player
choose RhythmGame, Bokutachi or LR2IR. Its `currentValue` follows
`playerRanking.provider`, and `onActivated` saves the choice in that player's
`profile.vars.generalVars.rankingProvider`. The component follows the saved
choice and refreshes when the score's submission state changes.

The controls use two columns in a narrow window, with Open ranking below
the provider and rank. Wider windows keep all three controls on one row.

The label reads the current position, and Open ranking opens the selected
provider's website. `previousPosition` is also
available if you want to compare the earlier best score with the current
opponents. It is not a historical snapshot of everyone else's ranks.

Normal results for course stages can use a ranking. Course summaries omit
it because the online services rank individual charts. Arena standings are
separate from online rankings and use the overlay described below.

## Read a completed course

`CourseResult.qml` receives [CourseResultContext](qml/qml-rhythmgameqml-courseresultcontext.html) for the overall summary.
Its `course` describes the course, and `charts` contains the chart metadata in
stage order. Its players have [BmsScoreCourse](@ref gameplay_logic::BmsScoreCourse) scores that aggregate the course.

For a saved course result, a chart may have been removed from the library.
`result.charts[index]` is then null, while `result.course.chartIds[index]` still
identifies that stage. Look up that identifier with `Rg.tables.search()` for
a table title, or display the identifier itself. Keep the stage in the list.
The course's recorded stages and combined graph data are still available.

\snippet 08-results/CourseResult.qml course-stages

\snippet 08-results/CourseResult.qml course-contract

\snippet 08-results/CourseResult.qml course-players

Keep the two root properties typed separately. The game doesn't send a course
summary to a normal result screen as a fallback. Default shares a private
view between its two result files, but another theme can draw them differently.

Both saved score types expose the totals used by `ScoreLine.qml` and
`ResultStats.qml`. A course score also combines its stages' hit events and
gauge history, so the graphs work without changing their bindings. The course
timing graph includes hits from all recorded stages, and its life graph uses
the combined course timeline. The roots still receive distinct types.

\include{lineno} 08-results/ScoreLine.qml

\include{lineno} 08-results/ResultStats.qml

\include{lineno} 08-results/HitDistribution.qml

\include{lineno} 08-results/LifeGraph.qml

## Close a result through the same input handler

Pass either result context to [StandardResultInput](qml/qml-rhythmgameqml-standardresultinput.html). It delays confirmation
briefly after the screen opens, then handles the standard keys. Your own
Continue button calls `confirm()` and uses the same delay.

\snippet 08-results/Result.qml result-input

\snippet 08-results/Result.qml result-button

For supported single charts, keys 5 and 7 retry with fresh randomization and
the same pattern respectively. The component gets the originating play from
the result context. A course summary has no single-chart retry.

The Retry and Same random buttons call the same method with a pattern choice.
`retryAvailable` includes the input delay and the restrictions on courses,
autoplay, replay, battle and Arena, so the theme doesn't repeat those checks.

\snippet 08-results/Result.qml result-retry

## Include Arena standings

The normal result screen adds [StandardArenaResultOverlay](qml/qml-rhythmgameqml-standardarenaresultoverlay.html). Its context
identifies the completed Arena round, so the panel can display the matching
standings and chat. Local results stay unchanged. StandardResultInput
disables confirmation while this result's chat is open, so Enter only sends
the message. This works with custom Arena panels too. Override `confirmEnabled`
only when your theme needs another confirmation rule.

\snippet 08-results/Result.qml arena-result

The course summary does not need an Arena overlay. See the
[complete example](10-shipping.md) for placement and chat options.

\include{lineno} 08-results/Result.qml

\include{lineno} 08-results/CourseResult.qml

Finish a chart and check the title, player name, score and judgement counts.
Compare the timing graph with a few deliberately early and late hits, then
switch gauges in the life graph. Resize the window and scroll through the
statistics. Open an imported score to check the messages for missing graph data.
Then play a course
and check that every stage shows a normal result. After closing the last stage
result, the course summary opens. If you use battle, check
that both players appear. The game owns the supplied data, so the theme must
not destroy the context or its scores when leaving the screen.

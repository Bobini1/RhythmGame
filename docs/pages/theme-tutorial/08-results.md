# 8. Show chart results and course summaries {#theme_tutorial_results}

[Previous](@ref theme_tutorial_gameplay) | [Tutorial](@ref theme_tutorial) | [Next: Complete theme example](@ref theme_tutorial_shipping)

**Goal:** Show scores, judgement counts and graphs for a completed chart or
course, with working Continue and Retry controls.

Install `docs/examples/theme-tutorial/08-results/` and choose it for Result and
Course Result. Each screen has its own QML file and can be selected separately.

[Download 08-results](examples/theme-tutorial/08-results.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/08-results).

\include 08-results/theme.json

## Read a completed chart

`Result.qml` receives a [ResultContext](qml/qml-rhythmgameqml-resultcontext.html)
for one completed chart, including a course stage. Its `chartData` and stage
fields stay fixed when gameplay advances to the next stage.
The game owns the context and its scores for the result screen's lifetime; the
theme presents them without destroying them.

\snippet 08-results/Result.qml result-contract

`result.players` contains one or two participants. Each
[ResultPlayer](qml/qml-rhythmgameqml-resultplayer.html) pairs a profile with a
saved [BmsScore](@ref gameplay_logic::BmsScore).

\snippet 08-results/Result.qml score-delegate

The score's `result` property holds totals such as points and clear type.
The score also provides replay and gauge history. Its totals stay fixed,
although its online submission state can still change.

## Add Continue and Retry controls

Pass `result` to [StandardResultInput](qml/qml-rhythmgameqml-standardresultinput.html).
It accepts both chart and course results. It briefly delays confirmation after
the screen opens, then handles the standard keys. The Continue button calls
`confirm()` and uses the same delay.

\snippet 08-results/Result.qml result-input

\snippet 08-results/Result.qml result-button

For supported single charts, keys 5 and 7 retry with fresh randomization and
the same pattern respectively. The component gets the originating play from
the result context. A course summary has no single-chart retry.

The Retry button calls `retry(false)`; Same random calls `retry(true)`.
`retryAvailable` includes the input delay and the restrictions on courses,
autoplay, replay, battle and Arena, so the theme doesn't repeat those checks.

\snippet 08-results/Result.qml result-retry

## Show judgement counts

`ResultStats.qml` displays the judgement counts and both graphs for one player.
The normal result and course summary pass their scores to the same component.

\snippet 08-results/Result.qml result-stats

Read the counts from `score.result.judgementCounts`, using a `Judgement` value
as the index. The example chooses a label and color for each displayed judgement.
PGREAT uses `Judgement.Perfect`. EMPTY POOR counts presses too far from a note,
and MINE counts mines hit. Some events such as `LnEndSkip` (the end of a
long note whose start was missed) can be skipped in most cases.

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
hit, so the graph excludes them, along with mines and the special long-note events.
The `deviation` value is in nanoseconds, and dividing it by one million gives
milliseconds.

\snippet 08-results/HitDistribution.qml timing-buckets

The graph uses the `Bad` window from `ChartData.timingWindows` as its outer
limit. Its 41 bars use fixed timing intervals: one centered on zero and 20 on
each side. This gives every score for the same chart the same scale.

\snippet 08-results/HitDistribution.qml timing-bars

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
[Canvas](https://doc.qt.io/qt-6/qml-qtquick-canvas.html) scales them to its current
size and repaints when the points or size change. A horizontal line marks the
clear threshold. A score without gauge history shows
"Gauge history unavailable".

\snippet 08-results/LifeGraph.qml gauge-path

## Keep the result usable in a small window

The statistics use three columns when there is enough width, and stack
vertically in a narrow window. A `ScrollView` holds the player displays, so
battle results can show both players without shrinking the graphs. Retry and
Continue stay outside the scroll area.

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

The label shows the current position; Open ranking opens the provider's website.
`previousPosition` compares the player's earlier best score against the current
opponents. It is not a historical ranking.

Normal results for course stages can use a ranking. Course summaries omit
it because the online services rank individual charts. Arena standings are
separate from online rankings and use the overlay described below.

## Read a completed course

`CourseResult.qml` receives a
[CourseResultContext](qml/qml-rhythmgameqml-courseresultcontext.html).
Its `course` describes the course, and `charts` contains chart metadata in
stage order. Each player's [BmsScoreCourse](@ref gameplay_logic::BmsScoreCourse)
combines their stage scores.
The combined hit graph uses the widest available Bad window from the course's
stages, so it can show every included hit on one fixed scale.

\snippet 08-results/CourseResult.qml course-contract

For a saved course result, a chart may have been removed from the library.
`result.charts[index]` is then null, while `result.course.chartIds[index]` still
identifies that stage. Look up that identifier with `Rg.tables.search()` for
a table title, or display the identifier itself. Keep the stage in the list.
The course's recorded stages and combined graph data are still available.

\snippet 08-results/CourseResult.qml course-stages

\snippet 08-results/CourseResult.qml course-players

Both score types provide the totals used by `ScoreLine.qml` and `ResultStats.qml`.
A course score combines hit events and gauge history across its stages, so
the same graphs work for both screens.

\include{lineno} 08-results/ScoreLine.qml

## Include Arena standings

Add [StandardArenaResultOverlay](qml/qml-rhythmgameqml-standardarenaresultoverlay.html)
to show standings and chat for the completed Arena round. It stays hidden for
local results. `StandardResultInput` disables confirmation while this result's
chat is open, so Enter sends the message without closing the screen.

\snippet 08-results/Result.qml arena-result

The course summary does not need an Arena overlay.

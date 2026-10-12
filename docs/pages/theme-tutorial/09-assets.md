# 9. Load images, previews and saved scores {#theme_tutorial_assets}

[Previous](08-results.md) | [Tutorial](index.md) | [Next: Share a theme](10-shipping.md)

Install `docs/examples/theme-tutorial/09-assets/` and choose it for Select.
It extends the earlier list with a chart image, an online rank and saved-score
information. The score count, best clear and best score stay visible with the
image as you move between charts. A hit distribution beside them shows the
last score.

[Download 09-assets](examples/theme-tutorial/09-assets.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/09-assets).

## Use the chart's image source

`chartData.stageFileSource` is ready to use as an image URL, including for
charts stored in archives. The example shows "No image" while no image is
available. An absent image is normal for many charts.

\snippet 09-assets/Select.qml chart-image

Use relative URLs for files supplied with your theme, as the first menu did.
For a raw local path, use `Rg.fileQuery.localFileUrl(path)` instead of adding
`file:///` yourself. Characters such as `#` and `?` have special meanings in
URLs. For assets inside songs, prefer the chart's source properties or
`Rg.songAssets` so archive paths continue to work.

## Use the controller's preview player

The controller already includes a
[StandardSelectPreview](qml/qml-rhythmgameqml-standardselectpreview.html)
as `preview`. It uses the focused chart and discovered preview files, and waits
300 milliseconds before playing, so moving through the list doesn't start
every preview. The binding inside the controller holds a pending preview
until scrolling stops.

\snippet 09-assets/Select.qml preview

When a chart has no preview file, the player uses its chart audio without
loading BGA. Set `preview.chartAudioEnabled: false` on the controller to use
preview files only. A folder or course has no preview. Change
`preview.delayMillis` or `preview.volume` to adjust the delay or volume.

The controller's `backgroundMusic` uses the selection track from the active
profile's BGM set. It stops while a preview plays and returns after 500
milliseconds without a playing preview. The delay avoids restarting music
between quick chart changes. Change `backgroundMusicDelayMillis` to adjust it.
You can also change the player's `source`, `volume` and `fadeInMillis`.

Set `preview: null` or `backgroundMusic: null` on the controller to omit either
player. Disabling the controller stops both kinds of audio and cancels any
preview load, since selection remains in memory during gameplay.

For a selector built without the controller, use StandardSelectPreview on its
own. Supply a `selectState`, or set `chartData` and `previewSource` directly.
Its `playing` property tells you when to stop your own background music.

## Read saved results without loading recordings

The controller's `results` map contains saved `BmsResult` objects for charts and
`BmsResultCourse` objects for courses. The keys are chart MD5s or course
identifiers. Each list is ordered from newest to oldest. An unknown key has no
list, so use `|| []` when reading it.

Results hold points, clear types, judgement counts and the settings used for a
run. They do not hold hit events or gauge histories. The game keeps results in
memory for each profile and updates them after saves and imports. Opening a
folder or changing the sort order can use them straight away.

The example binds `screen.chartResults` to
`selection.results[selection.focusedItem?.md5] || []` and uses its length for the
score count. The binding follows the focused chart and the active profile.
The ranking and replay controls use the same cached results.

The controller's [StandardRanking](qml/qml-rhythmgameqml-standardranking.html)
already provides `bestClearTypeResult` and `bestPointsResult` from the local
results. They can refer to different runs. The best score is the run with the
highest percentage of available points. The labels read each result's
`clearType` or `points`, using `NOPLAY` and zero when there are no saved results.
Reading these values doesn't load a recording or wait for an online response.

\snippet 09-assets/Select.qml saved-results

For a screen without a selection controller, bind to
`profile.scoreDb.chartResults[md5.toUpperCase()] || []` or
`profile.scoreDb.courseResults[identifier] || []`. Both maps update bindings
automatically after saves and imports. Result objects keep their identity for
the lifetime of the profile, so a panel can keep its selected result when a
new run is saved. Imports update an existing result's properties in place.

If a run is removed from the database, it disappears from the maps. An already
held result stays readable until the profile closes, but loading its full score
will fail if that run is no longer stored.

## Load a recording for a hit distribution

The hit distribution needs the individual hit events from a full score.
[ScoreDetails](qml/qml-rhythmgameqml-scoredetails.html) loads that score from a
saved result. The example uses the first result in `screen.chartResults`, which
is the latest run. It may differ from the runs behind the best clear and score.

\snippet 09-assets/Select.qml score-details

ScoreDetails uses the active profile by default. Changing its result or profile
cancels the previous request, so moving to another chart won't show the old
recording. It inherits the screen's `enabled` state and releases its score
when the screen becomes inactive.

The example uses the same `HitDistribution.qml` as the [result lesson](08-results.md).
It passes the loaded score's `replayData` to the graph. The heading identifies
the last score and shows the chart's judge rank. The graph keeps its
space while loading, so the song list doesn't move when the recording arrives.

\snippet 09-assets/Select.qml recording-graph

The judge rank describes the chart's timing difficulty. `ChartData.rank` uses
25 for Very hard, 50 for Hard, 75 for Normal and 100 for Easy. Other values
appear as "Custom" with the number. The rank comes from the selected chart's
metadata because saved results don't store it separately. Without that
metadata, the label shows "Unknown".

\snippet 09-assets/Select.qml judge-rank

A saved score doesn't always have a recording. For example, an imported score
may only contain the result. The graph shows a separate message for a missing
recording, a loading error or a chart with no saved scores. An available
recording with no timed hits shows "No replay data".

The helper calculates the distribution in JavaScript from the loaded events.
PGREAT, GREAT, GOOD and BAD contribute to the graph. Negative deviations are
early hits and positive deviations are late hits.

\snippet 09-assets/HitDistribution.qml timing-buckets

The normal replay and saved-result actions load their own full scores. You
don't need a separate ScoreDetails component for those actions.

## Show the online ranking

The controller's `ranking` follows the focused chart and the profile's chosen
provider. Bind a label to its `loading`, `position` and `playerCount` properties.
The display updates when a request finishes or the selection changes.
The dropdown beside the label shows the provider's name and lets the player
choose RhythmGame, Bokutachi or LR2IR. It saves the choice in
`selection.ranking.profile.vars.generalVars.rankingProvider`, so the next
screen uses the same provider.

\snippet 09-assets/Select.qml select-ranking

The button and F11 both call `selection.openInternetRanking()`. Tachi needs
to look up its chart ID before opening a link. The shared component handles
that lookup and cancels it if you move to another chart or leave the screen.

For a leaderboard, use `selection.ranking.entries` as your view's model.
Each entry includes `userName`, `bestPoints`, `maxPoints` and `bestClearType`.
The entries are ordered by score percentage. LR2IR also includes the active
profile's local best score, since it cannot identify that profile's LR2 account.
`position` is one-based, and zero means the profile has no ranking entry.

[StandardRanking](qml/qml-rhythmgameqml-standardranking.html) also works on its
own, as the [result lesson](08-results.md) shows. Override `ranking.provider`
to use a particular provider, or set `ranking.entriesEnabled` to false when
you only want to open links. `openInternetRankingAction` replaces the
controller's website action, for example with a panel supplied by your theme.

Browse quickly between charts, then enter gameplay while a preview is playing.
The preview should stop, and the saved-score labels and hit distribution should
follow the focused chart when you return. Also check an imported score without
a recording, a chart without saved scores and a folder. Missing data should
leave the list usable.

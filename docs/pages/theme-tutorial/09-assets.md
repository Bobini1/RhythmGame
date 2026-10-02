# 9. Load images, previews and saved scores {#theme_tutorial_assets}

[Previous](08-results.md) | [Tutorial](index.md) | [Next: Share a theme](10-shipping.md)

Install `docs/examples/theme-tutorial/09-assets/` and choose it for Select.
It extends the earlier list with a chart image, an online rank
and a button that shows saved results and timing statistics for the focused chart.

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

The example passes `selection.results[screen.chartData?.md5] || []` to
`OptionalDetails`. The binding follows the focused chart and the active profile.
The ranking and replay controls use the same cached results.

For a screen without a selection controller, bind to
`profile.scoreDb.chartResults[md5.toUpperCase()] || []` or
`profile.scoreDb.courseResults[identifier] || []`. Both maps update bindings
automatically after saves and imports. Result objects keep their identity for
the lifetime of the profile, so a panel can keep its selected result when a
new run is saved. Imports update an existing result's properties in place.

If a run is removed from the database, it disappears from the maps. An already
held result stays readable until the profile closes, but loading its full score
will fail if that run is no longer stored.

## Load a recording when you need it

[ScoreDetails](qml/qml-rhythmgameqml-scoredetails.html) takes a saved result and
loads its full score. It uses the main profile by default; set `profile` when
showing another player's history. The example loads the latest run only while
the details panel is open.

\snippet 09-assets/OptionalDetails.qml score-details

`score` is null while loading or after an error. Once ready, it is a `BmsScore`
for a chart or a `BmsScoreCourse` for a course. Imported scores may have no
recording or gauge history, so check those properties before using them.
Course scores provide the individual stages in `scores` and combined
`replayData` and `gaugeHistory` for drawing a graph across the course.

Changing the result or profile cancels this component's old request. Disabling
or destroying it also cancels the request and releases its score. Another
component loading the same run keeps its own request. The game shares decoded
data and keeps a limited cache for reuse.

You can calculate statistics in JavaScript from the loaded events. This example
counts early hits that removed a note. `deviation` is in nanoseconds: negative
means early, positive means late. Choose the judgements and events that make
sense for your display.

\snippet 09-assets/OptionalDetails.qml score-statistics

The normal replay and saved-result actions handle loading themselves. Pass a
result to `selection.openResult(item, result)` to open it; themes do not need a
separate ScoreDetails for that action.

## Show the online ranking

The controller's `ranking` follows the focused chart and the profile's chosen
provider. Bind a label to its `loading`, `position` and `playerCount` properties.
The display updates when a request finishes or the selection changes.

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

## Load optional content when it is opened

`OptionalDetails.qml` uses a checkable button to keep track of whether the
details are open. The `Loader` binds `active` to the button's `checked` state
and the screen's `enabled` state. Closing the details or leaving the screen
destroys the panel and its ScoreDetails. `Connections` unchecks the button when the main profile
changes.

\include{lineno} 09-assets/OptionalDetails.qml

F7 toggles the details. The shortcut explicitly follows `enabled`, because
shortcuts don't automatically stop when a retained screen loses input.

Browse quickly between charts, then enter gameplay while a preview is playing.
The preview should stop, and details should show the newly focused chart's
score and statistics when you return. Also check a folder and a chart without an image
or preview. Their absence should leave the list usable.

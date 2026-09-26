# 9. Load images, previews and saved scores {#theme_tutorial_assets}

[Previous](08-results.md) | [Tutorial](index.md) | [Next: Share a theme](10-shipping.md)

Install `docs/examples/theme-tutorial/09-assets/` and choose it for Select.
It extends the earlier list with a chart image, preview audio and a button
that reveals the number of saved scores for the focused chart.

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

## Start preview audio only while selection is active

The controller discovers preview files and provides them by chart directory.
The helper waits 300 milliseconds before starting a preview, which avoids
starting audio for every row passed during quick scrolling. Charts without a
preview file stay silent in this example.

\snippet 09-assets/Select.qml assets-state

\include{lineno} 09-assets/PreviewAudio.qml

The timer's source follows the preview URL while the screen is enabled, and
becomes empty when the screen is inactive. A new URL clears `delayedSource`
and restarts the delay. The timer sets `delayedSource` when the delay ends,
and the player loads that source through a binding.

The delay needs a stored value because it depends on time as well as the
current URL. The timer's source and the player's `playing` state can still
use ordinary bindings. When selection becomes inactive, the timer stops and
the player clears its source. The screen may remain in memory while you play, so
destruction alone is too late to stop its audio.

## Cancel score requests you no longer need

Score queries finish asynchronously, after the call returns. A request can
still be pending when the reader focuses another chart or leaves selection.
`PendingReplyGroup` groups replies so the helper can cancel the old requests
before starting another one.

\include{lineno} 09-assets/AsyncScores.qml

`refresh()` first cancels pending replies and clears the displayed count. It
then requests scores for the current MD5 when the screen is enabled.
`getScoresForMd5()` accepts a list, so the example passes `[md5]` for one chart.
The completion handler reads the matching entry from the returned score map.
The group also cancels retained replies when it is destroyed.

The helper uses change handlers because a query creates work that must be
cancelled when its inputs change. Its result arrives later through a callback.
Keep ordinary display values as bindings, and use handlers where an operation
needs to start, finish or be cancelled.

The example disables the controller's `scoresEnabled` option because the
helper demonstrates its own score query. Ordinary selectors can use the
controller's `scores` directly instead of adding this helper. Preview discovery
continues to use the controller.

## Load optional content when it is opened

`OptionalDetails.qml` uses a checkable button to keep track of whether the
details are open. The `Loader` binds `active` to the button's `checked` state
and the screen's `enabled` state. Closing the details or leaving the screen
destroys the label. `Connections` unchecks the button when the main profile
changes.

\include{lineno} 09-assets/OptionalDetails.qml

F7 toggles the details. The shortcut explicitly follows `enabled`, because
shortcuts don't automatically stop when a retained screen loses input.

Browse quickly between charts, then enter gameplay while a preview is playing.
The preview should stop, and details should show the newly focused chart's
score count when you return. Also check a folder and a chart without an image
or preview. Their absence should leave the list usable.

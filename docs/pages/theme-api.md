# Theme API {#theme_api}

The theme API covers QML screens and the game objects they use. It does not
provide a C++ plugin ABI. Start with [the tutorial](theme-tutorial/index.md) or
[the screen contracts](../../DEV_THEME.md).

## Screen roles

Each screen role is selected independently. A theme can omit roles and use
Default for them. `result` and `courseResult` are separate roles; a missing
course result does not fall back to a normal result.

## What is supported

The supported surface consists of:

- The manifest and settings schemas, screen role names, and required screen
  inputs described in `DEV_THEME.md`.
- The documented properties, signals and methods of `Standard*`, `ScoreDetails`
  and `PendingReplyGroup`. These components are optional. Their internal child
  objects and undocumented working state are not extension points.
- `GameplayContext`, `ResultContext`, `CourseResultContext` and their participant
  types. Custom gameplay flows use the context's lifecycle methods.
- `Rg` and the public properties, model roles, signals and invokable methods of
  the services and data objects reached through it. `Profile` is the profile
  type name. `chartLoader` exposes metadata lookup through `loadChartData` and
  `loadChartDataFromDb`; native runner creation and retry methods are internal.
- The public data and presentation types used with those objects, including
  `ChartData`, `Player`, notes, score and replay data, gauges, table values,
  `AudioPlayer`, `ChartAudioPlayer`, `ScoreReplayer`, `Input` and `QmlUtils`.
  Use `ChartRunner` only for its status constants. Native `ChartRunner` and
  `CourseRunner` lifecycle methods are internal.
- The navigation and window operations on `globalRoot` documented in
  `DEV_THEME.md`, including `currentScreen`. Screen objects returned by navigation
  are opaque unless that screen's author explicitly defines an interface.

`ContentFrame`, its scene stack, `ScreenContexts`, `GameplaySession`, the Arena
panel and placement implementation, `FontResolver`, and every LR2 implementation
type are private. So are files inside Default. A type appearing in generated
documentation or being reachable from QML does not make its internals supported.
Do not import another theme's files as a game API.

## Data and lifetime

The host owns supplied screen contexts. Local gameplay owns its runner. Arena
owns its runner separately. A result retains chart metadata; newly played scores
keep their live upload status, while saved-history views receive independent
copies. A course stage remains current until `gameplay.nextStage()` succeeds.
Do not retain a stage's `Player` after advancing.

All gameplay themes support courses. Each stage opens a normal result. Closing
the final stage result opens the separate course summary. Saved course results
keep a null metadata slot for each uninstalled chart, with its identifier at the
same index in `course.chartIds`. These identifiers accept MD5 or `sha256/<hash>`.
Fields explicitly named `md5` and `sha256` still contain the actual algorithm's
hash; they are not interchangeable identifiers.

Asynchronous score requests return `PendingReply`. Retaining a reply retains its
value, and retaining the delivered score keeps that score usable independently
of the reply. Themes never need a native `QJSValue` or a manual ownership step.
`ScoreDetails` manages a request for a component and exposes its loaded score.
The profile owns the resident `BmsResult` and `BmsResultCourse` cache and refreshes
it after saving or importing. Full replay and gauge data are loaded on demand.

Course full scores include individual stages and combined events and gauges.
Combined timestamps include each preceding stage's length. Note indices and
columns still identify notes within their original stage; they are not global
course note IDs. Use individual stage scores when matching events to notes.

## Values, labels and units

Use enum values and identifiers for stored choices. Shared sorting and replay
labels are translated defaults for presentation, and using them is optional.
A theme can supply its own text for any entry without changing shared state.
Translated text is never a stable identifier. Clear type strings remain data,
with presentation controlled by the theme.

New options may be added to the game. A theme can show all
provided options, or offer a deliberate subset. Do not reset a saved option
merely because a screen does not offer it. Show its shared label when available
or an appropriate unknown-value presentation. A user action selects the next
supported value; it must not depend on a fixed count of native enum members.

Chart and course `length`, event timestamps and timing deviations are in
nanoseconds. Negative timing deviation is early; positive deviation is late.
`Time.position` includes scroll changes; `Time.beatPosition` does not.
`noteScreenTimeMillis` is in milliseconds and must be finite and at least 1.
Audio player length is in seconds. Gameplay startup delays are in milliseconds.

Replay timing arrays use this order: Perfect, Great, Good, Bad, Poor, EmptyPoor.
Zero deviation is included in the late array. `totalEarly` and `totalLate` sum
only the first four buckets. These values are calculated from the loaded replay,
not cached in the score database. Themes can calculate different statistics from
`hitEvents` in JavaScript.

Arena standings expose `progress` as a fraction from 0 to 1 and `gaugeValue`
as gauge points from 0 to 100. Protocol scaling stays inside the engine.
`hasScore` distinguishes missing telemetry from a valid zero score.
`Rg.arenaRoomCount` is -1 when the directory is unavailable. The passive
directory's session object is private. Use `canSelect` and `canReady`
independently; readiness does not promise song-selection permission.

`OnlineRankingModel` filters and sorts before applying `offset` and `limit`.
Limit 0 means no limit. `rankingEntries` and `clearCounts` describe the filtered
entries before that slice. `playerCount` and `scoreCount` are provider totals,
and -1 means unavailable. A provider without per-player play counts supplies -1
for that entry too. Date sorting uses the provider's available date, which is
the personal-best date for Tachi. A missing date is 0. Removed or unsupported
provider fields are not fabricated from unrelated values.

## Files and overlays

Resolve relative theme assets in the requesting component with
`Qt.resolvedUrl()`. `AudioPlayer.source` accepts local paths, local file URLs,
and virtual song audio URLs. It reports requested playback through `playing`;
use `loaded` to check that audio is available. Clearing or resetting its source
stops playback and resets length. `FileQuery.readTextFile()` honors a BOM and
otherwise uses UTF-8. Legacy readmes request CP932 explicitly.

Preview lookup can inspect the song directory synchronously. Audio decoding and
virtual-asset reads have asynchronous paths; this is not a promise that all
preview work happens off the UI thread.

An Arena overlay's visual parent is its viewport. Set its parent to choose the
coordinate space. The standard placement component reserves theme setting keys
beginning with `arenaOverlay` for saved placement, size and chat preferences.
Use another prefix for unrelated settings. Its drag and resize working state is
private. The panel component remains replaceable.

## Reviewing API changes

Check how changes to documented members, units, ownership or screen inputs
affect themes. Run the lifetime, model and screen-flow tests. Check new sorting
modes, clear types and Arena policies in Default and the installable tutorial
examples too.

# Theme Development

Start with the [theme tutorial](docs/pages/theme-tutorial/index.md) for a sequence
of installable examples, from a main menu to gameplay and results. Use this
page to look up the theme contract and the
[architecture notes](docs/pages/theme-architecture.md) to understand the design.

If you want to edit a theme, you only need a text editor.
Edit the scripts and relaunch the game to see your changes.

All errors are printed to log.txt in the data folder. On platforms other than Windows,
the log is printed to the console as well.
You can also press F10 in-game to open the log overlay.

# Theme structure

A theme is a folder in `share/RhythmGame/themes/` that contains a `theme.json` file.
That file needs to have a `scripts` field with an object
containing relative paths to QML files for each game screen implemented by the theme.

Here is an example of a minimal `theme.json` file:

```json
{
  "scripts": {
    "result": "result.qml"
  }
}
```

A theme can replace any subset of the game's screens. Players choose a theme
for each role, so you can share just a result screen or a whole set of screens.

### Persistent settings

To provide configurable theme settings that will be stored separately for each user profile,
you can define a `settings` field in `theme.json`.

```json
{
  "scripts": {
    "result": "result.qml"
  },
  "settings": {
    "result": "result.json"
  }
}
```

It serves two purposes

- Defines variables of the screen
- Is used for generating a settings page in the settings screen

For persistent global settings affecting all profiles, you can use the
[Settings QML type](https://doc.qt.io/qt-6/qml-qt-labs-settings-settings.html).
But you probably won't need it. I only ever used it once, for storing screen resolution.

There are 8 different types of properties you can use in a settings file:

- boolean
- range
- string
- file
- font
- choice
- color
- hidden

You can also put them in groups that have names and descriptions.

The usage of most property types is self-explanatory, simply take a look at the default theme
to find out how to use them.

The special `hidden` type can be used to declare properties that will not be shown
in the auto-generated settings page.

You can access the values assigned to properties via
`profile.vars.themeVars[screen][themeName].varId`.

You can get the current theme's name with
[QmlUtils.themeName](https://bobini1.github.io/RhythmGame/classqml__components_1_1QmlUtilsAttached.html).
As for `screen`, your theme should be able to figure it out on its own. It can be `"result"`, `"k7"`, etc.

---
**NOTE**

You should not hard-code the name of your theme anywhere in your theme.
The actual name of a theme in the game is always the name of its folder.
Your theme should not break if a user renames the folder. Use `QmlUtils.themeName`.

---

Please don't access vars that don't belong to the current screen.
It creates hidden dependencies on other themes or screens of your theme that might not be enabled.

If you need to pass a variable between screens, suggest adding it to `profile.vars.generalVars`
in the engine.

### Overriding the settings page for a screen

You can override the auto-generated settings page for a screen by providing a QML file in a
`settingsScripts` field in `theme.json`.

```json
{
  "scripts": {
    "result": "result.qml"
  },
  "settings": {
    "result": "result.json"
  },
  "settingsScripts": {
    "result": "resultSettings.qml"
  }
}
```

The default theme does not use this feature.

### Translations

Translations are described in [DEV_LANG.md](DEV_LANG.md).

```json
{
  "scripts": {
    "result": "result.qml"
  },
  "settings": {
    "result": "result.json"
  },
  "settingsScripts": {
    "result": "resultSettings.qml"
  },
  "translations": {
    "en": "en.qm",
    "pl": "pl.qm",
    "zh-Hans": "zh_Hans.qm",
    "zh-Hant-HK": "zh_Hant_HK.qm"
  }
}
```

Translation keys use BCP 47 language tags with optional script and territory
subtags. See [DEV_LANG.md](DEV_LANG.md) for matching and compatibility details.

## Theme loading

The game validates manifests and settings at startup. It skips themes with
invalid settings and reports the theme and file in `log.txt`. Other themes
remain available. Restart after changing a manifest or QML file.
See [Theme API](docs/pages/theme-api.md) for the supported interfaces.

The part of the game that manages the theme most directly is
[RhythmGameQml/ContentFrame.qml](https://github.com/Bobini1/RhythmGame/blob/master/RhythmGameQml/ContentFrame.qml).
That qml file is compiled into the executable and is not part of any theme.
It contains the window of the entire game and exposes the supported screen-flow
operations through the globally accessible `globalRoot` object. For example,
use `globalRoot.openSelect()` to enter selection and
`globalRoot.returnToPreviousScreen()` to leave the current screen.

`ContentFrame` owns the underlying screen stack. Themes should use the semantic
`globalRoot` operations instead of accessing that stack directly. This keeps a
theme independent of the details of screen lifetime, retry, and Arena flow.

Custom flows can use these navigation operations:

- `currentScreen` is the currently presented screen.
- `openGameplay(gameplay)` pushes gameplay above the current screen.
- `replaceGameplay(gameplay, previousGameplay)` replaces the screen holding that
  previous context and anything above it. Omitting `previousGameplay` replaces
  the current screen. It returns
  the new screen, or `null` if creation fails or the target is unavailable.
  The replacement is created before removing the old screens, so a broken
  gameplay theme leaves the current screen intact.

Decide uses `replaceGameplay(gameplay)` to hand off to play. Arena uses
`openGameplay(gameplay)` to retain the screen underneath. The operation chooses
the stack change; it does not depend on the play mode.

Runner lifetime follows the local screen: decide owns it until gameplay takes
over; gameplay retains it while results are shown, and destroys it when removed
or replaced. `ContentFrame` handles this even when a theme omits the standard
input components. Themes must not destroy these runners themselves. When
`gameplay.isArena` is true, runner ownership stays with Arena.
`StandardMultiplayerFlow` releases its prepared runner when gameplay leaves
the stack. `ContentFrame` does not manage retry input or clean up prepared rounds.

---
**NOTE**

When going from `select` to `k7` and then `result`, the former screens are not
destroyed immediately. `ContentFrame` keeps those screens alive while newer
screens are presented and restores or closes them as the flow requires.

---

## Initial state of screens

`globalRoot` provides the following operations for screens that require initial
state:

```qml
function openChart(path, players = [{profile: Rg.profileList.mainProfile}])
function openCourse(course, players = [{profile: Rg.profileList.mainProfile}])
function openResult(result)
function openCourseResult(result)
function openSavedResult(score, profile, chartData)
function openSavedCourseResult(score, profile, course)
function openSettings(section = "")
```

These operations return the opened screen, or a falsy value if creation fails.
`players` is an array of one or two objects. Each has a required `profile` and
optional `autoplay`, `replay`, and `score` fields. Booleans default to false and
`score` defaults to null. Set `replay: true` with a full score to watch its
recording. With `replay: false`, a score supplies its saved note pattern for
G-Battle. Autoplay and replay are separate modes and cannot be combined.

For example, `globalRoot.openChart(chart.path)` starts manual play with the
main profile. `globalRoot.openChart(chart.path, [{profile, replay: true, score}])`
starts a replay. The Standard selection actions load full saved scores before
calling these operations.

`openResult` takes the `ResultContext` returned by `gameplay.finishStage()`.
`openCourseResult` takes the `CourseResultContext` returned by
`gameplay.finishCourse()`. Each context keeps the completed data available if
screen creation fails. The two `openSaved...` methods take full saved scores;
they copy the data needed by the new screen. Use `selection.openResult()` for
the usual saved-history action, including loading that full score.

Keep the following in mind when writing screens with initial state:

Each data-bearing screen declares one typed input:

| Screen role       | Required declaration                           |
|-------------------|------------------------------------------------|
| `settings`        | `required property string initialSection`     |
| `decide`          | `required property GameplayContext gameplay`   |
| All gameplay keys | `required property GameplayContext gameplay`   |
| `result`          | `required property ResultContext result`       |
| `courseResult`    | `required property CourseResultContext result` |

Settings receives a section name. An empty name opens the theme's usual first
page, and `"keys"` requests input configuration. Each settings theme maps these
names to its own layout and uses its first page for unknown names. For example,
selection can call `globalRoot.openSettings("keys")` without knowing a tab number.
Calling `openSettings()` while settings is already current keeps its current page.

Every gameplay theme supports courses. Decide and gameplay receive the same
context, which survives their handoff. A course stage uses the same live data
shape as a single chart; the theme does not receive different runner types.

`GameplayContext` provides:

- `chartData`: metadata for the current chart, including the last stage after
  course completion.
- `players`: one `Player` for solo or double play, two for local battle. Each
  supplies `profile`, live `score`, `notes`, `state`, timing and position.
- `bga`, `status`, `keymode`, and writable `inputMapping` for presentation.
- `course` (undefined for a single chart), `charts`, zero-based `stageIndex`, and
  `stageCount`. Single charts have one stage. `coursePlayers` supplies the
  course-wide combo state in participant order and is empty for single charts.
- `isArena`: whether this play belongs to Arena. Runner ownership remains internal.
- `arenaActive`: whether the session still presents this gameplay round. Use it
  to choose Arena targets or standings, independently of any overlay.

`Player.columns` supplies playable column indices in the default visual order.
The left scratch comes first and the right scratch comes last. Five-key modes
omit unused columns. Use `player.isScratchColumn(index)` for lane styling.
Themes can supply their own order, and keep using `player.state.columnStates[index]`
for the corresponding note model. Changing visual order does not change input mapping.

Bind through `gameplay.players` and `gameplay.chartData`; they change together
when a course installs its next stage. Do not retain an old stage's `Player`.
Pass the context to `StandardDecideFlow.gameplay` or `StandardGameplayFlow.gameplay`.

`ResultContext` always describes one chart, including a completed course stage.
It provides `chartData`, `players` (typed `ResultPlayer` entries, each pairing
`profile` with a `BmsScore`), and fixed `course`, `stageIndex`, and `stageCount`
metadata. Its `gameplay` identifies the originating play for standard retry;
that context remains live, so use the result's own metadata to describe the
completed stage.

`CourseResultContext` describes the aggregate course summary. It provides
`course`, `charts`, and `players` (typed `CourseResultPlayer` entries, each
pairing `profile` with a `BmsScoreCourse`). It has no single-chart retry.
`charts` keeps stage order and may contain null for an uninstalled chart in a
saved result. Read the matching identifier from `course.chartIds[index]`.
`Rg.tables.search(identifier)` can supply a title even when the chart is absent.
Chart identifiers are MD5 hashes or `sha256/<hash>`. Installed metadata in a
result is retained by that context, independently of the runner or song list.
Normal results and course summaries are independently selected screen roles.
There is no fallback from `courseResult` to a theme's normal `result`.
The profile selects each role separately, with Default providing unconfigured
roles. Default shares its private presentation through separate typed entry points.

`ResultContext.arenaRoundId` identifies an Arena result and is empty for local
play or saved-score views. Standard gameplay submits the Arena score before
opening the result and ends its presentation when the screen is destroyed.
`result.arenaActive` reports whether the session still presents the same round.
Use it for standings and chat input rules, even if your theme draws its own panel.
If screen creation fails, it keeps the submitted round for a later retry.
`ContentFrame` passes the result data without calling methods on the theme.

Both result participant lists have one or two entries without null padding.
Pass either result context to `StandardResultInput.result`. Completed metadata
and score totals are fixed; existing live score-submission notifications remain
available. The host retains the required play objects while results are shown.
Themes must not destroy injected contexts or their data.

This replaces the old root `chart`/`arenaManagedRunner` and parallel
`scores`/`profiles` properties for public QML themes. Migrate the root declaration
and behavior binding together. LR2 retains internal entry files for select,
decide, gameplay, result and course result. They receive these same contexts.
The shared LR2 renderer loads its own CSV path, metadata and saved settings,
and derives its legacy score fields from the result context. ContentFrame
does not pass LR2 configuration or legacy result properties.

## Input

Support mouse, keyboard and bound controller input where the screen needs it.
For example, a song list can accept the mouse wheel, arrow keys and scratch.
Default settings currently use mouse and keyboard controls.

For mouse input, use [MouseArea](https://doc.qt.io/qt-6/qml-qtquick-mousearea.html) or
[TapHandler](https://doc.qt.io/qt-6/qml-qtquick-taphandler.html).

For keyboard input, use [Keys](https://doc.qt.io/qt-6/qml-qtquick-keys.html)

For bound key input (controller or keyboard), use
[Input](https://bobini1.github.io/RhythmGame/classqml__components_1_1InputAttached.html).

`ContentFrame` automatically disables all input for screens that are not active.
Make sure to disable any background sounds and [Shortcuts](https://doc.qt.io/qt-6/qml-qtquick-shortcut.html) when
the screen is inactive (Shortcuts are not disabled automatically).
Song preview should not play during gameplay. You can use the
[enabled](https://doc.qt.io/qt-6/qml-qtquick-item.html#enabled-prop) property of `Item`
to detect when a screen is not active.
Disabling an Item also disables its child Items, including Standard controllers,
so they need no explicit binding to the parent's `enabled` property. Standard
components handle their own shortcuts, timers and audio internally. Objects
such as `Shortcut` and `Connections` do not inherit the disabled state.

For note and lane feedback during gameplay, use
[columnState.pressed](https://bobini1.github.io/RhythmGame/classgameplay__logic_1_1ColumnState.html#a116fbd7d8aec0c9ebad00828b7564ab6).
The gameplay state includes autoplay and replay presses. Use `Input` for
physical input actions such as opening a popup, rather than for note feedback.

### Reusable selection components

The public theme API serves ordinary QML themes. Types under
`RhythmGameQml/Lr2` are internal. LR2 can reuse Standard components or use its
own implementation, without changing what the public API promises.

Themes can import `RhythmGameQml` and opt into the standard selection behavior
without using the Default theme's presentation:

Use one high-level entry point or its chosen lower-level pieces, not both.
`StandardSelectController` already creates its input, shortcuts, state, session,
activation, presentation adapter, background music and preview player. The
[selector tutorial](docs/pages/theme-tutorial/05-select.md) includes a finite-list
example with keyboard activation and focus wiring.

For a complete standard selector, instantiate `StandardSelectController`. It
extends `StandardSelectState` with presentation adaptation, folder enter/leave
feedback, input, navigation, selection shortcuts and music. Its inherited `entries`
contain one logical copy of each filtered item; `presentationEntries` repeats
them to `minimumEntryCount` for circular selectors. Handle
`focusRequested(index)` and `moveRequested(steps, repeated, analog)` to update
the theme's presentation. Set `autoInitialize` to false and call `initialize()`
when a theme needs to control startup timing.

The entries are folder-path strings, `ChartData` objects, or the QML value types
`entry`, `table`, `level` and `course`. `entry` describes a table chart that was
not found in the local song database. It has table metadata such as `title` and
`url`, but no playable chart path. `table`, `level` and `course` use `name` for
their labels. Check the type before choosing fields or a delegate.

Use `setSortMode()`, `setReplayType()` and `setKeymodeFilter()` for player choices.
These methods save in the active profile and update the corresponding read-only
property. Switching profiles updates all three properties. Unknown values and
calls from an inactive controller return `false` without changing the choice.
Only the cycling actions wrap around the available options.
Properties such as `difficultyFilter` and `unscoredItemsLast` configure the
view itself and do not save profile settings.

For a separate view that should not change profile settings, use
`ChartFolderModel` to filter and sort its entries. Its writable properties
configure that model alone. The standard selector always uses the saved choices.

An empty folder or a filter that removes every entry leaves `entries` empty.
`focusedItem` is then `null`, and `focusedIndex` is `-1`. The same applies before
initialization. A finite list can show an empty-list message when
`entries.length === 0`. Only `presentationEntries` adds `null` placeholders
for a wheel, with at least one row and enough rows to meet `minimumEntryCount`.
Check `item == null` in a wheel delegate before reading fields. The tutorial's
[entry type table](docs/pages/theme-tutorial/05-select.md#theme_tutorial_select_entry_types)
lists the fields and activation behavior for each type.

The selector's optional `sortOptions` and `replayOptions` models contain objects
with a stable `value` and a translated `text`. Bind a `ComboBox` with
`textRole: "text"`, `valueRole: "value"` and `currentValue` set to the selected
value. Pass `currentValue` to `setSortMode()` or `setReplayType()` when activated.
`sortModeText` and `replayTypeText` provide the current labels for cycling buttons.
Use these labels directly, without another `qsTr()` call.

The labels are optional. Themes can use their own text, models or artwork with
the same selection actions. To change one caption, use an ordinary binding:

```qml
text: selection.replayType === StandardSelectState.Newest
    ? qsTr("Latest run") : selection.replayTypeText
```

For a list or combo box, format the text in a delegate or derive a model with
`map()`. Compare each option's `value`, rather than its position in the list.
The `StandardSelectOptions` singleton provides the same defaults for screens
such as Settings without creating a selector.
The sort label "Total" refers to the chart's TOTAL gauge value, not its note count.

For a different composition, use only the lower-level pieces required.

Browsing and activation:

- `StandardSelectSession` owns the raw folder contents, history and preview
  paths. Its `resolve...` methods only
  acquire data; `commitFolderContents()` is the explicit state change. A custom
  `tableCoursesProvider` can return the courses for a table.
- `StandardSelectActions` opens charts, courses and saved results. Its `exit()`
  leaves selection or its Arena room. Custom selectors can use it without the
  standard browsing state.
- `StandardSelectState` combines the session and activation behavior with
  sorting, filtering and focus. Its `entries` contain one logical copy of each
  filtered item. Preview-file discovery and folder clear-statistic loading can
  be disabled independently. Sorting and filtering follow profile settings
  unless the theme overrides them. The `results` map always reads the profile's
  memory cache for chart and course entries in the current folder. Recordings
  and gauge histories load only when a graph, saved-result screen or replay
  needs them. Child folders and table levels use separate summary requests for
  their clear statistics.
- Other screens can bind to `profile.scoreDb.chartResults[md5] || []` or
  `profile.scoreDb.courseResults[identifier] || []`. Chart keys are upper-case.
  Both maps update automatically after saves and imports. Cached result objects
  remain valid while the profile exists, and imports update them in place.
- `StandardSelectModelAdapter` can repeat a logical model to a requested
  minimum size for circular visual selectors. It supplies `null` placeholders
  when the source is empty. Themes with a finite list do not need to instantiate it.

Interaction policies:

- `StandardSelectNavigation` converts directional keys, scratch and mouse wheel
  input into `moveRequested(steps, repeated, analog)` signals. The theme remains
  responsible for positioning and animating its list or wheel.
- `StandardSelectInput` adds the standard activation, replay, autoplay,
  sorting and back-button mappings by extending `StandardSelectNavigation`. A
  custom state can supply semantic `activateAction`, `goBackAction` and
  `atTopLevel` values instead of pretending to be `StandardSelectState`.
  Autoplay and replay have state-backed implementations. Replay key 7 calls
  `openReplay()`, and key 6 calls `cycleReplayType()`. The profile's `replayType`
  chooses newest, best score rate, best clear or best combo. Bind a replay button
  to `replayAvailable` and call `openReplay()` from its click handler.
  `tryReplayAction(type)` can handle the request before the standard action.
  Scores without recordings are excluded from playback. Courses need a recording
  for every stage. Replays use the main profile alone and are unavailable in Arena.
  `resultForReplayType(type, false)` also includes scores without recordings when
  a theme needs to display a saved result.
- `StandardSelectShortcuts` provides F2 reload, F3 open-folder, F11
  internet-ranking and F12 settings shortcuts. F2 and F3 use a supplied
  `selectState` by default. F2, F3 and F12 defaults can be replaced or
  individually disabled. The controller supplies F11's standard ranking action.
  When used separately, F11 calls `openInternetRankingAction` or emits
  `openInternetRankingRequested` when no action is supplied. If the
  standard state declines F2 or F3, the corresponding request signal is also
  emitted for the theme.

`StandardSelectReload` supplies table/root-folder reload policy for custom
selectors. `reload(focusedItem, history, folderPath)` returns true when it requests
a reload/scan; otherwise the caller can refresh its current folder model.
`StandardSelectState` (and therefore `StandardSelectController`) includes it.
The old `globalRoot.reloadTableForItem()` and `scanRootSongFolderForPath()` helpers
have moved out of the host; custom callers should use this component.

Shared asynchronous lifetime:

- `PendingReplyGroup` owns any set of asynchronous replies with one cancellation
  lifetime. It can be reused by custom selection enrichment or other theme
  state that starts cancellable operations.

For a finite list, use `StandardSelectListView` with `controller: selection`.
Supply a delegate and call `activate(index)` from its click handler. It handles
arrow, wheel and controller navigation, current-row synchronization and preview
movement.
The theme still sets keyboard focus on the list and its enclosing focus scopes.

The controller also works without that list component. Keyboard `Keys` handlers stay on
the focused visual item and can forward up/down/release events to the
controller's `handleUpPressed`, `handleDownPressed` and `handleReleased`
methods. Forward wheel events from the view to `handleWheel(event)`. It converts
each notch into a row movement and accumulates smaller deltas. Wheel movement
sets `analog` to true in `moveRequested`, like analog scratch input, so the view
can use the same scrolling animation. `StandardSelectListView` forwards wheel
events automatically.

Set `enterFeedbackSource` and `leaveFeedbackSource` to choose folder sounds,
or use an empty URL to omit one. For custom feedback, set
`feedbackEnabled: false` and handle `enteredFolder` and `leftFolder` yourself.

Application-owned F1 and F4 behavior is internal to `ContentFrame`. F12 belongs
to `StandardSelectShortcuts`, so it is only available on selection screens that
opt into the standard selection shortcuts.

### Selection actions and previews

Buttons and input mappings use the same state methods and overrides.
`openAutoplay()` uses `autoplayAvailable` for a focused chart or complete course.
`cycleSortMode(delta)` follows `sortModes` and saves the profile's sort choice;
`setSortMode(mode)` chooses one directly. Keys 2 and 4 cycle sorting at the root,
and go back inside folders. `tryAutoplayAction` and `tryCycleSortModeAction`
can handle requests before the standard behavior.

`reloadCurrentFolderOrTable()`, `openSelectedFolder()` and `openSettings()`
apply their replacement actions regardless of whether a button or F-key
requested them. Search history stores each query and restores it when returning
from a song's chart directory.

`StandardSelectController.preview` is a `StandardSelectPreview` for the focused
chart. It delays playback, uses chart audio when no preview file exists, and
clears both players when disabled. `StandardSelectListView` binds `preview.moving`
automatically. For another view, bind it to that view's movement state.
Set `preview.chartAudioEnabled: false` to use preview files only, or
`preview: null` to omit previews.

The controller's `backgroundMusic` is an `AudioPlayer` for the profile's
selection BGM. It stops while a preview plays or the controller is disabled.
`backgroundMusicDelayMillis` defaults to 500, so music does not restart between
quick chart changes. Customize the player's source, volume or fadeInMillis,
or set `backgroundMusic: null` to omit it.

Put the controller outside a visual container that you disable for an options
panel. Bind `inputEnabled` and `shortcutsEnabled` to that container's enabled
state when music should continue while the panel is open.

For custom browsing, use `StandardSelectPreview` separately. Bind `selectState`
to a `StandardSelectState`, or supply `chartData` and `previewSource` directly.
Use `playing` to coordinate your own background music. Don't add a separate
preview player to a controller that already has one.

### Online rankings

`StandardSelectController.ranking` is a `StandardRanking` for the focused chart.
It uses the active profile's ranking provider and server settings. Bind your
display to `ranking.entries`, `ranking.position`, `ranking.playerCount`,
`ranking.clearCounts` and `ranking.loading`. The controller's
`openInternetRanking()` opens its website and is also the default F11 action.

Use `StandardRanking` directly on a result or another screen. Set `chartData`
and, for a result, set `profile` and `score` for that player. It refreshes on
submission changes and exposes `previousPosition` for comparing the earlier
best score against the current opponents. Course summaries have no chart
ranking. Individual course stages use the normal result interface.

The shared implementation loads one provider at a time, cancels obsolete
requests, includes the local best score in LR2IR, and resolves Tachi chart links.
It reads local results directly from the profile's cache, which updates after
saves and imports. Themes do not need to pass in a separate result list.
Set `entriesEnabled: false` when only website opening is needed. Opening a link
then requires no leaderboard download. Disabling the component stops its work.

For customization, override `ranking.provider`, `ranking.profile` or
`ranking.url`, or supply `openInternetRankingAction` on the controller.
`StandardRanking.openUrlAction(url)` replaces browser opening. A controller's
`ranking` can also be replaced with another `StandardRanking`, or set to null.
A replacement should bind `chartData` to the controller's focused item and
`enabled` to its enabled state. `OnlineRankingModel` remains available for
custom queries and sorting.

### Arena panels belong to the screen

Add `StandardArenaSelectOverlay` to a selector that supports Arena. Bind its
`navigationFocusTarget` to the song list so closing chat restores keyboard
focus. The panel shows the room, ready controls and chat while the player is
seated in a room. The overlay handles the double-Start ready gesture and supplies
its own `readyShortcutDescription`. Set its `readyShortcutEnabled` to false to
disable the gesture and hide its hint. Replacing `panelComponent` preserves the
gesture. Hidden or disabled overlays stop handling it and clear any pending
first press. The selection controller still handles chart proposals.

`StandardSelectController.exit()` leaves local selection or an Arena room.
Escape calls it by default. Use `goBack()` only for folder history. Set
`exitEnabled` to false to replace Escape, or set `exitAction` to replace the exit
action.

Add `StandardArenaGameplayOverlay` beside `StandardGameplayFlow`, and bind
both components to the same `gameplay` context. Add `StandardArenaResultOverlay`
to a normal result screen and bind its `result`. Local play and saved-score
views do not show these panels. Course summaries do not need an Arena overlay.

The overlays share `StandardArenaOverlay` for placement, saved preferences
and chat input. Set `defaultPixelRectHint` to choose an initial position,
`customizeMode` to show editing controls, and `z` to set the stacking order.
The visual parent is the viewport. Place the overlay directly inside the
screen root, or set `parent` to the item whose bounds it should use.
The read-only `viewport` property reports that parent. `themeVars`
defaults to the active profile's settings for the overlay's screen role.

Gameplay and result panels use F8 to toggle chat. The selector's chat shortcut
is disabled by default because a selector may already use F8. Change
`chatShortcut` or disable `chatShortcutEnabled` to supply different input.
Hidden and disabled screens do not restore chat state or handle the shortcut.
A custom Ready button binds to the selection overlay's `ready` and
`canToggleReady`, displays `readyDisabledReason` when useful, and calls
`toggleReady()`. The standard panel and double-Start gesture use the same
action. Disabling `readyShortcutEnabled` only disables the gesture.
On a result screen, `StandardResultInput` disables confirmation while the
matching Arena chat is open. Themes can still override `confirmEnabled`.

Replace `panelComponent` to keep the standard placement and input with your
own visuals. Call the overlay's `setChatOpen(open)` for chat tabs; it updates
the session and saves the preference together. Default's result screen uses its own `ArenaResultPanel` this way.
The [complete tutorial theme](docs/pages/theme-tutorial/10-shipping.md) shows
the standard overlays in installable select, gameplay and result screens.

`ContentFrame` does not create these panels. Screens no longer declare
`arenaNativeSelectPresentation`, `arenaNativeGameplayPresentation` or
`arenaNativeResultPresentation`, and they need no `rememberArenaChatSelection`
method. LR2 creates its overlays and customization editor inside its internal
screen wrapper. The host only handles screen operations and play object lifetime.

### Arena browser and screen lifetime

A multiplayer theme uses `StandardMultiplayerFlow` and reads its `session`.
Create and join buttons call `session.createRoom()` and `session.joinRoom()`.
The connection retry button calls `session.retry()`, and Back calls the flow's
`close()`. The theme no longer declares request signals for ContentFrame.

The browser, room selector, gameplay and result are ordinary screens on one
stack. `currentScreen` always identifies the actual theme. The browser remains
under selection, and removing the browser disconnects from Arena. The standard
selector returns to it when the room is left or lost. If Settings or a result
covers selection, that return waits until selection becomes active again.

The multiplayer flow opens loaded gameplay directly, without decide. The theme
finishes its intro and requests a start before Arena reports readiness. If a
round starts while Settings is open, gameplay returns to Settings afterward.
Neither Arena navigation nor its panels require a nested stack or shell.

### Reusable behavior for other screens

Choose the component for the screen you are writing. The
[tutorial](docs/pages/theme-tutorial/index.md) contains installable examples
for the main menu, selection, decide, gameplay and both result roles.

| Component              | What it handles                                             | How to use it                                                                    |
|------------------------|-------------------------------------------------------------|----------------------------------------------------------------------------------|
| `StandardMainController` | Song selection, Arena, Settings, quit and main-menu music | Call its methods from buttons. Bound START opens selection by default.           |
| `StandardMultiplayerFlow` | Arena connection, room selection and prepared gameplay | Place it inside the multiplayer screen and use its `session`. |
| `StandardDecideFlow`   | Timeout, accept, cancel and duplicate requests              | Bind `gameplay`. Set `pointerEnabled: false` if your screen has its own buttons. |
| `StandardGameplayFlow` | Startup, exit, retry, stage results and course continuation | Place it inside the gameplay screen and bind `gameplay`.                        |
| `StandardResultInput`  | Delayed result confirmation and supported retry             | Bind `result` and call `confirm()` from your Continue button.                    |

`StandardMainController` includes `StandardMainActions` and a `backgroundMusic`
player. It plays the profile's main-menu BGM while enabled. Customize the
player's source, volume or fadeInMillis, or set it to null to omit music.
Use `StandardMainActions` separately when you only need actions and Start input.

Decide starts by replacing itself with gameplay. Its replacement `startAction`
or `cancelAction` handles the whole transition, and the standard action does
not run afterward. The game controls play object lifetime in either case.

The gameplay flow inherits `enabled` through its parents. The host disables
inactive screens, including their nested controls. `stageActivated` is emitted
before each stage starts, so it is a suitable place to reset visuals or request score targets.
`closing` lets the theme clean up before leaving.

Bind `startReady` to delay the ready sequence for an intro. Start an outro
in `finishRequested` and bind `finishReady` to its completion. The flow still
saves scores and opens results. `readySoundSource` selects ready audio, and
`startDelayMillis` controls the following delay. The default delay is zero
when ready audio is available and 1000 milliseconds otherwise.

`dismissOverlayAction()` can close a popup and return true to consume Escape.
If a result fails to load, `presentationFailed` is emitted and the saved
scores are kept. `retryTransition()` tries to open the result again. Arena runs
the same intro, ready audio and delay, then holds the start request until every
participant is ready and the shared start time arrives. Asset loading and theme
preparation share the server's 60-second loading timeout. Exiting before play
cancels the round without saving a score and keeps everyone in the room.
Arena retains runner ownership. `StandardMultiplayerFlow`
releases the prepared runner when its gameplay screen leaves the stack.
Don't add `StandardGameplayInput` beside the complete flow or duplicate
runner start, finish or proceed calls.

The result component accepts both result context types. A
`tryHandleButtonAction(key)` can handle a display action, such as cycling
gauges, before retry or dismissal. Keys 5 and 7 request fresh randomization
and the same pattern respectively, using the originating play in the context.

### Gameplay score targets

`StandardGameplayTarget` takes `gameplay` and `playerIndex`. It follows the
player's profile and supplies `points`, `finalPoints`, and `available` for a
target display. Local targets use a fraction, best score, latest score or next
rank. Override `mode` or `fraction` for a different choice. Recorded targets
advance with consumed notes; scores without recordings use their score rate.
Battle and Arena use their live opponents.

The helper loads scores for each course stage and cancels obsolete queries.
`bestPoints`, `bestFinalPoints` and `bestMaxPoints` supply the personal-best
comparison independently of the chosen target. No display or flow component
is installed automatically.

Retry buttons on gameplay and normal results use `retryAvailable` and
`retry(samePattern)`. Pass false for fresh randomization or true for the same
pattern. The result component includes its input delay in availability.
`retryEnabled` disables both button and controller retries. The result's
`tryRetryAction(samePattern)` can handle a request before the standard retry.

### Gameplay note speed and visible models

`StandardGameplayViewport` takes the current `player`, `chartData` and
`noteHeight`. It reads the player's `noteScreenTimeMillis` and `hiSpeedFix`
settings and supplies `pixelsPerBeat`. Use that value with `player.position`
and each note's position to place notes and size long notes. Main, Start, Min,
Max and Avg select chart metadata; Off uses 120 BPM. Live BPM and scroll
changes remain part of the player's position.

The helper owns `player.state.setVisiblePositionSpans()` for that player.
It updates both note and bar-line models when settings, dimensions or the
current stage change. Fields sharing a player share its filtered models, so
their visible position spans must agree. For separate clipping of bar lines,
set `barLineHeight`; it defaults to `noteHeight`.

`visibleHeight` defaults to `noteHeight`. With lane cover or lift, set it to
the remaining distance from the lane cover to the judgement line. `noteHeight`
is the distance from the top note clipping edge to that line. Hidden cover
doesn't change the speed calculation. All dimensions use the same local
coordinates as the renderer. Default and the tutorial use this helper.
The native filter keeps long-note beginnings when their bodies cross either
edge of the visible range, including reverse scroll. Note changes notify both
the named roles and `display`, so either form can be used in a delegate.

### Gameplay speed and cover controls

`StandardGameplayFlow` and `StandardGameplayInput` include
`StandardGameplaySpeedControls`. Hold START and turn the scratch to change
`noteScreenTimeMillis`; hold SELECT and turn it to adjust a cover. Lane cover
takes priority when on, followed by lift or hidden. When both lift and hidden
are on, a short START+SELECT press switches between them. With all covers off,
SELECT changes the saved lane-cover ratio. Double-press START within 500 ms to
toggle lane cover while preserving note speed within the duration limits.

Each battle side uses its own player's profile. In double play, either side
changes the same profile, and either side's START/SELECT modifies either scratch.
The controls follow replacement players between stages and stop while the
screen is inactive or retry is being chosen. Holding both START and SELECT
also suspends adjustments. Set `speedControlsEnabled: false` on the flow or
input component to supply your own controls. Don't add a second instance of
the speed controls beside either component.

### Gameplay BGA

`StandardGameplayBga` is an optional visual component. Bind `gameplay` and
give it a size. It follows the current stage's four BGA layers, preserves their
relative sizes and treats black as transparent in the two overlay layers.
Bad and Poor judgements show the miss layer for 400 ms. Its visibility follows
`bgaOn` on the player's profile, or the main profile in battle. Override
`visible` to choose differently. A custom renderer can connect the layers to
its own video sinks instead.

### Components for custom gameplay flows

`StandardGameplayInput` maps Escape, the START+SELECT retry gesture, and speed
and cover controls.
Bind `gameplay` and handle `exitRequested` in your flow. The component does
not save scores or open results. A custom flow owns startup, completion,
course continuation and the return to selection.
Use the context's lifecycle methods:

- `gameplay.start()` requests a ready stage's start after the screen and its intro
  are ready. Local play starts immediately. Arena holds the request until the
  shared start time, so `status` remains Ready while waiting for other players.
- `gameplay.finishStage()` saves once and returns its `ResultContext`. Repeated
  calls return that same result. It returns null while loading and does not
  advance a course.
- After closing that normal result, `gameplay.nextStage()` advances and loads
  the next course stage. It returns false after the final stage or if a
  participant's profile was deleted. A failed advance leaves the current stage
  intact. The standard flow returns to selection if the course cannot continue.
- After the final normal result, `gameplay.finishCourse()` saves once and returns
  the `CourseResultContext` for `globalRoot.openCourseResult()`.

All methods reject nested calls while a lifecycle operation is in progress.
Check the returned object or boolean before navigating. Observe `status` to
wait for loading; call `start()` only once the stage is Ready. Closing the
course summary returns to selection. `StandardGameplayFlow` handles this
sequence and result-creation retries already.

For Arena, call `start()` once after preparation and retain the same screen while
waiting or reconnecting. Closing a prepared screen cancels the round; a start
request cannot be withdrawn to replay the intro. Before play, a custom exit
action must call `Rg.arenaSession.abandonCurrentRound()` rather than save a result.
Once play begins, `finishStage()` submits the local result as part of completion.
A custom flow must call `Rg.arenaSession.endResultPresentation(result.arenaRoundId)`
when its result screen closes, if the ID is nonempty. Preserve that exact ID;
a later round may already exist. `StandardGameplayFlow` handles this cleanup.
Native runners and screen-stack bookkeeping are internal implementation details.


Hold START+SELECT for `retryHoldDurationMillis`, which defaults to 1000.
During `retryChoosing`, releasing START retries the same pattern, releasing
SELECT uses fresh randomization, and releasing both cancels. Gameplay stays
visible while exit waits for the choice. Delay completion and gate your
other controls with `!retryChoosing`, but keep this component enabled so it
receives releases. `StandardGameplayFlow` already handles these gates.

`retryEnabled` and `exitEnabled` disable those actions separately.
`retryAction(samePattern)` replaces restarting, while `retry(samePattern)` and
`cancelRetry()` can be called from custom controls. `exit()` emits
`exitRequested` when exit is allowed. The handler decides whether to close
an overlay, abandon untouched play, or finish the stage with a result.

`StandardChartRetry` supplies `available` and `retry(samePattern)` without
input or completion handling. Supply `gameplay` during play, or supply
`result` on a normal result. It replaces gameplay
and any result above it. Courses, autoplay, replay, battle and Arena are
excluded. An unfinished attempt is discarded, while an already saved result
is kept. A supported request is handled even if loading fails, leaving the
current screen available.

`StandardGameplayAttemptState` reports whether either player has made a
scoring hit. Bind `gameplay` when using it in a custom flow.
`StandardInputKeys.isPlayKey(key)` identifies the lane keys used by the
standard decide and result controls.

### Action callbacks

Action properties are optional. Without an override, the component uses its
standard operation. A property documented as a replacement handles the whole
operation, and its return value is ignored. A `try...Action` runs first and
returns true when it handled the request. Returning false or undefined lets
the standard action continue.

Signals can let the theme respond without replacing an operation. For example,
`StandardGameplayFlow.closing` lets gameplay clean up its visuals before
leaving. Check the property reference before replacing an action. Use named
navigation operations such as `globalRoot.returnToPreviousScreen()` instead
of accessing `sceneStack` directly.

## Scaling

Your theme is expected to work on all screen sizes and aspect ratios.
There are two ways you can achieve that.

- Use [layouts](https://doc.qt.io/qt-6/qtquicklayouts-overview.html) to dynamically set the sizes of elements based on
  the size of their parents.
- Use fixed, hard-coded component sizes and [rescale](https://doc.qt.io/qt-6/qml-qtquick-item.html#scale-prop) the top
  item to fit the size of the window.

In the default theme, you can see the first approach in the `settings` screen. All other screens use scaling.

Please try resizing the window of the game when testing your theme edits.

## The Rg singleton

The [Rg](https://bobini1.github.io/RhythmGame/classRg.html) singleton is globally accessible upon importing
`RhythmGameQml`. It contains various API objects with methods and properties useful for themes.
Start with `Rg.profileList.mainProfile` for profile data. The
[profile lesson](docs/pages/theme-tutorial/03-profile.md) shows a greeting and a
saved setting, and the generated `Rg` reference lists the other services.

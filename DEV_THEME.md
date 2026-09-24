# Theme Development

Start with the [skin tutorial](docs/pages/theme-tutorial/index.md) for a sequence
of installable examples, from a main menu to gameplay and results. Use this
page to look up the theme contract and the
[architecture notes](docs/pages/skin-architecture.md) to understand the design.

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

The game loads and validates themes at startup. A critical configuration
error can prevent startup, so check `log.txt` in the data folder for the file
and error. Restart after changing a manifest or QML file.

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
- `replaceGameplay(gameplay, screen)` replaces that screen and anything above it
  with local gameplay. Omitting `screen` replaces the current screen. It returns
  the new screen, or `null` if creation fails or the target is unavailable.
  The replacement is created before removing the old screens, so a broken
  gameplay skin leaves the current screen intact.

Decide uses `replaceGameplay(gameplay)` to hand off to play. Arena uses
`openGameplay(gameplay)` to retain the screen underneath. The operation chooses
the stack change; it does not depend on the play mode.

Runner lifetime follows the local screen: decide owns it until gameplay takes
over; gameplay retains it while results are shown, and destroys it when removed
or replaced. `ContentFrame` handles this even when a skin omits the standard
input components. Skins must not destroy these runners themselves. When
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
function openChart(path, profile1, autoplay1, replay1, score1, profile2, autoplay2, replay2, score2)
function openCourse(course, profile1, autoplay1, replay1, score1, profile2, autoplay2, replay2, score2)
function openResult(scores, profiles, chartData, gameplay = null, arenaRoundId = "")
function openCourseResult(scores, profiles, chartDatas, course)
function openSettings(section = "")
```

These operations return the opened screen, or a falsy value if creation fails.
Standard gameplay retains saved scores when result creation fails. Pass the
originating `gameplay` explicitly when opening a result after play. Omit it
when viewing a saved score from selection. The host does not infer the origin
from whichever screen happens to be current.

Since gameplay screens and result screens need to be pushed with some initial property state,
those helper methods set them based on the parameters passed to them.

See [ChartLoader docs](https://bobini1.github.io/RhythmGame/classqml__components_1_1ChartLoader.html)
for an explanation of the parameters of `openChart` and `openCourse`.

Call them on `globalRoot`, for example `globalRoot.openChart(...)`.

Keep the following in mind when writing screens with initial state:

Each data-bearing screen declares one typed input:

| Screen role       | Required declaration                           |
|-------------------|------------------------------------------------|
| `settings`        | `required property string initialSection`     |
| `decide`          | `required property GameplayContext gameplay`   |
| All gameplay keys | `required property GameplayContext gameplay`   |
| `result`          | `required property ResultContext result`       |
| `courseResult`    | `required property CourseResultContext result` |

Settings receives a section name. An empty name opens the skin's usual first
page, and `"keys"` requests input configuration. Each settings skin maps these
names to its own layout and uses its first page for unknown names. For example,
selection can call `globalRoot.openSettings("keys")` without knowing a tab number.
Calling `openSettings()` while settings is already current keeps its current page.

Every gameplay skin supports courses. Decide and gameplay receive the same
context, which survives their handoff. A course stage uses the same live data
shape as a single chart; the skin does not receive different runner types.

`GameplayContext` provides:

- `chartData`: metadata for the current chart, including the last stage after
  course completion.
- `players`: one `Player` for solo or double play, two for local battle. Each
  supplies `profile`, live `score`, `notes`, `state`, timing and position.
- `bga`, `status`, `keymode`, and writable `inputMapping` for presentation.
- `isCourse`, `course` (undefined for a single chart), `charts`, zero-based `stageIndex`, and
  `stageCount`. Single charts have one stage. `coursePlayers` supplies the
  course-wide combo state in participant order and is empty for single charts.
- `isArena`: whether this play belongs to Arena. Runner ownership remains internal.
- `arenaActive`: whether the session still presents this gameplay round. Use it
  to choose Arena targets or standings, independently of any overlay.

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
Normal results and course summaries are independently selected screen roles.
There is no fallback from `courseResult` to a skin's normal `result`.
The profile selects each role separately, with Default providing unconfigured
roles. Default shares its private presentation through separate typed entry points.

`ResultContext.arenaRoundId` identifies an Arena result and is empty for local
play or saved-score views. Standard gameplay submits the Arena score before
opening the result and ends its presentation when the screen is destroyed.
`result.arenaActive` reports whether the session still presents the same round.
Use it for standings and chat input rules, even if your skin draws its own panel.
If screen creation fails, it keeps the submitted round for a later retry.
`ContentFrame` passes the result data without calling methods on the skin.

Both result participant lists have one or two entries without null padding.
Pass either result context to `StandardResultInput.result`. Completed metadata
and score totals are fixed; existing live score-submission notifications remain
available. The host retains the required play objects while results are shown.
Skins must not destroy injected contexts or their data.

This replaces the old root `chart`/`arenaManagedRunner` and parallel
`scores`/`profiles` properties for public QML skins. Migrate the root declaration
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
This property propagates to all child components.

For note and lane feedback during gameplay, use
[columnState.pressed](https://bobini1.github.io/RhythmGame/classgameplay__logic_1_1ColumnState.html#a116fbd7d8aec0c9ebad00828b7564ab6).
The gameplay state includes autoplay and replay presses. Use `Input` for
physical input actions such as opening a popup, rather than for note feedback.

### Reusable selection components

The public skin API serves ordinary QML skins. Types under
`RhythmGameQml/Lr2` are internal. LR2 can reuse Standard components or use its
own implementation, without changing what the public API promises.

Themes can import `RhythmGameQml` and opt into the standard selection behavior
without using the Default theme's presentation:

Use one high-level entry point or its chosen lower-level pieces, not both.
`StandardSelectController` already creates its input, shortcuts, state, session,
activation and presentation adapter. The
[selector tutorial](docs/pages/theme-tutorial/05-select.md) includes a finite-list
example with keyboard activation and focus wiring.

For a complete standard selector, instantiate `StandardSelectController`. It
extends `StandardSelectState` with presentation adaptation, folder enter/leave
feedback, input, navigation and selection shortcuts. Its inherited `entries`
contain one logical copy of each filtered item; `presentationEntries` repeats
them to `minimumEntryCount` for circular selectors. Handle
`focusRequested(index)` and `moveRequested(steps, repeated, analog)` to update
the skin's presentation. Set `autoInitialize` to false and call `initialize()`
when a skin needs to control startup timing.

For a different composition, use only the lower-level pieces required.

Browsing and activation:

- `StandardSelectSession` owns the raw folder contents, history, asynchronous
  score-query lifetime, scores and preview paths. Its `resolve...` methods only
  acquire data; `commitFolderContents()` is the explicit state change. A custom
  `tableCoursesProvider` can return the courses for a table.
- `StandardSelectActions` opens charts, courses and saved results. Its `exit()`
  leaves selection or its Arena room, and `handleStartPress(key)` handles the
  double-Start ready gesture. Custom selectors can use it without the standard
  browsing state.
- `StandardSelectState` combines the session and activation behavior with
  sorting, filtering and focus. Its `entries` contain one logical copy of each
  filtered item. Score, preview-file and folder-stat enrichment can be disabled
  independently, and all sorting/filtering policies have profile-backed
  defaults that a skin can override.
- `StandardSelectModelAdapter` can repeat a logical model to a requested
  minimum size for circular visual selectors. Skins with a finite list do not
  need to instantiate it.

Interaction policies:

- `StandardSelectNavigation` converts directional key and scratch input into
  semantic `moveRequested(steps, repeated, analog)` signals. The skin remains
  responsible for positioning and animating its list or wheel.
- `StandardSelectInput` adds the standard activation, replay, autoplay,
  sorting and back-button mappings by extending `StandardSelectNavigation`. A
  custom state can supply semantic `activateAction`, `goBackAction` and
  `atTopLevel` values instead of pretending to be `StandardSelectState`.
  Autoplay has a state-backed implementation. Replay needs
  `tryReplayAction`, since choosing a replay score remains skin-owned.
- `StandardSelectShortcuts` provides F2 reload, F3 open-folder, F11
  internet-ranking and F12 settings shortcuts. F2 and F3 use a supplied
  `selectState` by default. F2, F3 and F12 defaults can be replaced or
  individually disabled. F11 emits `openInternetRankingRequested`; if the
  standard state declines F2 or F3, the corresponding request signal is also
  emitted for the skin.

`StandardSelectReload` supplies table/root-folder reload policy for custom
selectors. `reload(focusedItem, history, folderPath)` returns true when it requests
a reload/scan; otherwise the caller can refresh its current folder model.
`StandardSelectState` (and therefore `StandardSelectController`) includes it.
The old `globalRoot.reloadTableForItem()` and `scanRootSongFolderForPath()` helpers
have moved out of the host; custom callers should use this component.

Shared asynchronous lifetime:

- `PendingReplyGroup` owns any set of asynchronous replies with one cancellation
  lifetime. It can be reused by custom selection enrichment or other skin
  state that starts cancellable operations.

The controller has no required list interface. Keyboard `Keys` handlers stay on
the focused visual item and can forward up/down/release events to the
controller's `handleUpPressed`, `handleDownPressed` and `handleReleased`
methods. Feedback remains configurable through the controller's
`feedbackEnabled`, action and sound-source properties.

Application-owned F1 and F4 behavior is internal to `ContentFrame`. F12 belongs
to `StandardSelectShortcuts`, so it is only available on selection screens that
opt into the standard selection shortcuts.

### Arena panels belong to the screen

Add `StandardArenaSelectOverlay` to a selector that supports Arena. Bind its
`navigationFocusTarget` to the song list so closing chat restores keyboard
focus. The panel shows the room, ready controls and chat while the player is
seated in a room. The selection controller still handles chart proposals and
the ready gesture. Bind `readyShortcutDescription` to the controller's property
of the same name. The overlay leaves the description empty by default, since
it does not install a ready gesture itself.

`StandardSelectController.exit()` leaves local selection or an Arena room.
Escape calls it by default. Use `goBack()` only for folder history. Set
`exitEnabled` or `readyEnabled` to false to replace the corresponding input,
or set `exitAction` to replace the exit action.

Add `StandardArenaGameplayOverlay` beside `StandardGameplayFlow`, and bind
both components to the same `gameplay` context. Add `StandardArenaResultOverlay`
to a normal result screen and bind its `result`. Local play and saved-score
views do not show these panels. Course summaries do not need an Arena overlay.

The overlays share `StandardArenaOverlay` for placement, saved preferences
and chat input. Set `defaultPixelRectHint` to choose an initial position,
`customizeMode` to show editing controls, and `z` to set the stacking order.
Place the overlay directly inside the screen root. If you use another parent,
set `viewport` to the item whose coordinates the overlay uses. `themeVars`
defaults to the active profile's settings for the overlay's screen role.

Gameplay and result panels use F8 to toggle chat. The selector's chat shortcut
is disabled by default because a selector may already use F8. Change
`chatShortcut` or disable `chatShortcutEnabled` to supply different input.
Hidden and disabled screens do not restore chat state or handle the shortcut.
On a result screen, disable confirmation while its Arena chat is open.

Replace `panelComponent` to keep the standard placement and input with your
own visuals. Default's result screen uses its own `ArenaResultPanel` this way.
The [complete tutorial skin](docs/pages/theme-tutorial/10-shipping.md) shows
the standard overlays in installable select, gameplay and result screens.

`ContentFrame` does not create these panels. Screens no longer declare
`arenaNativeSelectPresentation`, `arenaNativeGameplayPresentation` or
`arenaNativeResultPresentation`, and they need no `rememberArenaChatSelection`
method. LR2 creates its overlays and customization editor inside its internal
screen wrapper. The host only handles screen operations and play object lifetime.

### Arena browser and screen lifetime

A multiplayer skin uses `StandardMultiplayerFlow` and reads its `session`.
Create and join buttons call `session.createRoom()` and `session.joinRoom()`.
The connection retry button calls `session.retry()`, and Back calls the flow's
`close()`. The skin no longer declares request signals for ContentFrame.

The browser, room selector, gameplay and result are ordinary screens on one
stack. `currentScreen` always identifies the actual skin. The browser remains
under selection, and removing the browser disconnects from Arena. The standard
selector returns to it when the room is left or lost. If Settings or a result
covers selection, that return waits until selection becomes active again.

The multiplayer flow opens scheduled gameplay directly, without decide. If a
round starts while Settings is open, gameplay returns to Settings afterward.
Neither Arena navigation nor its panels require a nested stack or shell.

### Reusable behavior for other screens

Choose the component for the screen you are writing. The
[tutorial](docs/pages/theme-tutorial/index.md) contains an installable example
for each role.

| Component              | What it handles                                             | How to use it                                                                    |
|------------------------|-------------------------------------------------------------|----------------------------------------------------------------------------------|
| `StandardMainActions`  | Song selection, Arena, Settings and quit                    | Call its methods from buttons. Bound START opens selection by default.           |
| `StandardMultiplayerFlow` | Arena connection, room selection and scheduled gameplay | Place it inside the multiplayer screen and use its `session`. |
| `StandardDecideFlow`   | Timeout, accept, cancel and duplicate requests              | Bind `gameplay`. Set `pointerEnabled: false` if your screen has its own buttons. |
| `StandardGameplayFlow` | Startup, exit, retry, stage results and course continuation | Bind `gameplay` and put the component directly inside the screen root.           |
| `StandardResultInput`  | Delayed result confirmation and supported retry             | Bind `result` and call `confirm()` from your Continue button.                    |

Decide starts by replacing itself with gameplay. Its replacement `startAction`
or `cancelAction` handles the whole transition, and the standard action does
not run afterward. The game controls play object lifetime in either case.

The gameplay flow uses its parent as the owning screen. Set `screen` if you
nest it inside another item. `stageActivated` is emitted before each stage
starts, so it is a suitable place to reset visuals or request score targets.
`closing` lets the skin clean up before leaving.

Bind `startReady` to delay the ready sequence for an intro. Start an outro
in `finishRequested` and bind `finishReady` to its completion. The flow still
saves scores and opens results. `readySoundSource` selects ready audio, and
`startDelayMillis` controls the following delay. The default delay is zero
when ready audio is available and 1000 milliseconds otherwise.

`dismissOverlayAction()` can close a popup and return true to consume Escape.
If a result fails to load, `presentationFailed` is emitted and the saved
scores are kept. `retryTransition()` tries to open the result again. Arena
controls its own startup, and the host releases its play object when the
screen is removed. Don't add `StandardGameplayInput` beside the complete flow
or duplicate runner start, finish or proceed calls.

The result component accepts both result context types. A
`tryHandleButtonAction(key)` can handle a display action, such as cycling
gauges, before retry or dismissal. Keys 5 and 7 request fresh randomization
and the same pattern respectively, using the originating play in the context.

### Components for custom gameplay flows

`StandardGameplayInput` maps Escape and the START+SELECT retry gesture.
Bind `gameplay` and handle `exitRequested` in your flow. The component does
not save scores or open results. A custom flow owns startup, completion,
course continuation and the return to selection.

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

Signals can let the skin respond without replacing an operation. For example,
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

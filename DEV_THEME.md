# Theme Development

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

A theme does not need to implement all screens, it can just implement the ones you want.
You can think of it as a package that can contain replacements for any number of the game's screens.

### Persisent settings

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

There are 7 different types of properties you can use in a settings file:

- boolean
- range
- string
- file
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

Themes are loaded and validated when the game starts.
If there are any critical errors in configuration files,
the game will fail to start. Don't share broken themes!
Look at log.txt in the data folder to find out what went wrong.

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
function openResult(scores, profiles, chartData)
function openCourseResult(scores, profiles, chartDatas, course)
```

Since gameplay screens and result screens need to be pushed with some initial property state,
those helper methods set them based on the parameters passed to them.

See [ChartLoader docs](https://bobini1.github.io/RhythmGame/classqml__components_1_1ChartLoader.html)
for an explanation of the parameters of `openChart` and `openCourse`.

Call them on `globalRoot`, for example `globalRoot.openChart(...)`.

Keep the following in mind when writing screens with initial state:

Screens `k7`, `k14` and `k7battle` are expected to have single `var` property called `chart`. The assigned `chart` will
be either a [ChartRunner](https://bobini1.github.io/RhythmGame/classgameplay__logic_1_1ChartRunner.html) or
[CourseRunner](https://bobini1.github.io/RhythmGame/classgameplay__logic_1_1CourseRunner.html)
object, depending on whether the player is playing a single chart or a course.

Screen `result` is expected to have the following properties:

```qml
property list<BmsScore> scores
property list<Profile> profiles
property ChartData chartData
```

The lists will have 1 or 2 elements, depending on whether the game was played solo or in battle mode.

Screen `courseResult` is expected to have the following properties:

```qml
property list<BmsScoreCourse> scores
property list<Profile> profiles
property list<ChartData> chartDatas
property course course
```

You can always use `var` instead of the exact type if you want to be flexible.

## Input

It would be best if your theme supported mouse, keyboard and controller navigation.
For example, for scrolling a list of songs, it would be best to allow using the mouse wheel, arrow keys and scratch.
In the default theme, the `settings` screen was designed with mouse and keyboard in mind,
without support for controller navigation.

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

Gameplay screens should not use `Input` directly. Instead, use
[columnState.pressed](https://bobini1.github.io/RhythmGame/classgameplay__logic_1_1ColumnState.html#a116fbd7d8aec0c9ebad00828b7564ab6).
This will play nicely with autoplay and replays.
`Input` is reserved for actual input, not injected key presses.

### Reusable selection components

Themes can import `RhythmGameQml` and opt into the standard selection behavior
without using the Default theme's presentation:

- `StandardSelectSession` owns the raw folder contents, history, asynchronous
  score-query lifetime, scores and preview paths. Its `resolve...` methods only
  acquire data; `commitFolderContents()` is the explicit state change. A custom
  `tableCoursesProvider` can return the courses for a table.
- `StandardSelectActivation` implements the standard chart, course and Arena
  activation rules.
- `StandardSelectState` combines the session and activation behavior with
  sorting, filtering and focus. Its `entries` contain one logical copy of each
  filtered item. Score, preview-file and folder-stat enrichment can be disabled
  independently, and all sorting/filtering policies have profile-backed
  defaults that a skin can override.
- `StandardSelectModelAdapter` can repeat a logical model to a requested
  minimum size for circular visual selectors. Skins with a finite list do not
  need to instantiate it.
- `StandardSelectNavigation` converts directional key and scratch input into
  semantic `moveRequested(steps, repeated, analog)` signals. The skin remains
  responsible for positioning and animating its list or wheel.
- `StandardSelectInput` adds the standard activation, replay, autoplay,
  sorting and back-button mappings around `StandardSelectNavigation`. A custom
  state can supply semantic `activateAction`, `goBackAction` and `atTopLevel`
  values instead of pretending to be `StandardSelectState`.
- `StandardSelectShortcuts` provides F2 reload, F3 open-folder, F11
  internet-ranking and F12 settings shortcuts. F2 and F3 use a supplied
  `selectState` by default; every action can be replaced or individually
  disabled. If the standard state declines F2 or F3, the corresponding request
  signal is emitted for the skin.
- `StandardSelectFeedback` provides the default folder enter/leave sounds. It
  can be omitted, disabled or given replacement actions and sound sources.
- `StandardSelectController` is the convenience composition. It exposes
  supported policies, observations, actions and semantic methods without
  exposing mutable implementation objects. Set `tryOpenPlayableAction` to
  intercept standard chart/course/Arena activation, configure navigation and
  enrichment through the controller properties, or instantiate the lower-level
  components directly for a different composition. Set `autoInitialize` to
  false and call `initialize()` when a skin needs to control startup timing.
- `PendingReplyGroup` owns any set of asynchronous replies with one cancellation
  lifetime. It can be reused by custom selection enrichment or other skin
  state that starts cancellable operations.

The controller has no required list interface. Handle `focusRequested(index)`
and `moveRequested(steps, repeated, analog)` to update the skin's selector.
Keyboard `Keys` handlers stay on the focused visual item and can forward
up/down/release events to the controller's `handleUpPressed`,
`handleDownPressed` and `handleReleased` methods.

Application-owned F1 and F4 behavior is internal to `ContentFrame`. F12 belongs
to `StandardSelectShortcuts`, so it is only available on selection screens that
opt into the standard selection shortcuts.

### Reusable behavior for other screens

Other screens expose smaller components around their own natural seams rather
than copying the selection component structure:

- `StandardMainActions` provides the standard song-select, Arena, settings and
  quit destinations, including START opening song selection.
- `StandardDecideFlow` owns decide timeout, accept/cancel input, transition
  guarding and chart-runner destruction. It fills its parent by default; the
  skin owns the decide visuals. A replacement start/cancel action owns its full
  transition; `returnAfterGameplayAction` only customizes the follow-up for the
  built-in gameplay transition.
- `StandardResultInput` owns delayed result dismissal and retry input. A skin
  can supply `tryHandleButtonAction` for presentation-specific actions such as
  cycling a displayed gauge before the standard retry/dismissal handling runs.
  Skin pointer handlers can call `confirm()` to share the standard confirmation
  gate.
- `StandardGameplayAttemptState` tracks whether a chart has received a scoring
  hit. It is available separately for custom gameplay transitions.
- `StandardGameplayExit` combines that attempt state with Escape handling and
  normal result dispatch. Untouched charts return immediately; attempted plays
  finish and open their result. Presentation cleanup and result opening remain
  replaceable, as does the play-stop feedback. Completion detected while the
  screen is inactive is retained and presented when the screen becomes active.

`StandardInputKeys.isPlayKey(key)` is available when a custom component needs
the same lane-key classification used by the standard decide and result input.

All action properties are optional. Without an override these components call
the semantic operations exposed by the application content frame. An `Action`
property is a complete replacement and does not need to return a value. A
`try...Action` is a pre-handler: return `true` when it consumed the request, or
return `false`/`undefined` to continue with the standard behavior. Skins should
use `globalRoot.returnToPreviousScreen()` instead of accessing `sceneStack`
directly.

## Scaling

Your theme is expected to work on all screen sizes and aspect ratios.
There are two ways you can achieve that.

- Use [layouts](https://doc.qt.io/qt-6/qtquicklayouts-overview.html) to dynamically set the sizes of elements based on
  the size of their parents.
- Use fixed, hard-coded component sizes and [rescale](https://doc.qt.io/qt-6/qml-qtquick-item.html#scale-prop) the top
  item to fit the size of the window.

In the default theme, you can see the first approach in the `settings` screen. All other screens use scaling.
In general, scaling is easier to use.

Please try resizing the window of the game when testing your theme edits.

## The Rg singleton

The [Rg](https://bobini1.github.io/RhythmGame/classRg.html) singleton is globally accessible upon importing
`RhythmGameQml`. It contains various API objects with methods and properties useful for themes.
You will probably use it a lot! Start reading the docs there.

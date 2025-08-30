# Theme Development

If you want to edit a theme, you only need a text editor.
Edit the scripts and relaunch the game to see your changes.

All errors are printed to log.txt in the assets folder. On platforms other than Windows,
the log is printed to the console as well.
You can also press F10 in-game to open the log overlay.

# Theme structure

A theme is a folder in `assets/themes/` that contains a `theme.json` file.
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
        "pl": "pl.qm"
    }
}
```

## Theme loading

Themes are loaded and validated when the game starts.
If there are any critical errors in configuration files,
the game will fail to start. Don't share broken themes!
Look at log.txt in the assets folder to find out what went wrong.

The part of the game that manages the theme most directly is
[RhythmGameQml/ContentFrame.qml](https://github.com/Bobini1/RhythmGame/blob/master/RhythmGameQml/ContentFrame.qml).
That qml file is compiled into the executable and is not part of any theme.
It contains the window of the entire game and the `sceneStack`.
`sceneStack` is a globally accessible [StackView](https://doc.qt.io/qt-6/qml-qtquick-controls-stackview.html)
that contains screens, like `main`, `select`, `k7`, one on top of the other.
You can go to another screen by pushing it to the stack.
You can exit a screen by popping it from the stack.

---
**NOTE**

When going from `select` to `k7` and then `result`, the former screens are not destroyed.
When going back from `result`, it is the responsibility of the `k7` screen to remove itself from the stack
to go back to select. When `k7` pushes `result`, it should remember to remove itself when it becomes active again.

In most scenarios, you will want to push at most one screen on top of the stack at once.
Then that screen can push another screen on top of itself, if needed.

Do not remove screens from under yourself. It's messy and unnecessary.

---

## Initial state of screens

`sceneStack` contains a few helper methods:

```qml
function openChart(path, profile1, autoplay1, score1, profile2, autoplay2, score2)
function openCourse(course, profile1, autoplay1, score1, profile2, autoplay2, score2)
function openResult(scores, profiles, chartData)
function openCourseResult(scores, profiles, chartDatas, course)
```

Since gameplay screens and result screens need to be pushed with some initial property state,
those helper methods set them based on the parameters passed to them.

See [ChartLoader docs](https://bobini1.github.io/RhythmGame/classqml__components_1_1ChartLoader.html)
for an explanation of the parameters of `openChart` and `openCourse`.

To use those methods, simply call `sceneStack.openChart(...)` from anywhere in your theme.

Keep the folowing in mind when writing screens with initial state:

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

`sceneStack` automatically disables all input for screens that are not at the top of the stack.
Make sure to disable any background sounds when pushing a new screen on top of the stack.
Song preview should not play during gameplay. You can use the
[enabled](https://doc.qt.io/qt-6/qml-qtquick-item.html#enabled-prop) property of `Item`
to detect when a screen is not active.
This property propagates to all child components.

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

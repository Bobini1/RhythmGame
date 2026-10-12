# 1. Make a main menu {#theme_tutorial_first_screen}

[Tutorial](index.md) | [Next: QML basics](02-qml.md)

Install `docs/examples/theme-tutorial/01-main/` to replace the main menu with a
simple example main screen with four navigation buttons and a background image.

[Download 01-main](examples/theme-tutorial/01-main.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/01-main).

## Install the folder

First, close the game and copy the whole `01-main` folder into a `themes`
directory. A regular installation reads themes from its installation folder (the Default theme)
and your user data directory.
The usual locations are listed below. Linux uses `$XDG_DATA_HOME` in place of
`~/.local/share` when that variable is set. Windows uses the roaming AppData
directory if it has been moved from its usual location.

| Platform | User theme directory                               |
|----------|----------------------------------------------------|
| Windows  | `%APPDATA%/RhythmGame/themes/`                     |
| Linux    | `~/.local/share/RhythmGame/themes/`                |
| macOS    | `~/Library/Application Support/RhythmGame/themes/` |

Portable mode is enabled by `portable.ini` in the installation prefix, one
directory above `bin`. In portable mode the game uses the installation's data
directory and doesn't scan a separate user theme directory.

Keep the folder structure intact after extracting the download.

```text
themes/
    01-main/
        theme.json
        main.qml
        images/
            notes.svg
```

## Register the screen

The game reads `theme.json` to find the screens a theme provides. The `scripts`
object maps each screen role to a QML file, using paths relative to the theme's
folder. The first example provides only `main`.

\include 01-main/theme.json

The `$schema` URL helps a text editor check the file. It's not required.
The theme's name comes from its folder name, so the example will
appear as `01-main` in the game.

You can see all valid screen names [here](@ref theme_tutorial_stuck).

## Draw the menu

Open `01-main/main.qml`. `QtQuick` supplies the basic visual objects,
`QtQuick.Controls.FluentWinUI3` supplies the buttons and label, and `QtQuick.Layouts` arranges
them. `RhythmGameQml` supplies the game's types and also gives access to the Rg singleton,
which we won't use in this part of the tutorial.

The tutorial examples import the
[FluentWinUI3 style](https://doc.qt.io/qt-6/qtquickcontrols-fluentwinui3.html)
explicitly, so their controls use that style regardless of the game's default.
FluentWinUI3 works on both Windows and Linux. Fonts and colors can still vary
with the system settings. Read the documentation of specific styles for more info.

You can completely avoid using the Qt Quick Controls module and create your own custom controls manually if you want.
This can be a good idea if you want super custom controls,
but I recommend checking out the [Imagine style](https://doc.qt.io/qt-6/qtquickcontrols-imagine.html) first, it's very
customizable.

[StandardMainController](qml/qml-rhythmgameqml-standardmaincontroller.html) handles menu actions, the START button and
background music.
It draws nothing, so the theme needs to create its own buttons. Their `onClicked` handlers call
`openSelect()`, `openArena()`, `openSettings()` and `quit()` for the four menu actions.
The component is optional. You can call `globalRoot.openSelect()`,
`globalRoot.openArenaBrowser()`, `globalRoot.openSettings()` and
`globalRoot.quitApplication()` directly.
If you omit the component, make sure you give controller users some way to enter song select without using their mouse.
[StandardMainController](qml/qml-rhythmgameqml-standardmaincontroller.html) binds the START button to `openSelect()`.

In short, the rule is that each screen has a "contract".
You can make a main menu theme that does give the user any way to go to song select.
But this breaks the contract. It doesn't matter what UX you choose exactly but each screen has some roles.

\snippet 01-main/main.qml main-actions

\snippet 01-main/main.qml buttons

The image path is relative to `main.qml`. `sourceSize` sets the size used to
render the SVG, while the layout controls the space it occupies on screen.

\snippet 01-main/main.qml image

The complete screen is short enough to read before making a change.

\include{lineno} 01-main/main.qml

\image html theme-tutorial-main.png "The example menu with FluentWinUI3 controls in dark mode" width=640px

## Choose the theme and try a change

Second, launch the game and open Settings. Under Themes, select Main Menu and
choose `01-main` as its theme. Leave the other screens on Default or your
existing themes, then go back to see the newly selected main menu.

Click Song select or press your bound START button. The chosen selection
screen should open. Multiplayer opens the configured Multiplayer screen, where
you can join an Arena room. Settings opens settings, and Quit closes the game.

Third, edit the title or background color in `main.qml`, save the file, and
restart the game.

@note
Hot reload is something I'm planning once Qt 6.12 becomes more widely available.

Press F10 to inspect the log if the screen fails to load. `log.txt` is in the
game's data directory, next to `themes`.

Resize the window to a square, a 4:3 rectangle and a wide rectangle. The menu
should stay centered with all four buttons visible. The next lesson explains the
sizing and adds controls you can reuse.

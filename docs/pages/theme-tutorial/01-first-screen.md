# 1. Make a main menu {#theme_tutorial_first_screen}

[Tutorial](index.md) | [Next: QML basics](02-qml.md)

Install `docs/examples/theme-tutorial/01-main/` to replace the main menu with an
image and three buttons. Song select opens the selection screen already chosen
in your profile. Settings opens the game's settings, and Quit closes the game.

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
object maps each screen role to a QML file, using paths relative to the theme
folder. The first example provides only `main`.

\include 01-main/theme.json

The `$schema` URL helps a text editor check the file. It doesn't download or
enable a theme. The theme's name comes from its folder name, so the example will
appear as `01-main` in the game.

## Draw the menu

Open `01-main/main.qml`. `QtQuick` supplies the basic visual objects,
`QtQuick.Controls` supplies the buttons and label, and `QtQuick.Layouts` arranges
them. `RhythmGameQml` supplies the game's types and also gives access to the Rg singleton,
which we won't use in this part of the tutorial.

`StandardMainActions` handles menu actions and bound START input. It draws
nothing, so the theme needs to create its own buttons. Their `onClicked` handlers call
`openSelect()`, `openSettings()` and `quit()` for the three menu actions.
The component is optional. You can call `globalRoot.openSelect()`,
`globalRoot.openSettings()` and `globalRoot.quitApplication()` directly.
If you omit the component, handle bound START input yourself so controller
users can enter selection without a mouse.

\snippet 01-main/main.qml main-actions

\snippet 01-main/main.qml buttons

The image path is relative to `main.qml`. `sourceSize` sets the size used to
render the SVG, while the layout controls the space it occupies on screen.

\snippet 01-main/main.qml image

The complete screen is short enough to read before making a change.

\include{lineno} 01-main/main.qml

\image html theme-tutorial-main.png "The example menu rendered with Qt's Basic controls style" width=640px

## Choose the theme and try a change

Second, launch the game and open Settings. Under Themes, select Main Menu and
choose `01-main` as its theme. Leave the other screens on Default or your
existing themes, then restart the game to load the new menu.

Click Song select or press your bound START button. The chosen selection
screen should open. From the main menu, click Settings to change the selected
theme or other options. Click Quit when you want to close the game.

Third, edit the title or background color in `main.qml`, save the file, and
restart the game. Restart after changing a manifest too. F2 in song selection
reloads songs or tables, so it won't reload your QML edits.

Press F10 to inspect the log if the screen fails to load. `log.txt` is in the
game's data directory, next to `themes`, in either portable or user data mode.
Start with the first error naming your file and line, since later errors may
follow from it.

Resize the window to a square, a 4:3 rectangle and a wide rectangle. The menu
should stay centered with all three buttons visible. The next lesson explains the
sizing and adds controls you can reuse.

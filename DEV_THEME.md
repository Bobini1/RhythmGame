# Theme development

RhythmGame themes are written in QML. A theme can provide one screen or a whole
set, and players can choose a different theme for each screen. Design your
screens to work alongside screens from other themes.

Start with the [theme tutorial](docs/pages/theme-tutorial/index.md). Each lesson
has a complete example folder you can install and edit. You only need a text
editor and the game to try it.

## Theme folders

A theme folder contains its QML files, images and other assets, plus a
`theme.json` file. The manifest tells the game which screens the theme provides
and where to find them. For example:

```json
{
  "scripts": {
    "main": "main.qml"
  }
}
```

The player can select this theme for the main menu and keep their other screens
on Default. The [first lesson](docs/pages/theme-tutorial/01-first-screen.md)
explains installation and screen registration. The tutorial also lists the
[supported screen names](@ref theme_tutorial_stuck).

## Shared behavior

Import `RhythmGameQml` to use the game's data and Standard components. Your
QML defines the appearance, while the Standard components handle common actions
such as browsing songs, starting play and opening results.

Choose the component for the screen you're building:

| Screen | Component | Common behavior |
|--------|-----------|-----------------|
| Main menu | [StandardMainController](https://bobini1.github.io/RhythmGame/qml/qml-rhythmgameqml-standardmaincontroller.html) | Navigation and menu music |
| Song selection | [StandardSelectController](https://bobini1.github.io/RhythmGame/qml/qml-rhythmgameqml-standardselectcontroller.html) | Browsing, input, previews and saved scores |
| Decide | [StandardDecideFlow](https://bobini1.github.io/RhythmGame/qml/qml-rhythmgameqml-standarddecideflow.html) | Confirmation before play |
| Gameplay | [StandardGameplayFlow](https://bobini1.github.io/RhythmGame/qml/qml-rhythmgameqml-standardgameplayflow.html) | Startup, exit, retry and results |
| Result and course result | [StandardResultInput](https://bobini1.github.io/RhythmGame/qml/qml-rhythmgameqml-standardresultinput.html) | Confirmation and supported retries |
| Multiplayer browser | [StandardMultiplayerFlow](https://bobini1.github.io/RhythmGame/qml/qml-rhythmgameqml-standardmultiplayerflow.html) | Connecting to Arena and entering rooms and gameplay |

The components are optional and expose ways to customize their behavior. Start
with the complete controller or flow for your screen, then change the parts you
need. Its reference page describes the available properties and actions.

Arena panels also belong to themes. The [selection](docs/pages/theme-tutorial/05-select.md),
[gameplay](docs/pages/theme-tutorial/07-gameplay.md) and
[result](docs/pages/theme-tutorial/08-results.md) lessons show how to include
the standard room and standings panels.

The game manages screen creation and navigation. For custom navigation, use
`globalRoot` operations such as `openSelect()` and `returnToPreviousScreen()`.
Let the game manage the underlying screen stack.

## Game data

The [Rg singleton](https://bobini1.github.io/RhythmGame/class_rg.html) provides
access to shared data such as profiles and the song library. For example,
`Rg.profileList.mainProfile` gives you the active profile. The
[profile lesson](docs/pages/theme-tutorial/03-profile.md) shows how to use it.

The game supplies play and result data when it opens those screens. Decide and
gameplay receive a [GameplayContext](https://bobini1.github.io/RhythmGame/qml/qml-rhythmgameqml-gameplaycontext.html),
and result screens receive a [ResultContext](https://bobini1.github.io/RhythmGame/qml/qml-rhythmgameqml-resultcontext.html)
or [CourseResultContext](https://bobini1.github.io/RhythmGame/qml/qml-rhythmgameqml-courseresultcontext.html).
Bind your display to the supplied data so it follows changes during play.

Every gameplay theme must support both single charts and courses. Normal result
and course result are separate screens that players choose independently.
The [gameplay](docs/pages/theme-tutorial/07-gameplay.md) and
[result](docs/pages/theme-tutorial/08-results.md) lessons cover both cases.

## Settings and translations

Themes can declare saved preferences in the manifest's `settings` field. The
game stores them separately for each profile, screen and theme, and generates
controls for them in Settings. The [profile lesson](docs/pages/theme-tutorial/03-profile.md)
walks through adding and using a color setting.

Use `QmlUtils.themeName` when accessing your theme's settings so the theme still
works if its folder is renamed. Keep each screen's preferences independent of
other screens, since the player may select those screens from another theme.

The manifest's `translations` field declares the languages a theme supports.
Follow the [translation lesson](docs/pages/theme-tutorial/04-translations.md)
for an example, or [DEV_LANG.md](DEV_LANG.md) for the translation workflow.

## Trying a theme

Restart the game after editing theme files. Press F10 to inspect errors in the
log overlay, or read `log.txt` in the game's data folder. If a broken theme
prevents you from reaching Settings, the tutorial explains
[how to restore the screen choices](@ref theme_tutorial_stuck).

Test your screens at different window sizes and with mouse, keyboard and
controller input. The [QML lesson](docs/pages/theme-tutorial/02-qml.md) introduces
layouts and scaling. Screens can remain loaded while another screen is shown,
so pause custom sounds and shortcuts while your screen is disabled. The
Standard components handle their own inactive state.

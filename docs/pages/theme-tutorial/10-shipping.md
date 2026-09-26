# 10. Combine and share your screens {#theme_tutorial_shipping}

[Previous](09-assets.md) | [Tutorial](index.md)

Install `docs/examples/theme-tutorial/10-complete/` to try the tutorial screens
together. The folder contains its own copies of every helper it uses, so it
doesn't depend on the earlier lesson folders. It keeps Default for Settings
and Multiplayer because it doesn't provide those roles.

[Download 10-complete](examples/theme-tutorial/10-complete.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/10-complete).

## Choose the screens in the manifest

The manifest combines the main menu, enhanced selector, decide, gameplay and
both result roles. The same gameplay file serves several keymodes, while the
two result roles keep separate files.

\include 10-complete/theme.json

Choose `10-complete` for the roles you want to replace, then restart the game.
The menu has the profile greeting and saved color setting from lesson 3.
The menu and its setting include the Polish translations from lesson 4.
Later screens still use their English source text, so you can add their
translations by following the same steps.

You can remove a role from the manifest when sharing a theme for just that
screen. Include every helper and asset that the remaining files use. A theme
must work with other authors' screens, so avoid imports from Default or
assumptions about which selector opened your gameplay screen.

## Add Arena panels to the screens

The complete example has an Arena button that opens the configured Multiplayer
screen. The example keeps Default for that role, then uses its own selector
after you join a room. A custom Multiplayer screen can use
`StandardMultiplayerFlow` to connect, enter a room and open scheduled rounds.
The browser and selector are ordinary screens on the same stack. Leaving the
room returns to the browser; closing the browser disconnects from Arena.

\snippet 10-complete/main.qml arena-menu

Each screen adds its own panel. `StandardArenaSelectOverlay` shows the room,
ready controls and chat while you are in a room. Its `navigationFocusTarget`
returns keyboard focus to the song list after chat closes. The earlier
selector examples include the same component. The controller handles leaving
the room and the double-Start ready gesture; the panel only presents the room
and its controls.

\snippet 10-complete/Select.qml arena-select

Gameplay binds its overlay to the same context as `StandardGameplayFlow`.
The overlay shows standings for that Arena play and stays hidden during
local play or a course. F8 switches between standings and chat.

\snippet 10-complete/Gameplay.qml arena-gameplay

The result overlay uses `result.arenaActive` to show the matching Arena result.
The context checks the round ID and session state, so the theme doesn't need
to repeat those checks. The overlay stays hidden for local results and saved
scores. Course summaries have their own result type and do not include it.

\snippet 10-complete/Result.qml arena-result

The result input disables confirmation while Arena chat is open, so pressing
Enter to send a message does not also close the result screen. It checks
`result.arenaActive`, which tells it whether the context belongs to the current
Arena result presentation. Removing the overlay does not change that state.

\snippet 10-complete/Result.qml result-input

Set an overlay's `defaultPixelRectHint` to choose its initial position and
size. Players can then move and resize it, and their profile keeps those
settings for the chosen theme. Set `customizeMode` to show its editing controls.
`z` places the panel above the example's content.

The overlays also allow a different chat shortcut or a replacement
`panelComponent`. See the
[overlay reference](qml/qml-rhythmgameqml-standardarenaoverlay.html) for the
properties that a custom panel uses. Default's result screen supplies its own
panel this way, while keeping the standard placement and chat behavior.

## Check a complete play

First, follow the path from the main menu through selection, decide, gameplay
and result. Cancel from decide once, then finish a chart. Confirm that returning
to selection keeps the folder and focus, and that preview audio stops during play.

Second, check a course through a normal result after every stage, including
the last stage, followed by the course summary. Try each
keymode you advertise, including battle if it is in the manifest. Check normal
play, autoplay and replay where the chosen selector offers them. Standard
components supply shared behavior, but your layout still needs to display the
corresponding data.

Third, change the window size and the profile. Inspect long titles, an empty
song list, missing images and a chart with no saved scores. Use F10 to look for
errors naming your QML files. A successful load alone doesn't show whether
notes, sound or transitions behave correctly during play.

## Package the folder

Rename a copy of the folder to the name you want players to select. Run it
under that name to check that settings use `QmlUtils.themeName` and all paths
stay relative. The renamed copy can have fresh setting values because its
folder name is also its settings identity.

Zip the theme folder with `theme.json` directly inside it. Include source
assets, any required fonts and compiled `.qm` translations. Keep profiles,
databases, songs, logs and editor output out of the package. State which game
build you tested and which screen roles the theme provides in a README.

The note renderer is a starting point for a theme, with no BGA, gauge display
or speed controls. Add the presentation features you need before distributing
it as your regular gameplay theme, then repeat the play checks.

## Keep shared behavior in the standard components

Change visual properties first, then a specific behavior option if needed.
A full action replacement makes your theme responsible for that operation.
For example, `startAction` on decide replaces the transition itself, while
changing the title or timeout keeps the standard transition.

\image html theme-responsibilities.svg "Responsibilities of a theme and the game"

The [architecture notes](../theme-architecture.md) explain the alternatives.
The [theme reference](../../../DEV_THEME.md) and [QML reference](qml/qml-api.html)
describe the interfaces to check when updating your theme for another game build.

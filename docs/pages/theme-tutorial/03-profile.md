# 3. Read a profile and save a theme setting {#theme_tutorial_rg}

[Previous](02-qml.md) | [Tutorial](index.md) | [Next: Translations](04-translations.md)

Install `docs/examples/theme-tutorial/03-profile/` and choose it for Main Menu.
The menu greets the active profile by name. Its greeting color is a setting
you can change in the game's existing settings screen.

[Download 03-profile](examples/theme-tutorial/03-profile.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/03-profile).

## Read the active profile

[Rg](@ref Rg) gives QML access to the game's shared objects. It is a singleton, which
means you use the existing object by name after importing `RhythmGameQml`.
You don't create an [Rg](@ref Rg) instance.

Follow `Rg.profileList.mainProfile` to the active profile, then read
`vars.generalVars` for preferences shared across themes. The example binds a
local `profile` property so the rest of the file can use a shorter expression.

\snippet 03-profile/main.qml profile

The greeting joins `"Hello, "` and `generalVars.name` with `+`. If the active
profile changes, the binding follows the new profile and the label updates.

\snippet 03-profile/main.qml profile-label

Use the main profile for shared screens such as the menu. During gameplay,
use the profile attached to each player for that player's display. A battle
can involve two profiles, and a result's score owner need not be the current
main profile. Later lessons show the supplied player and result objects.

## Declare a setting for the menu

The manifest points the `main` screen to a settings file as well as a QML file.
The game reads the setting definitions and the settings screen adds the appropriate controls for changing the values,
unless you use the special `hidden` property type.

\include 03-profile/theme.json

\include 03-profile/main-settings.json

`id` is the name used in QML. `name` and `description` are the text shown in
the settings form, and `default` supplies the first value for a profile.
The `color` type tells the form which control to use.

Saved values live under `profile.vars.themeVars[screenName][themeName]`.
The example uses `"main"` for its screen and `QmlUtils.themeName` for its folder name.
That's because a user can rename the theme's folder - so we do this to avoid hard-coding it.
[QmlUtils](@ref qml_components::QmlUtilsAttached) is
an [attached property](https://doc.qt.io/qt-6/qtqml-typesystem-attachedtypes.html).

Theme settings are per-profile and separate for each theme.
Different screens of the same theme should not share settings.
If you make a theme with gameplay screens,
you will need separate json files with settings for 7k and 14k, even though they might end up very similar.

@note
In theory, you can access the settings of any screen at any point.
You can do profile.vars.themeVars["select"]["Default"] to get the settings of the Default theme's select, for example.
The settings screen does it for all installed themes for obvious reasons.
But if you abuse this to create cross-screen or cross-theme shared settings, it might get confusing for users.

@note
As a special case, if your theme does not define 5k/10k screens,
the 7k/10k screens will be selectable for those key modes, and they *will* share settings across key modes,
automatically.

## Try the saved value

Open Settings > Themes > Main Menu and change Accent color. Return to the
menu, then restart the game to check that the color persists. Change the
profile's name in its player settings and check the greeting too.

The next lesson adds translations for the greeting, buttons and color setting.

## Find other game data

Start with the object related to the task, then follow its generated reference.
The \ref Rg page lists the full set of properties and their types.

| Task                                     | Objects to read                                                       |
|------------------------------------------|-----------------------------------------------------------------------|
| Show profile preferences or saved scores | `profileList`, then the profile's `vars` or `scoreDb`                 |
| Inspect themes or supported languages    | `themes`, `languages`                                                 |
| Work with the song library               | `rootSongFoldersConfig`, `songFolderFactory`, `tables`                |
| Load images, previews or local files     | `songAssets`, `songDirectoryFilePathFetcher`, `fileQuery`             |
| Build device or audio settings           | `inputTranslator`, `gamepadManager`, `audioEngine`, `programSettings` |
| Build online ranking or Arena UI         | `onlineScores`, `arenaSession`, `arenaRoomCount`               |

Most screens won't need to use those objects directly. For example,
[StandardSelectController](qml/qml-rhythmgameqml-standardselectcontroller.html) already handles ordinary browsing and
chart activation through `chartLoader`. `globalRoot` is a separate object supplied
by the game for navigation, such as `globalRoot.returnToPreviousScreen()`.
Use its named operations rather than accessing the internal screen stack.

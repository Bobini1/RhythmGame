# 3. Read a profile and save a theme setting {#theme_tutorial_rg}

[Previous](02-qml.md) | [Tutorial](index.md) | [Next: Translations](04-translations.md)

Install `docs/examples/theme-tutorial/03-profile/` and choose it for Main Menu.
The menu greets the active profile by name. Its greeting color is a setting
you can change in the game's existing settings screen.

[Download 03-profile](examples/theme-tutorial/03-profile.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/03-profile).

## Read the active profile

`Rg` gives QML access to the game's shared objects. It is a singleton, which
means you use the existing object by name after importing `RhythmGameQml`.
You don't create an `Rg` instance. The spelling has a lowercase `g`.

Follow `Rg.profileList.mainProfile` to the active profile, then read
`vars.generalVars` for preferences shared across themes. The example binds a
local `profile` property so the rest of the file can use a shorter expression.

\snippet 03-profile/main.qml profile

The greeting uses `generalVars.name`. If the active profile changes, the
binding follows the new profile and the label updates.

\snippet 03-profile/main.qml profile-label

Use the main profile for shared screens such as the menu. During gameplay,
use the profile attached to each player for that player's display. A battle
can involve two profiles, and a result's score owner need not be the current
main profile. Later lessons show the supplied player and result objects.

## Declare a setting for the menu

The manifest points the `main` role to a settings file as well as a QML file.
The game reads the setting definitions and Default builds a settings form for
them, so you don't need to write your own color picker.

\include 03-profile/theme.json

\include 03-profile/main-settings.json

`id` is the name used in QML. `name` and `description` are the text shown in
the settings form, and `default` supplies the first value for a profile.
The `color` type tells the form which control to use.

Saved values live under `profile.vars.themeVars[screenRole][themeName]`.
The example uses `"main"` for its role and `screen.QmlUtils.themeName` for
its folder name. `QmlUtils` is an attached object, so the value describes the
theme that contains `screen`. Don't put `"03-profile"` in the lookup, because
the user can rename the folder.

Theme settings belong to a profile, a screen role and a theme name. Read your
own role's settings even when several roles use the same QML file. Another
screen may come from another theme. Renaming your folder also changes the name
under which settings are saved, so the renamed copy can start with defaults.

## Try the saved value

Open Settings > Themes > Main Menu and change Accent color. Return to the
menu, then restart the game to check that the color persists. Change the
profile's name in its player settings and check the greeting too.

The text uses `qsTr()` and `%1` rather than joining a greeting and a name with
`+`. A translator can move `%1` to the right place for their language. The next
lesson adds the translation files.

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
| Build online ranking or Arena UI         | `onlineScores`, `arenaSession`, `arenaDirectorySession`               |

Most screens won't need to use those objects directly. For example,
`StandardSelectController` already handles ordinary browsing and chart
activation through `chartLoader`. `globalRoot` is a separate object supplied
by the game for navigation, such as `globalRoot.returnToPreviousScreen()`.
Use its named operations rather than accessing the internal screen stack.

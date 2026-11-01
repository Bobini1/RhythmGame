# 3. Read a profile and save a theme setting {#theme_tutorial_rg}

[Previous](https://bobini1.github.io/RhythmGame/theme_tutorial_qml.html) | [Tutorial](https://bobini1.github.io/RhythmGame/theme_tutorial.html) | [Next: Translations](https://bobini1.github.io/RhythmGame/theme_tutorial_translations.html)

**Goal:** Greet the active player by name and save a greeting color for their
profile.

Install `docs/examples/theme-tutorial/03-profile/` and choose it for Main Menu.

[Download 03-profile](https://bobini1.github.io/RhythmGame/examples/theme-tutorial/03-profile.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/03-profile).

## Read the active profile

[Rg](https://bobini1.github.io/RhythmGame/classRg.html) gives QML access to the game's shared objects. It is a singleton, which
means you use the existing object by name after importing `RhythmGameQml`.
You don't create an [Rg](https://bobini1.github.io/RhythmGame/classRg.html) instance.

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
The settings screen uses these definitions to create controls for changing
the values. A setting with type `hidden` has no control.

\include 03-profile/theme.json

\include 03-profile/main-settings.json

`id` is the name used in QML. `name` and `description` are the text shown in
the settings form, and `default` supplies the first value for a profile.
The `color` type tells the form which control to use.

Saved values live under `profile.vars.themeVars[screenName][themeName]`.
The example uses `"main"` for its screen and `QmlUtils.themeName` for its folder
name, so it still works if a user renames the folder.
[QmlUtils](https://bobini1.github.io/RhythmGame/classqml__components_1_1QmlUtilsAttached.html) is
an [attached property](https://doc.qt.io/qt-6/qtqml-typesystem-attachedtypes.html).

Settings belong to a profile, screen and theme. Read only the current screen's
settings; another screen may use a different theme.

If a theme omits `k5` or `k10`, the game can use its `k7` or `k14` screen,
respectively. These fallback screens share the original screen's settings.

## Try the saved value

Open Settings > Themes > Main Menu and change Accent color. Return to the
menu, then restart the game to check that the color persists. Change the
profile's name in its player settings and check the greeting too.

The next lesson adds translations for the greeting, buttons and color setting.

## Find other game data

The \ref Rg reference lists the available game services. Later lessons use
standard components to handle tasks such as browsing and opening charts.
For screen navigation, use the separate `globalRoot` object, for example
`globalRoot.returnToPreviousScreen()`.

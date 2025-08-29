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

You can get the current theme's name with `QmlUtils.themeName`. As for `screen`,
your theme should be able to figure it out on its own. It can be `"result"`, `"gameplay"`, etc.

Please don't access vars that don't belong to the current screen.
It creates hidden dependencies on other themes or screens of your theme that might not be enabled.

If you need to pass a variable between screens, suggest adding it to profile.vars.generalVars
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

The part of the game that manages
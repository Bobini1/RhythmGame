# Translating the game

Translations are theme-specific.
The C++ part of the game does not contain any translatable strings.

In a theme, translatable string come in two forms:

- Theme settings strings (used only in the settings screen)
- Strings in QML files

## Theme settings strings

Those are quite straightforward. If you take a look at
[the json file](https://github.com/Bobini1/RhythmGame/blob/master/share/RhythmGame/themes/Default/settings/k7.json)
defining the settings for the k7 screen, you will notice that the name and description
of each setting are defined in the file itself.
Simply add new languages next to "en" and "pl" (English and Polish).

Locale identifiers use a supported subset of BCP 47: an ISO 639 language code,
an optional ISO 15924 script code, and an optional ISO 3166-1 territory code.
For example, `fr-CA` is Canadian French, `zh-Hans` is Simplified Chinese,
`zh-Hant-TW` is Traditional Chinese for Taiwan, and `zh-Hant-HK` is
Traditional Chinese for Hong Kong. Hyphens are preferred. Existing identifiers
with underscores, such as `fr_CA`, remain accepted and are normalized at load
time.

Script subtags matter when a language uses more than one writing system. The
game first looks for the same language, script, and territory, then the same
language and script, and finally a generic language entry. It does not select
an explicitly incompatible script while a compatible generic entry exists.

For example, here is a property with an added Canadian French translation:

```json
{
  "id": "verticalGauge",
  "name": {
    "en": "Vertical Gauge",
    "pl": "Pionowy wskaźnik",
    "fr-CA": "Jauge verticale"
  },
  "description": {
    "en": "Whether the life gauge is vertical (fills upwards).",
    "pl": "Pionowy wskaźnić życia (wypełnia się w górę).",
    "fr-CA": "Si la jauge de vie est verticale (se remplit vers le haut)."
  },
  "type": "boolean",
  "default": false
}
```

To make a language selectable in the game, the theme's `theme.json` file needs to declare it.
This can look like this:

```json
{
  "translations": {
    "en": "translations/Default_en.qm",
    "pl": "translations/Default_pl.qm",
    "fr-CA": "",
    "zh-Hans": "translations/Default_zh_Hans.qm",
    "zh-Hant-TW": "translations/Default_zh_Hant_TW.qm"
  }
}
```

# Translating a theme

The qm file is a compiled Qt translation file. Those are used for translating strings in QML files,
which is explained in the next section. Providing a qm file is optional, you can leave the value empty if
your theme does not contain any translatable strings in QML files.

## Strings in QML files

To mark a string in a QML file as translatable, you need to wrap it in the `qsTr()` function.
An in-depth explanation of how to use it can be found in the
[Qt documentation](https://doc.qt.io/qt-6/i18n-source-translation.html#qml-use-qstr).

For translating strings in QML files, you will need to use the Qt Linguist toolchain.
There are three essential tools:

- `lupdate`: Extracts translatable strings from QML files and creates a `.ts` file.
- `Linguist`: A GUI tool for translators to translate the strings in a `.ts` file.
- `lrelease`: Compiles a `.ts` file into a `.qm` file that can be used by the game.

This repository contains a CMake target called `update_translations` that will run `lupdate` for you for the default
theme.
Add your language to `qt_standard_project_setup` in [CMakeLists.txt](CMakeLists.txt) to generate a `.ts` file for it.
There is also a target called `release_translations` that will run `lrelease`
(it also runs automatically when you build the game).

But if you're just a translator who doesn't want to build the game or you're developing your own theme,
you can also run the tools manually in the command line.
You can get them by [installing Qt](https://www.qt.io/download-qt-installer-oss).

Here is what it should look like inside Qt Linguist:

![Linguist](docs/images/linguist.png)

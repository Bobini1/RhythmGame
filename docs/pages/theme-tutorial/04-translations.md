# 4. Add a translation {#theme_tutorial_translations}

[Previous](03-profile.md) | [Tutorial](index.md) | [Next: Song selection](05-select.md)

Install `docs/examples/theme-tutorial/04-translations/` and choose it for Main
Menu. It contains the previous menu with Polish translations for the greeting,
buttons and color setting. The compiled translation is included, so you can
try it before installing any translation tools.

[Download 04-translations](examples/theme-tutorial/04-translations.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/04-translations).

## Mark text in QML

`qsTr()` marks a literal string for translation. `%1` is replaced with the
profile name after translation, so the name itself isn't translated.

\snippet 04-translations/main.qml profile-label

Qt groups translations by context. For `qsTr()` the default context is the
QML file's name without `.qml`, which is `main` here. The context in the
translation file must match. If you rename a QML file, update its translations
as well. A shared component can set a fixed context with `pragma Translator`.

## Include the translation files

The manifest declares `en` and `pl` as available languages. An empty English
path uses the source text. The Polish path points to a compiled `.qm` file.

\include 04-translations/theme.json

The editable `.ts` file contains the source strings and translations.

\include 04-translations/translations/tutorial_pl.ts

Open Settings > General > Language, choose Polish, and return to the menu.
The buttons should read "Wybór utworu", "Ustawienia" and "Wyjdź". Choose English again
to return to the source text. The language preference belongs to the profile,
so other screens with Polish translations will change too.

Settings labels use translations stored directly in JSON. They don't pass
through `lupdate` or the QML translation file.

\include 04-translations/main-settings.json

## Edit and compile a translation

To change the translated strings, install Qt's Linguist tools. With their
directory on your command search path, run the following from inside
`04-translations`. `lupdate` collects the strings, Linguist lets you edit the
translations, and `lrelease` builds the file loaded by the game.

```console
lupdate main.qml -ts translations/tutorial_pl.ts
linguist translations/tutorial_pl.ts
lrelease translations/tutorial_pl.ts -qm translations/tutorial_pl.qm
```

Keep both files in your source folder and include the `.qm` file when sharing
the theme. Restart the game after replacing it. Add any new QML files to the
`lupdate` command so their strings are collected too.

The [translation guide](../../../DEV_LANG.md) explains language tags such as
`fr-CA` and `zh-Hant-TW`. Qt's [source translation guide](https://doc.qt.io/qt-6/i18n-source-translation.html)
explains contexts and placeholders in more detail.

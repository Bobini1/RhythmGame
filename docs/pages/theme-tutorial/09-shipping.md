# 9. Complete theme example {#theme_tutorial_shipping}

[Previous](08-results.md) | [Tutorial](index.md)

The earlier lesson folders are deliberately independent. `09-complete` is the
one self-contained theme that combines their screens and helpers in one folder.
It is a working reference rather than a new feature lesson: lessons 1 through 8
explain the behavior of each screen.

[Download 09-complete](examples/theme-tutorial/09-complete.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/09-complete).

Its manifest registers the main menu, enhanced select screen, decide, gameplay,
chart result and course result screens. One gameplay file serves several
keymodes; the chart and course result screens have separate files.

\include 09-complete/theme.json

The main menu retains the profile greeting and saved color setting from lesson 3,
and the Polish translation from lesson 4. The normal path runs from selection
through decide, gameplay and result; courses show a chart result after each
stage, then the course result.

It does not implement Settings or Multiplayer. Default's Multiplayer browser
will open this theme's Select screen after joining an Arena room. The Select,
Gameplay and Result screens use the Arena panels from the [selection](05-select.md),
[gameplay](07-gameplay.md) and [result](08-results.md) lessons.

For supported contracts and customization points, see the [Theme API](../theme-api.md),
[theme reference](../../../DEV_THEME.md) and [QML reference](qml/qml-api.html).

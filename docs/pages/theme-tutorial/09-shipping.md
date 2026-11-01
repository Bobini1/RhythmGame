# 9. Complete theme example {#theme_tutorial_shipping}

[Previous](@ref theme_tutorial_results) | [Tutorial](@ref theme_tutorial)

**Goal:** Install the combined theme and use it as a starting point for your own.

`09-complete` brings the screens and helpers from lessons 1 through 8 into one
folder. Use the earlier lessons to look up how each screen works.

[Download 09-complete](examples/theme-tutorial/09-complete.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/09-complete).

Copy `docs/examples/theme-tutorial/09-complete/` into your theme directory and
restart. In Settings > Themes, choose `09-complete` for Main Menu, Select,
Decide, your gameplay modes, Result and Course Result. Keep your existing
Settings and Multiplayer themes.

The manifest lists these screens. One gameplay file serves all supported
keymodes; chart and course results use separate files.

\include 09-complete/theme.json

The main menu retains the profile greeting and saved color setting from lesson 3,
and the Polish translation from lesson 4. The normal path runs from selection
through decide, gameplay and result; courses show a chart result after each
stage, then the course result.

Default's Multiplayer browser opens this theme's Select screen after joining
an Arena room. The Select, Gameplay and Result screens use the Arena panels
from the [select](@ref theme_tutorial_select),
[gameplay](@ref theme_tutorial_gameplay) and [result](@ref theme_tutorial_results) lessons.

To make your own version, copy and rename the whole folder. Keep its helper
files, settings and assets together. Change one screen at a time and restart
to check it.

For component properties and customization points, see the
[QML reference](qml/qml-api.html).

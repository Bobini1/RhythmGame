# Create a theme {#theme_tutorial}

Build a theme, starting with a main menu and adding song selection, gameplay
and results. A theme can replace one screen or several. Players choose a theme
for each screen, so your theme can work alongside others.

You need RhythmGame and a text editor. Themes use
[QML](https://doc.qt.io/qt-6/qtqml-index.html) to describe the interface and
JavaScript for logic. The first lessons introduce the basics; you don't need
to compile the game. Save your changes and restart to load them.

## Follow the lessons

Each lesson has a corresponding folder under `docs/examples/theme-tutorial/` and a
ZIP download on the generated documentation site. You can install any lesson
on its own. Each ZIP contains a single theme folder, including its manifest,
QML files and any assets it uses.

| Lesson                               | Installable folder | What you will see                                                 |
|--------------------------------------|--------------------|-------------------------------------------------------------------|
| \subpage theme_tutorial_first_screen | `01-main`          | An image with Song select, Multiplayer, Settings and Quit buttons |
| \subpage theme_tutorial_qml          | `02-qml`           | Reusable buttons and a color change                               |
| \subpage theme_tutorial_rg           | `03-profile`       | A profile greeting with a saved color setting                     |
| \subpage theme_tutorial_translations | `04-translations`  | The same menu in English or Polish                                |
| \subpage theme_tutorial_select       | `05-select`        | Browse charts, then show saved scores and rankings                |
| \subpage theme_tutorial_decide       | `06-decide`        | A confirmation screen before play                                 |
| \subpage theme_tutorial_gameplay     | `07-gameplay`      | Notes, long notes, mines and a live score                         |
| \subpage theme_tutorial_results      | `08-results`       | Chart results and a separate course summary                       |
| \subpage theme_tutorial_shipping     | `09-complete`      | A self-contained version of lessons 1 through 8                  |

Start with [the first menu](https://bobini1.github.io/RhythmGame/theme_tutorial_first_screen.html), even if you plan to work on
gameplay. It explains how to install, select and reload an example. Later
lessons use the same steps for other screens.

## Read the code alongside the game

The code snippets show the parts used in each step. The download and source
folder contain the complete files. Comments
such as `// [main-actions]` mark the start and end of a snippet for the docs
generator. QML ignores the comments, so you can remove them in your own copy.
GitHub displays the include commands as text. Read the [generated documentation
site](https://bobini1.github.io/RhythmGame/pages.html) to see them expanded.

The examples use simple visuals and reusable components for browsing, input
and screen transitions. You can change the appearance while keeping that
behavior, then replace individual actions when needed.

## Find more detail

The [theme overview](../../../DEV_THEME.md) introduces how themes fit into the
game. The [QML reference](https://bobini1.github.io/RhythmGame/qml/qml-api.html) describes the Standard components,
their properties and their actions.

## If you get stuck {#theme_tutorial_stuck}

If a broken theme prevents you from opening Settings, close the game and edit
your profile's theme config file:

| Platform | Theme config file                                                               |
|----------|---------------------------------------------------------------------------------|
| Windows  | `%APPDATA%/RhythmGame/profiles/<profile>/theme_config.json`                     |
| Linux    | `~/.local/share/RhythmGame/profiles/<profile>/theme_config.json`                |
| macOS    | `~/Library/Application Support/RhythmGame/profiles/<profile>/theme_config.json` |

Set the affected screen to `"Default"`. To reset all screen choices, use:

```json
{
  "courseResult": "Default",
  "decide": "Default",
  "k10": "Default",
  "k14": "Default",
  "k5": "Default",
  "k5battle": "Default",
  "k7": "Default",
  "k7battle": "Default",
  "main": "Default",
  "multiplayer": "Default",
  "result": "Default",
  "select": "Default",
  "settings": "Default"
}
```

You will need to restart the game to see the changes.

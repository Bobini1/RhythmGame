# Create a theme {#theme_tutorial}

A RhythmGame theme can provide one or more screens - like select, decide, k7, k14, main.
The game can use a different theme for each screen, in a mix-and-match fashion.
In this tutorial, we will start with the main screen to show the basics and then proceeds to more advanced scenarios.
In general, The level of difficulty for a theme author goes (easiest to hardest) - main, decide, result, select,
gameplay.
It is also possible for a theme to override settings and the lobby list view.

The language used for writing themes is [QML](https://doc.qt.io/qt-6/qtqml-index.html).
It's like a declarative tree of components + JavaScript for any logic you might need.
QML was picked as the technology for this project because it has a ton of useful components you can use out of the box
and it can be loaded at runtime, without any compilation step. You can learn it as you go through this tutorial
but you will surely need to look at Qt's docs eventually anyway. Unless an LLM does that for you.

## Follow the lessons

Each lesson has a corresponding folder under `docs/examples/theme-tutorial/` and a
ZIP download on the generated documentation site. You can install any lesson
on its own. Each ZIP contains a single theme folder, including its manifest,
QML files and any assets it uses.

| Lesson                               | Installable folder | What you will see                                  |
|--------------------------------------|--------------------|----------------------------------------------------|
| \subpage theme_tutorial_first_screen | `01-main`          | An image with Song select, Settings and Quit buttons |
| \subpage theme_tutorial_qml          | `02-qml`           | Reusable buttons and a color change                |
| \subpage theme_tutorial_rg           | `03-profile`       | A profile greeting with a saved color setting      |
| \subpage theme_tutorial_translations | `04-translations`  | The same menu in English or Polish                 |
| \subpage theme_tutorial_select       | `05-select`        | A list of folders, charts and courses              |
| \subpage theme_tutorial_decide       | `06-decide`        | A confirmation screen before play                  |
| \subpage theme_tutorial_gameplay     | `07-gameplay`      | Notes, long notes, mines and a live score          |
| \subpage theme_tutorial_results      | `08-results`       | Chart results and a separate course summary        |
| \subpage theme_tutorial_assets       | `09-assets`        | Chart images, preview audio and saved score counts |
| \subpage theme_tutorial_shipping     | `10-complete`      | The screens combined into one theme                |

Start with [the first menu](01-first-screen.md), even if you plan to work on
gameplay. It explains how to install, select and reload an example. Later
lessons use the same steps for other screen roles.

## Read the code alongside the game

Every displayed snippet comes from a file in the lesson's folder. Comments
such as `// [main-actions]` mark the start and end of a snippet for the docs
generator. QML ignores the comments, so you can leave them in your own copy.
GitHub displays the include commands as text. Read the [generated documentation
site](https://bobini1.github.io/RhythmGame/pages.html) to see them expanded.

The examples keep their appearance simple so you can follow their behavior.
They use the set of reusable components for browsing, input and transitions that was prepared so that you don't have to
reinvent the wheel. Although you can override almost everything if you feel the need to do so.

## Find more detail

The [theme reference](../../../DEV_THEME.md) lists the screen contracts and
settings format. The [QML reference](qml/qml-api.html) describes the standard
components. The [architecture notes](../theme-architecture.md) explain why the
API uses optional components inside each screen.

The examples describe the current source API. Builds from before the screen
context changes cannot load the later lessons. See the
[maintenance notes](authoring.md) for the checks used when updating the tutorial.

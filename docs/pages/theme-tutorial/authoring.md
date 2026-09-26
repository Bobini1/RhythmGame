# Maintain the theme tutorial {#theme_tutorial_authoring}

[Tutorial](index.md)

Each reader lesson has an independently installable directory under
`docs/examples/theme-tutorial/`. Keep the example working when editing its
explanation. Snippets come from the QML files through Doxygen include commands,
so there is no second copy of the code in the prose.

## Update the source and the lesson together

Use the full path below `EXAMPLE_PATH`, such as `07-gameplay/Gameplay.qml`,
in each include or snippet. Filenames repeat across lessons, so a bare
`main.qml` would be ambiguous. Pair snippet comments and include enough code
to explain where the shown properties belong.

When an example changes, update its copy in `10-complete` if that screen is
included there. The deliberate duplication lets readers copy any folder into
the game without installing a shared tutorial package. Keep lesson 4's `.ts`
and compiled `.qm` files together, including their copies in `10-complete`.

The `docs` target creates a ZIP for each lesson folder and copies the source
folders into the generated site. Download links point to those archives.
Do not add a folder without a valid `theme.json` to the lesson archive list.

## Check the generated pages and examples

Build the `docs` target and inspect `docs/html/theme_tutorial.html` in the build
directory. Check links, snippet contents and figures, then open a downloaded
archive to confirm it contains the entire theme folder.

Validate manifests against `schemas/theme.json` and settings against
`schemas/themeSettings.json`. Run QML lint with the project's module paths.
The application registers some domain types at runtime, so standalone lint
can report unresolved types even when the game can load them. Record those
warnings rather than describing the result as warning-free.

Try changed screens in the game before claiming gameplay or visual coverage.
For gameplay changes, include courses, double play and battle as applicable.
For result changes, check both a course stage and the aggregate summary.
Automated syntax or lifecycle checks do not replace play checks.

## Keep the public API clear

Teach ordinary QML themes through their context objects and Standard components.
LR2 is an internal adapter and does not define the public API. Keep the
normal-result and course-result roles distinct even if a theme shares their
presentation.

The tutorial follows the source revision that contains it. It does not claim
compatibility with older builds that lack the screen context API. Record the
game build and tested play modes when publishing a release of an example theme.

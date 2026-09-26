# 5. Browse songs and courses {#theme_tutorial_select}

[Previous](04-translations.md) | [Tutorial](index.md) | [Next: Decide](06-decide.md)

Install `docs/examples/theme-tutorial/05-select/` and choose it for Select.
The folder provides only song selection, so keep your current choices for
Main Menu, Decide and gameplay. Open Song select to see the new list.

[Download 05-select](examples/theme-tutorial/05-select.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/05-select).

## Use the standard browsing behavior

`StandardSelectController` loads folders and tables, remembers the path back,
and opens charts or courses. It also supplies input mappings and selection
shortcuts. The theme supplies the list and tells the controller which item is
focused.

\snippet 05-select/Select.qml select-controller

`autoInitialize: false` lets the screen wait until its children exist before
opening the root folder. The screen calls `initialize()` from
`Component.onCompleted`. The controller's `entries` contain one copy of each
filtered item, which suits a normal list. A circular wheel can use
`presentationEntries` to repeat entries when it needs more visible rows.

Use the complete controller once. Adding `StandardSelectInput` or
`StandardSelectShortcuts` alongside it would add behavior already included
by the controller.

## Keep list focus and selection together

When `currentIndex` changes, the screen calls `setFocused()` with the entry
at that index. In the other direction, `focusRequested` restores the row
chosen by the controller, such as when returning from a folder.

\snippet 05-select/Select.qml select-focus

The `ListView` keeps keyboard focus and forwards arrow presses and releases
to the controller. `keyNavigationEnabled: false` prevents the view from also
handling the same arrow presses. The controller emits `moveRequested`, and
the theme moves its current row. The modulo calculation wraps past either end
of the list.

A delegate describes one visible row. Its required `modelData` receives the
entry, while `index` identifies the row. `labelFor()` handles both directory
paths and objects with a title or name. Clicking a row focuses and activates it.

The `ComponentBehavior: Bound` pragma at the top keeps delegates in the
context where they were defined. The row can therefore refer to `screen`
and `songList`, while its required properties receive the model values.

## Include the Arena room panel

The selector also adds `StandardArenaSelectOverlay`. The panel appears when
you join an Arena room and supplies ready controls and chat. Closing chat
returns focus to `songList`. Local selection keeps the panel hidden.

\snippet 05-select/Select.qml arena-select

The [complete example](10-shipping.md) explains how each screen adds its own
Arena panel and how you can change its appearance.

\include{lineno} 05-select/Select.qml

## Try a complete browsing cycle

Enter a song folder, choose a chart, and play it using your existing gameplay
theme. After closing the result, the list should still show the folder you
left. Use Back or Left to go up a folder, and Leave selection to leave the screen. In Arena, Leave selection and Escape
leave the room and return to the browser.

Bound controller keys 1 and 3 activate an entry, key 5 starts autoplay, and
scratch moves the selection. F2 reloads the current folder or table, F3 opens
the selected folder, and F12 opens Settings. The example disables F11 because
it has no ranking view. Replay key 7 also needs a theme action that chooses a
saved replay, which the minimal list doesn't provide.

If you have a difficulty table with courses, open it and choose a course.
The same activation call opens the course. In Arena, choosing a chart proposes
it to the room instead. Press Start twice on either player side to toggle ready.
The example's Arena panel describes that gesture using the controller's
`readyShortcutDescription`. An empty list can mean there are
no scanned songs or that the profile's selection filters exclude them.

The [controller reference](qml/qml-rhythmgameqml-standardselectcontroller.html)
lists the optional actions. Start by changing the list's appearance before
replacing browsing behavior.

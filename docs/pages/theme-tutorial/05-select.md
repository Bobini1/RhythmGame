# 5. Browse songs and courses {#theme_tutorial_select}

[Previous](04-translations.md) | [Tutorial](index.md) | [Next: Decide](06-decide.md)

Install `docs/examples/theme-tutorial/05-select/` and choose it for Select.
The folder provides only song selection, so keep your current choices for
Main Menu, Decide and gameplay. Open Song select to see the new list.

[Download 05-select](examples/theme-tutorial/05-select.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/05-select).

## Use the standard browsing behavior

[StandardSelectController](qml/qml-rhythmgameqml-standardselectcontroller.html) loads folders and tables, remembers the
path back, and opens charts or courses. It also supplies input mappings and selection
shortcuts, background music and chart previews. The theme draws the rows, and
`StandardSelectListView` connects the current row to the controller.

\snippet 05-select/Select.qml select-controller

The controller opens the root folder automatically. Its `entries` contain one
instance of each filtered item, which suits a normal list. A circular wheel can use
`presentationEntries` to repeat entries when it needs more visible rows.

Use the complete controller once. Adding [StandardSelectInput](qml/qml-rhythmgameqml-standardselectinput.html) or
[StandardSelectShortcuts](qml/qml-rhythmgameqml-standardselectshortcuts.html) alongside it would add behavior already
included by the controller.
You can use those constituent components instead of the controller if you want some more custom behavior.

The controller plays the selection track from the active profile's BGM set.
When a focused chart's preview starts, the background music stops. The
standard list holds a pending preview until scrolling stops.
Disabling the controller stops both kinds of audio, including while gameplay
is open. The [assets lesson](09-assets.md) explains the preview settings.

## Recognize the entry types {#theme_tutorial_select_entry_types}

The controller's `entries` list contains several types. Each delegate receives
one of them as `modelData`. Check its type before reading properties such as
`title` or `name`.

| Entry                          | Type check                  | Useful fields for a delegate                                                            | What `selection.goForward(item)` does                                                                                     |
|--------------------------------|-----------------------------|-----------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------|
| Song folder                    | `typeof item === "string"`  | The value is the folder path. Use its last path segment as a label.                     | Opens the folder.                                                                                                         |
| Installed chart                | `item instanceof ChartData` | `title`, `subtitle`, `artist`, `playLevel`, `keymode`, `md5`, `path`, `stageFileSource` | Starts play, or proposes the chart in Arena.                                                                              |
| Missing table chart            | `item instanceof entry`     | `title`, `subtitle`, `artist`, `level`, `md5`, `url`, `urlDiff`                         | Does nothing. The table describes the chart, but the chart was not found in the song database.                            |
| Difficulty table or collection | `item instanceof table`     | `name`, `symbol`, `levels`, `courses`, `identifier`                                     | Opens its levels and courses.                                                                                             |
| Table level                    | `item instanceof level`     | `name`, `entries`                                                                       | Opens its charts. Installed charts become `ChartData`; missing charts remain `entry`.                                     |
| Course                         | `item instanceof course`    | `name`, `chartIds`, `constraints`, `unavailableReason`                                  | Requests course play. The course must be supported and all stages must be installed. Course play is unavailable in Arena. |

`ChartData` starts with a capital letter. The names `entry`, `table`, `level`
and `course` are lowercase QML value types. The example has
[`pragma ValueTypeBehavior: Addressable`](https://doc.qt.io/qt-6/qtqml-documents-structure.html#valuetypebehavior)
at the top of the file so it can use those names in `instanceof` checks.
It is required!

The example uses one `ItemDelegate` and chooses its text by type. Folder,
table, level and course labels identify what opening the row will do.
Missing charts get a separate label, while installed charts show their title.

\snippet 05-select/Select.qml entry-labels

The generated references list the fields on
[ChartData](classgameplay__logic_1_1_chart_data.html),
[entry](structresource__managers_1_1_entry.html),
[table](structresource__managers_1_1_table.html),
[level](structresource__managers_1_1_level.html) and
[course](structresource__managers_1_1_course.html).

## Handle an empty folder or selection

When browsing or filtering produces no entries, `entries` is `[]`,
`focusedItem` is `null` and `focusedIndex` is `-1`.
The example shows a separate message above the empty list:

\snippet 05-select/Select.qml select-empty

A circular wheel can use `presentationEntries` instead.
It duplicates entries up to the specified minimum count.
When there are 0 entries in the logical list,
`presentationEntries` uses `null` placeholders.

Pass the row's data to `setFocused()`. The QML delegate is the visual item
that displays the row, so don't pass the delegate itself.

A missing chart from a table shows up as an `entry` with metadata.

An unsuccessful search leaves the previous list in place.

## Show the stages of a course

The example shows a stage list when a course is selected. It keeps the stage
order, including repeated charts, and marks missing charts as not installed.
The list scrolls within a limited height so long courses leave room for browsing.

\snippet 05-select/Select.qml course-stages

A course's `loadCharts()` result contains a `ChartData` for each installed
stage. A missing stage is a string containing its MD5 or `sha256/<hash>`
identifier. Those strings are not folder paths.

Pass either form of identifier to
[Rg.tables.search ()](classresource__managers_1_1_tables.html). It searches
downloaded tables and external collections without needing the chart installed.
Matching ignores letter case, and an unknown identifier returns an empty list.

`CourseStages.qml` uses the first match's `entry` to show a missing chart's
title and subtitle. If no title is known, it displays the identifier.

\include{lineno} 05-select/CourseStages.qml

## Connect a list to the controller

[StandardSelectListView](qml/qml-rhythmgameqml-standardselectlistview.html)
extends `ListView` with the navigation needed by a song selector. Set its
`controller` and supply a delegate for each row.

\snippet 05-select/Select.qml select-list

The list keeps its current row and the controller's focused entry together.
Up, Down and the mouse wheel move between rows, wrapping at either end.
Return, Enter and Right open the current entry, and Left goes back. Bound
controller input uses the same selection. Going back from a folder restores
its row automatically.

Wheel input works while the pointer is over the list and changes the selected
entry. The controller accumulates small wheel movements, so a fine scroll wheel
also works without extra code in the theme.

A row's click handler calls `songList.activate(row.index)`. That method selects
the row, gives the list keyboard focus and opens the entry. The list also pauses
preview startup while it is moving.

## Give the list keyboard focus

Set `focus: true` on the list and its enclosing `Pane`, as the example does.
This tells Qt where to send keyboard events when the screen becomes active.
The chosen row and keyboard focus are separate: another button can have
keyboard focus while the same song remains selected.

For a custom wheel, use `StandardSelectController` directly. Handle its
`moveRequested` and `focusRequested` signals, then call `setFocused()` when
your wheel chooses an entry. The controller reference describes that interface.

## Draw the current row's highlight

A delegate describes one visible row. Its `modelData` receives the
entry, while `index` identifies the row. `labelFor()` handles the entry types
described above. Clicking a row makes it current and activates it.

\snippet 05-select/Select.qml select-delegate

The theme binds `highlighted` to `ListView.isCurrentItem`.
`ListView.isCurrentItem` is true for the row at `songList.currentIndex`, so
the highlight follows changes to the current row automatically.

## Filter by keymode

The keymode buttons use named `SelectKeymodeFilter` values. SP shows 5K and
7K charts, while DP shows 10K and 14K charts. All removes the keymode filter.
Folders, tables and courses remain in the list.

\snippet 05-select/Select.qml select-keymode

Each button's `checked` state follows the controller's
[keymodeFilter](qml/qml-rhythmgameqml-standardselectstate.html#keymodeFilter-prop).
`checkable` and `autoExclusive` keep one button selected. The `Flow` wraps the
buttons onto another row when the window is narrow.

Clicking a button calls `selection.setKeymodeFilter()`, which saves the choice
in the active profile and updates `selection.keymodeFilter`. Switching profiles
updates the filter to that profile's choice. Sorting and replay choices work
the same way through `setSortMode()` and `setReplayType()`.

The three properties are read-only. Bind your controls to them and use the
methods to change the choice. A setter returns `false` for an unknown value
or an inactive controller, and leaves the saved choice unchanged.

## Use the same actions from buttons and keys

The controller also supplies actions for sorting and autoplay.
The example calls those actions directly from buttons.

\snippet 05-select/Select.qml select-actions

[cycleSortMode ()](qml/qml-rhythmgameqml-standardselectstate.html#cycleSortMode-method)
cycles the available sort choices and saves the choice in the active profile.
Bound keys 2 and 4 use the same action at the root. Inside a folder they go back.
A menu that chooses a particular sort mode can call `setSortMode(mode)`.
The button displays
[sortModeText](qml/qml-rhythmgameqml-standardselectstate.html#sortModeText-prop),
the translated name of the current sort mode. "Total" means the chart's TOTAL
gauge value. It does not mean the number of notes.

[openAutoplay ()](qml/qml-rhythmgameqml-standardselectstate.html#openAutoplay-method)
starts the focused chart or course. `autoplayAvailable` keeps the button disabled
for folders, incomplete courses and Arena. Bound key 5 uses the same action,
and also enters folders while browsing.

Press F12 to open Settings. This uses the controller's `openSettings()` action.
The controller also exposes
`reloadCurrentFolderOrTable()` for F2 and `openSelectedFolder()` for F3.
Their optional action overrides apply to both buttons and shortcuts.

## Play a saved replay

The controller chooses a saved run using the active profile's replay setting.
The choices are newest, best score, best clear and best combo. The combo box
uses [replayOptions](qml/qml-rhythmgameqml-standardselectstate.html#replayOptions-prop).
Each option has a `text` label and a stable `value`. The two role properties tell
the combo box which field to display and which field to select.
`currentValue` follows `selection.replayType`, and `setReplayType(currentValue)`
saves the player's choice. A value is not a position in the list.

\snippet 05-select/Select.qml select-replay

These shared labels already follow the game language, so do not wrap them in
`qsTr()` again. They are optional. You can draw your own text or artwork and use
the same controller actions. To change one caption, choose your text in a
binding or derive your own model. Compare the option's `value`, rather than its
position in the list. The
[StandardSelectOptions reference](qml/qml-rhythmgameqml-standardselectoptions.html)
has an example and explains how to use the labels outside a selector.

The button calls [openReplay ()](qml/qml-rhythmgameqml-standardselectstate.html#openReplay-method),
which is also the action used by bound key 7. Bound key 6 cycles the replay
choice. No extra handler is needed on the controller.

`replayAvailable` keeps the button disabled until a usable recording is loaded.
Scores imported without replay data can still be viewed as results, but cannot
be played back. A course replay needs recordings for all its stages.
Replay playback uses the main profile alone and is unavailable in Arena.

For a custom replay picker, set the controller's `tryReplayAction` to a function
that returns true when it handles the request. Returning false lets the standard
action continue.

## Include the Arena room panel

Open Multiplayer from the main menu, then join an Arena room. The game opens
your configured selector, so the same example serves local and multiplayer play.
Keep Default selected for the `multiplayer` role when using this example.
Use Leave selection to leave the room and return to the browser. Closing the
browser returns to the main menu.

The selector also adds [StandardArenaSelectOverlay](qml/qml-rhythmgameqml-standardarenaselectoverlay.html).
The panel appears when
you join an Arena room and supplies ready controls, the double-Start ready
shortcut and chat. Closing chat
returns focus to `songList` thanks to the `navigationFocusTarget` property.
The panel is only visible in multiplayer mode.

\snippet 05-select/Select.qml arena-select

The [complete example](10-shipping.md) explains how each screen adds its own
Arena panel and how you can change its appearance.

\include{lineno} 05-select/Select.qml

## Try a complete browsing cycle

Enter a song folder, choose a chart, and play it using your existing gameplay
theme. After closing the result, the list should still show the folder you
left. Use Back or Left to go up a folder, and Leave selection to leave the screen.

Bound controller keys 1 and 3 activate an entry, key 5 starts autoplay, and
scratch moves the selection. F2 reloads the current folder or table, F3 opens
the selected folder, F11 opens the chart's online ranking page, and F12 opens
Settings. F11 uses the provider chosen in the profile's settings and works
without a ranking display in the theme. Try key 7 or the Replay button on a chart
you have played before, then use key 6 or the combo box to choose another run.

The controller's `ranking` is a
[StandardRanking](qml/qml-rhythmgameqml-standardranking.html). It follows the
focused chart and supplies entries, the player's position and loading state.
[Lesson 9](09-assets.md) adds a small display using those properties. A button
can call `selection.openInternetRanking()` to use the same action as F11.

If you have a difficulty table with courses, open it and choose a course.
The same activation call opens the course. In Arena, choosing a chart proposes
it to the room instead. Press Start twice on either player side to toggle ready.
The Arena overlay handles the shortcut and supplies the hint on its Ready
control. Set the overlay's `readyShortcutEnabled` to false if you want to provide
your own shortcut. Open an empty folder or choose a filter that excludes all
charts to check the empty-list message.

The [controller reference](qml/qml-rhythmgameqml-standardselectcontroller.html)
lists the optional actions. Start by changing the list's appearance before
replacing browsing behavior.

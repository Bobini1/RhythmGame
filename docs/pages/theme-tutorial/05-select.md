# 5. Browse songs and courses {#theme_tutorial_select}

[Previous](@ref theme_tutorial_translations) | [Tutorial](@ref theme_tutorial) | [Next: Decide](@ref theme_tutorial_decide)

**Goal:** Build a song list that opens charts and courses, then add saved
scores, replays and rankings.

Install `docs/examples/theme-tutorial/05-select/` and choose it for Select.
Keep your current themes for Main Menu, Decide and gameplay. Start with the
list; the chart details panel is optional.

[Download 05-select](examples/theme-tutorial/05-select.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/05-select).

## Use the standard browsing behavior

[StandardSelectController](qml/qml-rhythmgameqml-standardselectcontroller.html) loads folders and tables, remembers the
path back, and opens charts or courses. It also handles input, shortcuts,
background music and chart previews. The theme draws the rows, and
`StandardSelectListView` connects the current row to the controller.

\snippet 05-select/Select.qml select-controller

The controller opens the root folder automatically. Its `entries` contain one
instance of each filtered item, which suits a normal list. A circular wheel can use
`presentationEntries` to repeat entries when it needs more visible rows.

Use the complete controller once. Adding [StandardSelectInput](qml/qml-rhythmgameqml-standardselectinput.html) or
[StandardSelectShortcuts](qml/qml-rhythmgameqml-standardselectshortcuts.html) alongside it would add behavior already
included by the controller.
Use those smaller components when building your own browsing behavior.

Background music stops when a chart preview starts. The list waits until
scrolling stops before playing a preview. Both stop when selection becomes
inactive, such as during gameplay. See
[StandardSelectPreview](qml/qml-rhythmgameqml-standardselectpreview.html) to
change the preview delay, volume or audio source.

## Recognize the entry types {#theme_tutorial_select_entry_types}

The controller's `entries` list contains several types. Each delegate receives
one of them as `modelData`. Check its type before reading properties such as
`title` or `name`.

| Entry                          | Type check                  | Useful fields for a delegate                                                            | What `selection.goForward(item)` does                                                                                     |
|--------------------------------|-----------------------------|-----------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------|
| Song folder                    | `typeof item === "string"`  | The value is the folder path. You can use its last path segment as a label.             | Opens the folder.                                                                                                         |
| Installed chart                | `item instanceof ChartData` | `title`, `subtitle`, `artist`, `playLevel`, `keymode`, `md5`, `path`, `stageFileSource` | Starts play, or proposes the chart in Arena.                                                                              |
| Missing table chart            | `item instanceof entry`     | `title`, `subtitle`, `artist`, `level`, `md5`, `url`, `urlDiff`                         | Does nothing. The table describes the chart, but the chart was not found in the song database.                            |
| Difficulty table or collection | `item instanceof table`     | `name`, `symbol`, `levels`, `courses`, `identifier`                                     | Opens its levels and courses.                                                                                             |
| Table level                    | `item instanceof level`     | `name`, `entries`                                                                       | Opens its charts. Installed charts become `ChartData`; missing charts remain `entry`.                                     |
| Course                         | `item instanceof course`    | `name`, `chartIds`, `constraints`, `unavailableReason`                                  | Requests course play. The course must be supported and all stages must be installed. Course play is unavailable in Arena. |

`ChartData` starts with a capital letter. The names `entry`, `table`, `level`
and `course` are lowercase QML value types. The example has
[`pragma ValueTypeBehavior: Addressable`](https://doc.qt.io/qt-6/qtqml-documents-structure.html#valuetypebehavior)
at the top of the file. This is required for those `instanceof` checks.

The example uses one `ItemDelegate` and chooses its text by type. Folder,
table, level and course labels identify what opening the row will do.
Missing charts get a separate label, while installed charts show their title.

\snippet 05-select/Select.qml entry-labels

The generated references list the fields on
[ChartData](@ref gameplay_logic::ChartData),
[entry](@ref resource_managers::Entry),
[table](@ref resource_managers::Table),
[level](@ref resource_managers::Level) and
[course](@ref resource_managers::Course).

## Connect a list to the controller

[StandardSelectListView](qml/qml-rhythmgameqml-standardselectlistview.html)
extends `ListView` with the navigation needed by a song select screen. Set its
`controller` and supply a delegate for each row.

\snippet 05-select/Select.qml select-list

The list keeps its current row and the controller's focused entry together.
Up, Down and the mouse wheel move between rows, wrapping at either end.
Return, Enter and Right open the current entry, and Left goes back. Bound
controller input uses the same selection. Going back from a folder restores
its row automatically.

Wheel input works while the pointer is over the list and changes the selected
entry.

A row's click handler calls `songList.activate(row.index)`. That method selects
the row, gives the list keyboard focus and opens the entry. The list also pauses
preview startup while it is moving.

Set `focus: true` on the list and its enclosing `Pane`, as the example does.
This tells Qt where to send keyboard events when the screen becomes active.
The chosen row and keyboard focus are separate: another button can have
keyboard focus while the same song remains selected.

A delegate describes one visible row. Its `modelData` receives the
entry, while `index` identifies the row. `labelFor()` chooses the label by type.

\snippet 05-select/Select.qml select-delegate

Bind `highlighted` to `ListView.isCurrentItem` so the highlight follows the
current row.

For a custom wheel, handle the controller's `moveRequested` and `focusRequested`
signals. Call `setFocused()` with the row's data when the wheel chooses an entry.
The [controller reference](qml/qml-rhythmgameqml-standardselectcontroller.html)
describes this interface.

## Handle an empty folder or selection

When browsing or filtering produces no entries, `entries` is `[]`,
`focusedItem` is `null` and `focusedIndex` is `-1`.
The example shows a separate message above the empty list:

\snippet 05-select/Select.qml select-empty

A circular wheel's `presentationEntries` uses `null` placeholders when the
list is empty. Check for null before reading a row's properties. Missing table
charts still appear as `entry` objects. An unsuccessful search leaves the
previous list in place.

## Show the stages of a course

The example shows a stage list when a course is selected. It keeps the stage
order, including repeated charts, and marks missing charts as not installed.
The list scrolls within a limited height so long courses leave room for browsing.

\snippet 05-select/Select.qml course-stages

A course's `loadCharts()` result contains a `ChartData` for each installed
stage. A missing stage is a string containing its MD5 or `sha256/<hash>`
identifier. Those strings are not folder paths.

`CourseStages.qml` looks up missing charts with
[Rg.tables.search()](@ref resource_managers::Tables). It uses the first
match's title and subtitle, or displays the identifier if no title is known.

\include{lineno} 05-select/CourseStages.qml

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

These properties are read-only. Bind controls to them and use the methods to
change the choice.

## Use the same actions from buttons and keys

Buttons call the same controller actions as bound keys.

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
Their action overrides apply to both buttons and shortcuts.

## Play a saved replay

The controller chooses a saved run using the active profile's replay setting.
The choices are newest, best score, best clear and best combo. The combo box
uses [replayOptions](qml/qml-rhythmgameqml-standardselectstate.html#replayOptions-prop).
Each option has a `text` label and a stable `value`. The two role properties tell
the combo box which field to display and which field to select.
`currentValue` follows `selection.replayType`, and `setReplayType(currentValue)`
saves the player's choice.

\snippet 05-select/Select.qml select-replay

These labels are already translated; use them without `qsTr()`. For custom
labels, see
[StandardSelectOptions](qml/qml-rhythmgameqml-standardselectoptions.html).

The button calls [openReplay ()](qml/qml-rhythmgameqml-standardselectstate.html#openReplay-method),
which is also the action used by bound key 7. Bound key 6 cycles the replay
choice. No extra handler is needed on the controller.

`replayAvailable` keeps the button disabled until a usable replay is loaded.
Scores imported without replay data can still be viewed as results, but cannot
be played back. A course replay needs replay data for all its stages.
Replay playback uses the main profile alone and is unavailable in Arena.

## Add an optional chart details panel

The panel shows the selected chart's image, saved scores and timing graph.
Its `chartData` binding is null for folders, courses and missing table entries.

\snippet 05-select/Select.qml selected-chart-state

`ChartData.stageFileSource` is ready to use as an `Image` URL, including for a
chart inside an archive. The example shows a small placeholder when a chart has
no image. For a raw local path, use `Rg.fileQuery.localFileUrl(path)` instead
of constructing a `file:///` URL yourself.

\snippet 05-select/Select.qml chart-image

The controller's `results` map holds [BmsResult](@ref gameplay_logic::BmsResult)
summaries without replay data. It updates after scores are saved or imported.
`StandardRanking` finds the best clear and best score from these results; they
can be different runs.

\snippet 05-select/Select.qml saved-results

Use [ScoreDetails](qml/qml-rhythmgameqml-scoredetails.html) when the panel needs
a full score, such as replay hit events or gauge history. The example loads the
latest saved result. Changing the selected result cancels the previous load, so
the graph cannot show replay data for the chart you just left.

\snippet 05-select/Select.qml score-details

The graph receives the full score's replay data and the selected chart's timing
windows. It keeps the same fixed scale that the [result lesson](@ref theme_tutorial_results)
uses. A saved result can exist without a replay, so the panel distinguishes
no saved score, a replay that is loading, a failed load and a score without
replay data.

\snippet 05-select/Select.qml replay-graph

`selection.ranking` follows the selected chart and the provider saved in the
profile. The provider picker is optional. `openInternetRanking()` is the same
action as F11, and the controller cancels an unfinished request when selection
changes or the screen closes.

\snippet 05-select/Select.qml select-ranking

## Include the Arena room panel

Open Multiplayer from the main menu, then join an Arena room. The game opens
your configured select screen, so the same example serves local and multiplayer play.
Keep Default selected for the `multiplayer` screen when using this example.
Use Leave selection to leave the room and return to the browser.

Add [StandardArenaSelectOverlay](qml/qml-rhythmgameqml-standardarenaselectoverlay.html)
for ready controls and chat. It appears only in an Arena room. Press START
twice to toggle ready. `navigationFocusTarget` returns keyboard focus to
`songList` when chat closes.

\snippet 05-select/Select.qml arena-select

## Try a complete browsing cycle

Enter a song folder, choose a chart, and play it using your existing gameplay
theme. After closing the result, the list should still show the folder you
left. Use Back or Left to go up a folder, and Leave selection to leave the screen.

Try the same navigation with scratch and bound keys 1 or 3. Open an empty
folder to check its message. On a chart with a saved replay, try Replay and
choose another run from the combo box. If you have a course, select it and
check its stage list before starting play.

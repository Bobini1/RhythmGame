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
shortcuts, background music and chart previews. The theme draws the list and
tells the controller which entry is current.

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
`preview.moving` binding holds a pending preview until the list stops scrolling.
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
| Course                         | `item instanceof course`    | `name`, `chartIds`, `constraints`, `unavailableReason`                                      | Requests course play. The course must be supported and all stages must be installed. Course play is unavailable in Arena. |

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
[Rg.tables.search()](classresource__managers_1_1_tables.html). It searches
downloaded tables and external collections without needing the chart installed.
Matching ignores letter case, and an unknown identifier returns an empty list.

`CourseStages.qml` uses the first match's `entry` to show a missing chart's
title and subtitle. If no title is known, it displays the identifier.

\include{lineno} 05-select/CourseStages.qml

## Keep the current row and controller in sync

The theme sets `songList.currentIndex` and draws the current row's highlight.
The controller stores the corresponding entry in `focusedItem`, which it
uses for actions such as opening a chart. The controller has no reference to
`songList`, so the theme connects the view to the controller.

For example, pressing Down moves the current row through these steps:

1. The list forwards the key event to `selection.handleDownPressed(event)`.
   The controller emits `moveRequested` with a step of `1`.
2. The theme's `onMoveRequested` handler changes `songList.currentIndex`.
   The modulo calculation wraps from the last row to the first.
3. The list's `onCurrentIndexChanged` handler calls `selection.setFocused()`
   with the entry at the new index. The controller now uses that entry for
   its actions.

The controller can also choose an entry when browsing changes the list.
For example, going back from a folder restores the entry for that folder.
The controller updates its `focusedItem` and emits `focusRequested(index)`.
The theme's `onFocusRequested` handler sets `songList.currentIndex` to match.

The controller snippet above handles both movement requests and requests for
a specific row. The list supplies the other half of the connection:

\snippet 05-select/Select.qml select-focus

## Give the list keyboard focus

The controller's `focusedItem`, `setFocused()` and `focusRequested` refer to
the chosen entry. Qt's keyboard focus determines which item receives key
events. The theme manages keyboard focus too.

The example sets `focus: true` on the `Pane` and `ListView` so key events
reach the list when the screen is active. Clicking a row calls
`songList.forceActiveFocus()` to return keyboard input to the list. The
separate assignment to `songList.currentIndex` chooses the clicked row.

The list forwards Up and Down presses and releases to the controller through
its `Keys` handlers. The controller accepts the arrow events, and
`keyNavigationEnabled: false` disables the list's built-in arrow navigation.

Input is disabled for screens that are inactive.
For example, select does not receive any input while gameplay is active above it.
This is tracked by the property [`enabled`](https://doc.qt.io/qt-6/qml-qtquick-item.html#enabled-prop).
It cascades down to all child items when set to false.

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

Clicking a button saves `selectKeymodeFilter` in the active profile. The
controller follows that setting and filters its entries again. Writing to the
profile keeps the controller's binding intact, so the buttons also follow
profile changes. The handler then returns keyboard focus to the song list.

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
In Arena, Leave selection and Escape leave the room and return to the lobby browser.

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

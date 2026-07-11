# Arena Select Panel Interaction Follow-up Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keep the Arena select header pinned to the panel top in both modes, make moving and edge resizing discoverable, remove the redundant Leave button, and make Return and keypad Enter activate the same chart.

**Architecture:** Keep `ArenaSelectPanel` as the single Default/LR2/Beatoraja room surface. Constrain its header through explicit layout policies, top-align non-expanding Details content, expand existing resize handlers along their complete edges, and route both confirm keys through the existing select activation paths.

**Tech Stack:** Qt 6, QML/Qt Quick Controls, Qt Quick Layouts, Qt Quick Test, Catch2, CMake.

## Global Constraints

- The full-width header has a fixed compact height and remains pinned to the panel's top inset.
- Switching Details/Chat must not change the header or tab scene-space Y positions.
- The full header is the ordinary Arena-select move surface and exposes a move cursor plus a subtle grip cue.
- The room title retains usable width; Details and Chat remain compact adjacent tabs.
- Invisible side resize targets span their usable edges, with compact corner targets and correct cursors.
- Details leaves unused height below its summary, never above the header or body.
- The panel contains no Leave button; Escape continues to leave the room through existing Default and LR2/Beatoraja select paths.
- Return and keypad Enter activate the focused chart through the same path.
- Preserve the user's unrelated local files.
- Follow red-green-refactor for every production behavior change.

---

### Task 1: Fixed Top Header and Top-aligned Details

**Files:**
- Modify: `test/qml/tst_ArenaDefaultSelect.qml`
- Modify: `test/qml/tst_ArenaLegacySelect.qml`
- Modify: `RhythmGameQml/Arena/ArenaSelectPanel.qml`
- Modify: `RhythmGameQml/Arena/ArenaSelectionSummary.qml`

**Interfaces:**
- Consumes: `ArenaSelectPanel.detailMode`, `ArenaSelectPanel.dragHandle`, and the existing shared Details/Chat controls.
- Produces: object `arenaSelectHeader`, a fixed-height full-header `dragHandle`, compact adjacent tab widths, a decorative drag cue, and no `arenaSelectLeave` object.

- [ ] **Step 1: Write the failing geometry test**

Add this behavior to `tst_ArenaDefaultSelect.qml` using the existing helpers:

```qml
function test_panel_header_and_body_stay_at_top_across_modes() {
    const session = createSession();
    const panel = createTemporaryObject(panelComponent, testCase, {
        "height": 480, "session": session, "width": 640
    });
    const header = findChild(panel, "arenaSelectHeader");
    const details = findChild(panel, "arenaSelectDetailsTab");
    const chat = findChild(panel, "arenaSelectChatTab");
    const roster = findChild(panel, "arenaSelectRoster");
    const summary = findChild(panel, "arenaSelectSelection");
    verify(header !== null);
    verify(summary !== null);
    const headerY = header.mapToItem(panel, 0, 0).y;
    const detailsY = details.mapToItem(panel, 0, 0).y;
    const chatY = chat.mapToItem(panel, 0, 0).y;
    const rosterY = roster.mapToItem(panel, 0, 0).y;
    const summaryY = summary.mapToItem(panel, 0, 0).y;
    compare(headerY, 10);

    mouseClick(chat, chat.width / 2, chat.height / 2, Qt.LeftButton);
    const chatView = findChild(panel, "arenaSelectChat");
    verify(chatView !== null);
    compare(header.mapToItem(panel, 0, 0).y, headerY);
    compare(details.mapToItem(panel, 0, 0).y, detailsY);
    compare(chat.mapToItem(panel, 0, 0).y, chatY);
    compare(roster.mapToItem(panel, 0, 0).y, rosterY);
    compare(chatView.mapToItem(panel, 0, 0).y, summaryY);
}
```

Update Default and legacy action tests to require `findChild(..., "arenaSelectLeave") === null`. Add assertions that the room title retains positive width and `chat.x` is adjacent to `details.x + details.width` at `520 x 320`.

- [ ] **Step 2: Verify RED**

Run:

```powershell
cmake --build build/dev-rel --config Release --target RhythmGame_arena_qml_test --parallel 2
ctest --test-dir build/dev-rel -C Release -R "^ArenaQml$" --output-on-failure
```

Expected: the new test fails because `arenaSelectHeader` does not exist and the current header expands vertically; Leave-removal assertions fail too.

- [ ] **Step 3: Implement the fixed full-width header**

In `ArenaSelectPanel.qml`, expose the complete header as `dragHandle`, assign `objectName: "arenaSelectHeader"`, and set equal `Layout.minimumHeight`, `Layout.preferredHeight`, and `Layout.maximumHeight` values. Add a `HoverHandler` with `Qt.SizeAllCursor`, a muted decorative grip, a fill-width title with a usable minimum, and a non-expanding `TabBar`. Give each tab `width: implicitWidth`, reduced horizontal padding, and a minimum 32-pixel height. Remove `arenaSelectLeave`; retain the ready-state text and Ready button.

- [ ] **Step 4: Top-align Details content**

Append this unpainted spacer to `ArenaSelectionSummary.qml`:

```qml
Item {
    Layout.fillHeight: true
    Layout.minimumHeight: 0
}
```

- [ ] **Step 5: Verify GREEN**

Run the commands from Step 2. Expected: all Arena QML tests pass with identical header/tab/body origins across modes.

- [ ] **Step 6: Commit Task 1**

```powershell
git commit --only test/qml/tst_ArenaDefaultSelect.qml test/qml/tst_ArenaLegacySelect.qml RhythmGameQml/Arena/ArenaSelectPanel.qml RhythmGameQml/Arena/ArenaSelectionSummary.qml -m "fix: pin Arena select controls to panel top"
```

### Task 2: Full-edge Invisible Resize Targets

**Files:**
- Modify: `test/qml/tst_ArenaOverlayPlacement.qml`
- Modify: `RhythmGameQml/Arena/ArenaOverlayPlacementFrame.qml`
- Modify: `RhythmGameQml/Arena/ArenaOverlayResizeHandle.qml`

**Interfaces:**
- Consumes: existing resize edge directions and `interactionDelta` signals.
- Produces: horizontal targets spanning between corners, vertical targets spanning between corners, and compact centered focus chrome.

- [ ] **Step 1: Write failing edge-target tests**

Extend `test_select_direct_move_and_invisible_resize()`:

```qml
const top = findChild(frame, "arenaResizeTop");
const right = findChild(frame, "arenaResizeRight");
compare(top.height, 16);
verify(top.width >= frame.width - 32);
compare(right.width, 16);
verify(right.height >= frame.height - 32);
```

Drag the right target near `y = 24` and the bottom target near `x = 24`; verify width/height change and each gesture commits once.

- [ ] **Step 2: Verify RED**

Run the Task 1 test commands. Expected: side-length assertions fail because all current side targets are `16 x 16` midpoint squares.

- [ ] **Step 3: Span the side handlers**

In `ArenaOverlayPlacementFrame.qml`, set top/bottom `width: Math.max(16, root.width - 32)` with `x: 16`, and left/right `height: Math.max(16, root.height - 32)` with `y: 16`. Retain current thickness, edge directions, safe clamping, and compact corners. In `ArenaOverlayResizeHandle.qml`, center a compact focus indicator around the chrome rather than filling a side-length handler.

- [ ] **Step 4: Verify GREEN**

Run the Task 1 test commands. Expected: all Arena QML tests pass, including off-center side drags.

- [ ] **Step 5: Commit Task 2**

```powershell
git commit --only test/qml/tst_ArenaOverlayPlacement.qml RhythmGameQml/Arena/ArenaOverlayPlacementFrame.qml RhythmGameQml/Arena/ArenaOverlayResizeHandle.qml -m "fix: resize Arena panel from full edges"
```

### Task 3: Equivalent Confirm Keys and Escape-only Room Exit

**Files:**
- Modify: `test/arena/ArenaOverlayPolicy.test.cpp`
- Modify: `share/RhythmGame/themes/Default/scripts/select/List.qml`
- Modify: `RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml`

**Interfaces:**
- Consumes: Default `List.goForward(current)`, LR2 `selectGoForward()`, and existing seated Escape shortcuts.
- Produces: Default `Keys.onEnterPressed` parity and one LR2 `handleConfirmKey(event)` path used by Return and keypad Enter.

- [ ] **Step 1: Write the failing source-contract test**

Add a focused case to `ArenaOverlayPolicy.test.cpp`:

```cpp
const auto defaultList = qmlSource(
  "share/RhythmGame/themes/Default/scripts/select/List.qml");
requireContains(defaultList,
                { "Keys.onReturnPressed", "Keys.onEnterPressed",
                  "goForward(current)" });
const auto legacy = qmlSource(
  "RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml");
requireContains(legacy,
                { "function handleConfirmKey", "Keys.onReturnPressed",
                  "Keys.onEnterPressed", "root.handleConfirmKey(event)",
                  "root.selectGoForward()" });
const auto panel = qmlSource("RhythmGameQml/Arena/ArenaSelectPanel.qml");
CHECK_FALSE(panel.contains(QStringLiteral("arenaSelectLeave")));
```

- [ ] **Step 2: Verify RED**

Run:

```powershell
cmake --build build/dev-rel --config Release --target RhythmGame_test --parallel 2
build/dev-rel/test/bin/RhythmGame_test.exe "[arena][ArenaOverlayPolicy]"
```

Expected: failure because Default has no keypad-Enter handler and LR2 keypad Enter does not use the Return path.

- [ ] **Step 3: Add Default keypad-Enter parity**

Add beside the Return handler in `List.qml`:

```qml
Keys.onEnterPressed: {
    goForward(current);
}
```

- [ ] **Step 4: Share the LR2 confirm path**

Extract existing Return behavior into `handleConfirmKey(event)`, preserving Readme close, Decide skip, Result close, navigation readiness, event acceptance, and `selectGoForward()`. Route both handlers through it:

```qml
Keys.onReturnPressed: event => root.handleConfirmKey(event)
Keys.onEnterPressed: event => root.handleConfirmKey(event)
```

- [ ] **Step 5: Verify GREEN**

Run:

```powershell
cmake --build build/dev-rel --config Release --target RhythmGame_test RhythmGame_arena_qml_test --parallel 2
build/dev-rel/test/bin/RhythmGame_test.exe "[arena][ArenaOverlayPolicy]"
ctest --test-dir build/dev-rel -C Release -R "^ArenaQml$" --output-on-failure
```

Expected: the focused policy test and Arena QML suite pass.

- [ ] **Step 6: Commit Task 3**

```powershell
git commit --only test/arena/ArenaOverlayPolicy.test.cpp share/RhythmGame/themes/Default/scripts/select/List.qml RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml -m "fix: align Arena chart confirm keys"
```

### Task 4: Final Verification

**Files:**
- Inspect: all files modified by Tasks 1-3
- Test: all configured project targets

**Interfaces:**
- Consumes: completed interaction fixes.
- Produces: a verified branch ready for visual testing.

- [ ] **Step 1: Run the aggregate Release build**

```powershell
cmake --build build/dev-rel --config Release --parallel 2
```

Expected: successful build with no new QML registration or lint failures.

- [ ] **Step 2: Run the complete configured suite**

```powershell
ctest --test-dir build/dev-rel -C Release --output-on-failure
```

Expected: every configured test passes.

- [ ] **Step 3: Check scope and whitespace**

```powershell
git diff --check origin/codex/online-arena...HEAD
git status --short
git diff --stat origin/codex/online-arena...HEAD
```

Expected: no whitespace errors and the user's unrelated local files remain untouched.

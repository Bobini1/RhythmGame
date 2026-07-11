# Arena Select Panel Polish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make Default and LR2/Beatoraja Arena song selection use one bounded, always-visible, directly movable and invisibly resizable room panel without disturbing normal song selection.

**Architecture:** Extend the existing normalized `ArenaOverlayPlacementFrame` and theme-vars persistence with a select placement kind and direct-manipulation switches. Move the neutral room UI into the shared `RhythmGameQml` module, then mount it through a transparent fullscreen placement host in both native Default select and the legacy fallback.

**Tech Stack:** Qt 6, QML/Qt Quick Controls, C++ theme-var seeding, Qt Quick Test, Catch2, CMake.

## Global Constraints

- Preserve the normal `640 x 480` Default StageFile and keep `stageFileFrame.png` visible in Arena.
- Default placement starts from authored `1920 x 1080` coordinates `x = 728`, `y = 120`, `width = 520`, `height = 480` and is clamped to the viewport.
- Both native and legacy select use the same always-expanded Details/Chat panel; there is no `+` or collapsed state.
- Exactly one of Details and Chat remains selected after mouse or Space activation.
- The title region is always draggable; eight invisible edge/corner hit areas are always resizable and expose resize cursors.
- Select manipulation does not use F2 and must not change gameplay/result F2 behavior.
- Placement persists only as `arenaOverlaySelect{X,Y,Width,Height}Normalized` inside the active select theme's existing theme-vars map.
- Do not modify, stage, or commit the user's unrelated `vcpkg.json`, generated vcpkg folders, profiler files, downloaded themes, or other untracked files.
- Follow test-first red-green-refactor for every production behavior change.

---

### Task 1: Select placement persistence and direct manipulation

**Files:**
- Modify: `test/arena/ArenaThemeVars.test.cpp`
- Modify: `test/qml/FakeArenaThemeVars.qml`
- Modify: `test/qml/tst_ArenaOverlayPlacement.qml`
- Modify: `src/resource_managers/Vars.cpp`
- Modify: `RhythmGameQml/Arena/ArenaOverlayPlacementFrame.qml`
- Modify: `RhythmGameQml/Arena/ArenaOverlayResizeHandle.qml`

**Interfaces:**
- Consumes: existing normalized placement APIs in `ArenaOverlayPlacementFrame`.
- Produces: placement kind `selectRoom`, layout variant `select`, prefix `arenaOverlaySelect`, `defaultPixelRectHint`, `directMoveEnabled`, `directResizeEnabled`, `moveHandle`, and independently hidden resize chrome.

- [ ] **Step 1: Write failing C++ tests for select-theme geometry**

Change the existing expectation that select vars are absent into assertions that the active select theme owns four `-1.0` defaults, remains independent from gameplay/result maps, persists a changed normalized rectangle, and reloads it with the profile.

```cpp
auto* select = screenThemeVars(profileA, QStringLiteral("select"));
REQUIRE(select != nullptr);
for (const auto& suffix : { QStringLiteral("XNormalized"),
                            QStringLiteral("YNormalized"),
                            QStringLiteral("WidthNormalized"),
                            QStringLiteral("HeightNormalized") }) {
    CHECK(select->value(QStringLiteral("arenaOverlaySelect") + suffix) == -1.0);
}
```

- [ ] **Step 2: Run the focused C++ test and verify RED**

Run:

```powershell
cmake --build build/dev-rel --config Release --target RhythmGame_test --parallel 2
build/dev-rel/test/bin/RhythmGame_test.exe "[arena][ArenaThemeVars]"
```

Expected: failure because the select theme map or `arenaOverlaySelect*` properties are missing.

- [ ] **Step 3: Write failing QML tests for the new placement contract**

Add select properties to `FakeArenaThemeVars.qml`, construct a frame with `placementKind: "selectRoom"` and `layoutVariant: "select"`, and assert:

```qml
compare(frame.resolvedPixelRect, Qt.rect(728, 120, 520, 480))
verify(frame.directMoveEnabled)
verify(frame.directResizeEnabled)
compare(findChild(frame, "arenaResizeRightChrome").visible, false)
```

Mouse-drag only the supplied title handle and then the right resize hit area. Verify the normalized select properties change and unrelated K7/result properties do not.

- [ ] **Step 4: Run the focused QML test and verify RED**

Run:

```powershell
cmake --build build/dev-rel --config Release --target RhythmGame_arena_qml_test --parallel 2
ctest --test-dir build/dev-rel -C Release -R "^ArenaQml$" --output-on-failure
```

Expected: failure because the select prefix, hint, and direct interaction properties do not exist.

- [ ] **Step 5: Implement select theme-var seeding**

Teach `ensureArenaOverlayThemeVars` to accept the non-aliased `select` screen and call:

```cpp
addPlacement(screenVars, QStringLiteral("Select"));
```

Keep all existing K5/K7/K10/K14/result behavior intact.

- [ ] **Step 6: Implement select placement and direct interaction switches**

Add the select prefix and validation:

```qml
case "select": return "arenaOverlaySelect"
```

Permit only `placementKind === "selectRoom" && layoutVariant === "select"`. Prefer a valid `defaultPixelRectHint` before the existing gameplay/result default. Enable the move handler when `customizeMode || directMoveEnabled`, parent it to `moveHandle || root`, and enable resize hit areas when `customizeMode || directResizeEnabled`.

Split resize hit-area interaction from its chrome:

```qml
required property bool interactionEnabled
required property bool chromeVisible
visible: interactionEnabled
enabled: interactionEnabled
activeFocusOnTab: chromeVisible && enabled
```

Keep the hover resize cursor active while hiding both grip rectangles when `chromeVisible` is false.

- [ ] **Step 7: Run focused tests and verify GREEN**

Run both commands from Steps 2 and 4. Expected: both focused suites pass with no QML warnings.

- [ ] **Step 8: Commit only Task 1 files**

```powershell
git commit --only test/arena/ArenaThemeVars.test.cpp test/qml/FakeArenaThemeVars.qml test/qml/tst_ArenaOverlayPlacement.qml src/resource_managers/Vars.cpp RhythmGameQml/Arena/ArenaOverlayPlacementFrame.qml RhythmGameQml/Arena/ArenaOverlayResizeHandle.qml -m "feat: support direct Arena select placement"
```

### Task 2: Shared, exclusive, always-expanded select panel

**Files:**
- Create: `RhythmGameQml/Arena/ArenaSelectPanel.qml`
- Create: `RhythmGameQml/Arena/ArenaSelectOverlay.qml`
- Modify: `RhythmGameQml/Arena/ArenaLegacySelectOverlay.qml`
- Modify: `RhythmGameQml/CMakeLists.txt`
- Delete: `share/RhythmGame/themes/Default/scripts/select/ArenaSelectPanel.qml`
- Modify: `RhythmGameQml/ContentFrame.qml`
- Modify: `test/qml/tst_ArenaDefaultSelect.qml`
- Modify: `test/qml/tst_ArenaLegacySelect.qml`

**Interfaces:**
- Consumes: Task 1's `selectRoom` placement frame and direct manipulation properties.
- Produces: common object names `arenaSelectDetailsTab`, `arenaSelectChatTab`, `arenaSelectRoster`, `arenaSelectSelection`, `arenaSelectChat`, `arenaSelectReady`, `arenaSelectLeave`; `ArenaSelectPanel.dragHandle`; and fullscreen `ArenaSelectOverlay` with bounded `placementFrame` and `panel` aliases.

- [ ] **Step 1: Write failing tab and shared-UX tests**

For both Default and legacy harnesses, assert the shared controls exist immediately, no expand control exists, and run this sequence by mouse and Space:

```qml
compare(details.checked, true)
compare(chat.checked, false)
mouseClick(details, details.width / 2, details.height / 2)
compare(details.checked, true)
compare(chat.checked, false)
mouseClick(chat, chat.width / 2, chat.height / 2)
compare(details.checked, false)
compare(chat.checked, true)
chat.forceActiveFocus()
keyClick(Qt.Key_Space)
compare(chat.checked, true)
```

Retain action-routing assertions for ready, kick, chat send, and leave.

- [ ] **Step 2: Write a failing real-Loader geometry regression**

Mount `ArenaLegacySelectOverlay` as the direct source item of a fullscreen `Loader` at `1280 x 720`. Assert its fullscreen root is transparent while `arenaSelectPlacementFrame` remains smaller than the viewport, has safe margins, and contains the shared panel without an `arenaLegacyExpand` object.

- [ ] **Step 3: Run Default and legacy QML tests and verify RED**

Run:

```powershell
cmake --build build/dev-rel --config Release --target RhythmGame_arena_qml_test --parallel 2
ctest --test-dir build/dev-rel -C Release -R "^ArenaQml$" --output-on-failure
```

Expected: failures for repeat-click exclusivity, the legacy `+`, divergent object names, and fullscreen Loader geometry.

- [ ] **Step 4: Move the room panel into the shared module**

Move the current neutral panel implementation to `RhythmGameQml/Arena/ArenaSelectPanel.qml`. Replace independent checkable buttons and `detailMode` assignments with one `TabBar` containing two `TabButton`s; expose:

```qml
readonly property string detailMode: tabs.currentIndex === 1 ? "chat" : "details"
readonly property alias dragHandle: titleDragHandle
```

Make `titleDragHandle` the flexible room-title region only, leaving both tabs outside that pointer-handler area. Keep the roster present in both modes and choose summary/chat from the authoritative tab index.

- [ ] **Step 5: Add the transparent shared placement host**

Create fullscreen `ArenaSelectOverlay.qml` with a single inner `ArenaOverlayPlacementFrame`:

```qml
placementKind: "selectRoom"
layoutVariant: "select"
customizeMode: false
directMoveEnabled: true
directResizeEnabled: true
resizeChromeVisible: false
minimumPixelSize: Qt.size(520, 320)
moveHandle: panel.dragHandle
```

Expose read-only aliases for tests. The only dark rectangle remains inside `ArenaSelectPanel`.

- [ ] **Step 6: Replace the legacy collapsed overlay**

Make `ArenaLegacySelectOverlay` a thin fullscreen wrapper around `ArenaSelectOverlay`, retain native-presentation suppression, and add required `themeVars`. In `ContentFrame.qml`, pass:

```qml
themeVars: globalRoot.resolvedThemeVars("select")
```

Do not add F2 handling.

- [ ] **Step 7: Register the shared components and remove the theme-local duplicate**

Add both new QML files to `RhythmGameQml/CMakeLists.txt` and delete the Default-local `ArenaSelectPanel.qml`, allowing the existing `import RhythmGameQml` to resolve the shared type.

- [ ] **Step 8: Run focused tests and verify GREEN**

Run the command from Step 3. Expected: both suites pass, including mouse and keyboard tab reactivation and fullscreen Loader bounds.

- [ ] **Step 9: Commit only Task 2 files**

```powershell
git commit --only RhythmGameQml/Arena/ArenaSelectPanel.qml RhythmGameQml/Arena/ArenaSelectOverlay.qml RhythmGameQml/Arena/ArenaLegacySelectOverlay.qml RhythmGameQml/CMakeLists.txt share/RhythmGame/themes/Default/scripts/select/ArenaSelectPanel.qml RhythmGameQml/ContentFrame.qml test/qml/tst_ArenaDefaultSelect.qml test/qml/tst_ArenaLegacySelect.qml -m "fix: unify Arena select room panel"
```

### Task 3: Restore Default composition and gap placement

**Files:**
- Modify: `share/RhythmGame/themes/Default/scripts/select/Select.qml`
- Modify: `test/qml/tst_ArenaDefaultSelect.qml`
- Modify: `test/arena/ArenaOverlayPolicy.test.cpp`

**Interfaces:**
- Consumes: Task 2's fullscreen `ArenaSelectOverlay` and Task 1's `defaultPixelRectHint`.
- Produces: full StageFile, restored wheel backing, and scaled gap-based Default placement hint.

- [ ] **Step 1: Write failing Default composition tests**

Update the source-policy regression to require an always-`480` StageFile, no Arena `PreserveAspectCrop`, no Arena visibility condition on `stageFileFrame`, a fullscreen native panel Loader, and a default hint derived from:

```qml
Qt.rect(root.contentLeft + 728 * root.contentScale,
        root.contentTop + 120 * root.contentScale,
        520 * root.contentScale,
        480 * root.contentScale)
```

In the QML viewport harness, assert the unstored `1920 x 1080` panel starts at `Qt.rect(728, 120, 520, 480)`, scales at `2560 x 1440`, and remains clamped at `1024 x 768`.

- [ ] **Step 2: Run focused tests and verify RED**

Run:

```powershell
cmake --build build/dev-rel --config Release --target RhythmGame_test RhythmGame_arena_qml_test --parallel 2
build/dev-rel/test/bin/RhythmGame_test.exe "[arena][ArenaOverlayPolicy]"
ctest --test-dir build/dev-rel -C Release -R "^ArenaQml$" --output-on-failure
```

Expected: failure because the current source shrinks/crops StageFile, hides the composite, and hard-mounts the panel under the cover.

- [ ] **Step 3: Restore Default select composition**

In `Select.qml`, restore:

```qml
fillMode: Image.Stretch
height: 480
width: 640
```

Remove the `stageFileFrame` Arena visibility condition. Replace the fixed small panel Loader geometry with a fullscreen scene-space Loader whose `ArenaSelectOverlay` receives `root.themeVars`, `root`, the session, and the scaled gap rectangle as `defaultPixelRectHint`.

- [ ] **Step 4: Run focused tests and verify GREEN**

Run all commands from Step 2. Expected: focused C++ and QML tests pass without warnings.

- [ ] **Step 5: Run the full Arena verification set**

Run:

```powershell
build/dev-rel/test/bin/RhythmGame_test.exe "[arena]"
ctest --test-dir build/dev-rel -C Release -R "^ArenaQml$" --output-on-failure
```

Expected: all Arena C++ and QML tests pass.

- [ ] **Step 6: Review translations and QML registration**

Confirm all remaining user-visible strings are existing `ArenaSelectPanel` strings wrapped in `qsTr()`, the moved component keeps the `ArenaSelectPanel` translation context, and CMake packages both new shared QML files. No new copy is introduced.

- [ ] **Step 7: Commit only Task 3 files**

```powershell
git commit --only share/RhythmGame/themes/Default/scripts/select/Select.qml test/qml/tst_ArenaDefaultSelect.qml test/arena/ArenaOverlayPolicy.test.cpp -m "fix: preserve Default select layout in Arena"
```

### Task 4: Final branch verification and review

**Files:**
- Inspect: all Task 1-3 files
- Test: all configured test targets

**Interfaces:**
- Consumes: completed Task 1-3 commits.
- Produces: verified branch ready for user testing.

- [ ] **Step 1: Build all test targets**

```powershell
cmake --build build/dev-rel --config Release --target RhythmGame_test RhythmGame_arena_qml_test --parallel 2
```

- [ ] **Step 2: Run the complete configured test suite**

```powershell
ctest --test-dir build/dev-rel -C Release --output-on-failure
```

Expected: every configured test passes.

- [ ] **Step 3: Run QML static checks used by the repository**

Build the normal aggregate target so generated QML type registration and lint/build errors surface:

```powershell
cmake --build build/dev-rel --config Release --parallel 2
```

Expected: successful build with no new QML warnings from the changed components.

- [ ] **Step 4: Inspect scope before review**

```powershell
git status --short
git diff origin/codex/online-arena...HEAD --stat
```

Confirm the user's unrelated staged and untracked files remain uncommitted.

- [ ] **Step 5: Request whole-branch review**

Use the requesting-code-review workflow against the implementation range, fix all Critical/Important findings test-first, and rerun the covering tests before re-review.

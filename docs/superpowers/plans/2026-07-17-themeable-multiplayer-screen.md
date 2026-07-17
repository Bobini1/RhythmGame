# Themeable Multiplayer Screen Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the multiplayer room browser an independently selectable theme screen supplied by the Default theme.

**Architecture:** `ContentFrame` keeps the Arena shell, session lifetime, and transitions to select, but loads the configured `multiplayer` script from the theme system. The Default theme owns the room-browser presentation and its browser-only helper components; reusable Arena models and controls remain in the `RhythmGameQml` module.

**Tech Stack:** Qt 6, QML, CMake theme scanning, Qt Linguist translations.

## Global Constraints

- The theme screen key is `multiplayer`; its visible title remains `Online Arena`.
- `ArenaSession`, protocol behavior, room behavior, overlays, and backend type names do not change.
- Existing profiles default to the Default multiplayer screen through `fillWithDefaults`.
- Third-party themes are optional and appear as choices only when they provide `multiplayer`.
- Do not add QML/UI tests or tests that parse source code as strings.
- Preserve all unrelated worktree changes.

---

### Task 1: Register the multiplayer theme screen

**Files:**
- Modify: `share/RhythmGame/themes/Default/theme.json`
- Modify: `share/RhythmGame/themes/Default/scripts/settings/ThemeSettings.qml`

**Interfaces:**
- Consumes: generic `scripts` entries parsed by `scanThemes` and defaulted by `fillWithDefaults`.
- Produces: `themeConfig.multiplayer` and `family.screens.multiplayer.script` for `ContentFrame`.

- [ ] **Step 1: Add the Default screen manifest entry**

Add the following entry alongside `main`, `select`, and `settings`:

```json
"multiplayer": "scripts/multiplayer/Multiplayer.qml"
```

- [ ] **Step 2: Expose Multiplayer in Theme Settings**

Insert `multiplayer` after `main` in `orderedScreens`, and add its display name:

```qml
let order = ["k7", "k7battle", "k5", "k5battle", "k10", "k14", "main", "multiplayer", "settings", "select", "decide", "result", "courseResult"];
```

```qml
"multiplayer": QT_TR_NOOP("Multiplayer"),
```

- [ ] **Step 3: Verify the generic theme machinery recognizes the screen**

Run:

```powershell
cmake --build build/dev-relwithdebinfo --target RhythmGame_test --config RelWithDebInfo --parallel 2
./build/dev-relwithdebinfo/test/bin/RhythmGame_test.exe "[themes]"
```

Expected: the existing theme scanner/configuration tests pass; no multiplayer-specific UI test is added.

- [ ] **Step 4: Commit the registration**

```powershell
git add -- share/RhythmGame/themes/Default/theme.json share/RhythmGame/themes/Default/scripts/settings/ThemeSettings.qml
git commit -m "feat: register multiplayer theme screen"
```

### Task 2: Move the Default browser presentation into the theme

**Files:**
- Move: `RhythmGameQml/Arena/ArenaBrowser.qml` to `share/RhythmGame/themes/Default/scripts/multiplayer/Multiplayer.qml`
- Move: `RhythmGameQml/Arena/ArenaLoginPanel.qml` to `share/RhythmGame/themes/Default/scripts/multiplayer/ArenaLoginPanel.qml`
- Move: `RhythmGameQml/Arena/ArenaRoomMemberStack.qml` to `share/RhythmGame/themes/Default/scripts/multiplayer/ArenaRoomMemberStack.qml`
- Modify: `RhythmGameQml/CMakeLists.txt`

**Interfaces:**
- Consumes: `ArenaSession`, `Profile`, `ArenaAvatar`, and `ArenaStatusAnnouncer` from `RhythmGameQml`.
- Produces: a theme component with required `session` and `activeProfile` properties plus `createRequested`, `joinRequested`, `retryRequested`, and `exitRequested` signals.

- [ ] **Step 1: Move the browser and browser-only helpers**

Move the three files without changing behavior. Rename only the root browser file to `Multiplayer.qml`.

- [ ] **Step 2: Preserve translations and resolve shared QML types**

Place this before the imports in `Multiplayer.qml` so existing `ArenaBrowser` translations retain their context:

```qml
pragma Translator: "ArenaBrowser"
```

Add the module import to the moved member stack because `ArenaAvatar` remains shared:

```qml
import RhythmGameQml
```

Keep `ArenaStatusAnnouncer` in `RhythmGameQml`, because select and room overlays also use it.

- [ ] **Step 3: Remove moved files from the core QML module**

Remove these entries from `RHYTHM_GAME_QML_FILES`:

```cmake
Arena/ArenaBrowser.qml
Arena/ArenaLoginPanel.qml
Arena/ArenaRoomMemberStack.qml
```

- [ ] **Step 4: Check the moved files**

Run:

```powershell
git diff --check -- RhythmGameQml/CMakeLists.txt share/RhythmGame/themes/Default/scripts/multiplayer
```

Expected: no whitespace errors.

- [ ] **Step 5: Commit the presentation move**

```powershell
git add -- RhythmGameQml/CMakeLists.txt RhythmGameQml/Arena/ArenaBrowser.qml RhythmGameQml/Arena/ArenaLoginPanel.qml RhythmGameQml/Arena/ArenaRoomMemberStack.qml share/RhythmGame/themes/Default/scripts/multiplayer
git commit -m "refactor: move multiplayer browser into default theme"
```

### Task 3: Load the selected multiplayer screen from the Arena shell

**Files:**
- Modify: `RhythmGameQml/ContentFrame.qml`

**Interfaces:**
- Consumes: `globalRoot.configuredScreen("multiplayer").script` and the Task 2 screen contract.
- Produces: the same create/join/retry/exit behavior and browser/select transitions as before.

- [ ] **Step 1: Resolve the configured screen**

Add a component URL derived from the selected screen:

```qml
readonly property var multiplayerScreen: configuredScreen("multiplayer")
```

- [ ] **Step 2: Replace the hard-coded `ArenaBrowser` component**

Remove `arenaBrowserComponent`. Give `arenaBrowserLoader` a `configuredSource` property and load it with initial required properties:

```qml
readonly property url configuredSource: globalRoot.multiplayerScreen
    ? globalRoot.multiplayerScreen.script
    : ""

function loadConfiguredScreen(): void {
    if (configuredSource.toString().length === 0) {
        arenaShell.requestCloseArena();
        return;
    }
    setSource(configuredSource, {
        "session": arenaShell.session,
        "activeProfile": globalRoot.mainProfile
    });
}

Component.onCompleted: loadConfiguredScreen()
onConfiguredSourceChanged: loadConfiguredScreen()
```

Keep the Loader active while select is shown so returning to the browser preserves its state.

- [ ] **Step 3: Wire the themed screen through explicit `Connections`**

Connect the loaded screen's four signals without relying on dynamic scope:

```qml
Connections {
    target: arenaBrowserLoader.status === Loader.Ready
        ? arenaBrowserLoader.item
        : null

    function onCreateRequested(name, password): void {
        arenaShell.session.createRoom(name, password);
    }
    function onExitRequested(): void {
        arenaShell.requestCloseArena();
    }
    function onJoinRequested(roomId, password): void {
        arenaShell.session.joinRoom(roomId, password);
    }
    function onRetryRequested(): void {
        arenaShell.session.retry();
    }
}
```

If loading fails, log the component error and call `requestCloseArena()` so no invisible active session remains.

- [ ] **Step 4: Build the QML resources**

Run:

```powershell
cmake --build build/dev-relwithdebinfo --target RhythmGame_exe --config RelWithDebInfo --parallel 2
```

Expected: `RhythmGame_exe` and the Default theme QML compile successfully.

- [ ] **Step 5: Commit the shell integration**

```powershell
git add -- RhythmGameQml/ContentFrame.qml
git commit -m "feat: load multiplayer browser from selected theme"
```

### Task 4: Update translations and perform the multiplayer smoke check

**Files:**
- Modify: `share/RhythmGame/themes/Default/translations/Default_en.ts`
- Modify: `share/RhythmGame/themes/Default/translations/Default_pl.ts`
- Modify: `share/RhythmGame/themes/Default/translations/Default_jp.ts`

**Interfaces:**
- Consumes: moved Default QML sources and the preserved `ArenaBrowser` translation context.
- Produces: current source locations plus the translated `Multiplayer` Theme Settings label.

- [ ] **Step 1: Regenerate translation sources**

Run:

```powershell
cmake --build build/dev-relwithdebinfo --target Default_translations --config RelWithDebInfo
```

Expected: browser entries point to `scripts/multiplayer`; existing Polish translations remain translated, and `Multiplayer` is the only new copy requiring translation.

- [ ] **Step 2: Fill the Polish screen label and compile catalogs**

Translate the Theme Settings label as `Tryb wieloosobowy`, then run:

```powershell
cmake --build build/dev-relwithdebinfo --target release_translations --config RelWithDebInfo
```

Expected: all configured `.qm` files compile without a missing target-language warning.

- [ ] **Step 3: Run focused static verification**

Run:

```powershell
git diff --check
cmake --build build/dev-relwithdebinfo --target RhythmGame_exe --config RelWithDebInfo --parallel 2
```

Expected: no new whitespace errors and a successful executable build.

- [ ] **Step 4: Smoke-test the screen lifecycle**

Run `build/dev-relwithdebinfo/bin/RhythmGame.exe` and verify:

1. Theme Settings lists Multiplayer and defaults it to Default.
2. Online Arena opens with the unchanged browser layout.
3. Login and create/join dialogs still work.
4. Entering a room opens select; leaving returns to the same browser.
5. Exiting Arena closes the session and returns to main.

- [ ] **Step 5: Commit translations and final adjustments**

```powershell
git add -- share/RhythmGame/themes/Default/translations/Default_en.ts share/RhythmGame/themes/Default/translations/Default_pl.ts share/RhythmGame/themes/Default/translations/Default_jp.ts
git commit -m "i18n: update multiplayer screen translations"
```

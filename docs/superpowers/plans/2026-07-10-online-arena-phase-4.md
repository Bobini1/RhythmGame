# Online Arena Phase 4 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship movable forced Arena overlays, native Default and complete LR2/Beatoraja presentation, accessible chat/result polish, and a production-hardened single-replica Coolify service.

**Architecture:** Keep `ArenaSession` and the Phase 3 value models authoritative, seed hidden normalized geometry in the existing profile theme-vars maps, and wrap shared application-owned overlay content in a normalized placement frame. Default opts into native select/result panels while screens without those capabilities receive legacy overlays. The Bun wire protocol remains 1.2; Phase 4 adds only HTTP/gateway operations, bounded metrics, proxy-aware admission, and release verification.

**Tech Stack:** Qt 6.12/C++23/QML/Qt Quick Test/Catch2/CMake/CTest, Bun 1.3.14/TypeScript 5.9/Zod 4/Bun test, JSON profile settings, Prometheus text exposition, Docker/Coolify/Traefik WSS.

## Global Constraints

- The controlling specification is `docs/superpowers/specs/2026-07-10-online-arena-phase-4-design.md`; the umbrella and Phase 1-3 specifications retain authority over earlier behavior.
- Protocol major/minor remain `1.2`; canonical capabilities remain `rooms-v1`, `rounds-v1`, `competition-v1`; Phase 1-3 fixture bytes must not drift.
- `placementKind` is exactly `gameplayLeaderboard` or `resultStandings`; `layoutVariant` is `k5`, `k7`, `k10`, `k14`, or `result`.
- Placement is normalized and persisted through existing per-profile,
  per-screen, per-theme-family vars files. Hidden K5/K7/K10/K14/result fields
  preserve layout-specific geometry without adding an Arena settings file.
- The compact Arena gameplay leaderboard is forced on. No file, UI control, or QML property may persist or expose hidden visibility.
- F2 owns one Arena customization mode; Default participates in existing gameplay customization; LR2/Beatoraja customize only the application overlay.
- Arena customization suppresses chart input but never pauses chart time, telemetry, sound, BGA, or networking.
- Chat is closed on gameplay entry, opens deliberately, never pauses, and closes before customization or abort.
- Default receives native select/result surfaces; a screen without explicit native capability receives application-owned fallback UI.
- No transient Arena state is added to `ChartData`, `GeneralVars`,
  `OnlineRankingModel`, saved play options, or room persistence. The only
  profile vars are hidden theme geometry and the profile-wide F2 hint version.
- All remote strings use `Text.PlainText`; all Phase 4 actions are keyboard/focus accessible and status is never color-only.
- Supported Arena copy is English source plus complete Polish; no new advertised locale is added.
- Official deployment remains one database-free, in-memory replica. Restart/deploy destroys rooms and must not receive IR DB/auth/score secrets.
- Metrics are disabled by default, token-protected when enabled, low-cardinality, and contain no player/session/address/chart/score data.
- Forwarded addresses are trusted only from configured proxy CIDRs; raw addresses and forwarding headers are never logged.
- Existing unrelated dirty files are preserved. Commit independently in `T:/RG/.worktrees/online-arena` and `T:/RhythmGame-IR/.worktrees/online-arena`, staging only task-owned paths.
- Actual Netcup/Coolify/DNS mutation requires external operator credentials; without them, complete artifacts and record the exact environmental blocker rather than weakening policy or claiming deployment.
- Use TDD, run the focused gate before each commit, and perform spec and code-quality review after every task.

---

## File map

### RhythmGame client

- Existing `src/resource_managers/Vars.cpp`: hidden Arena geometry fields in
  each relevant profile theme-vars map.
- Existing `src/arena/ArenaSession.*`: presentation customization gate for the current Arena runner.
- Existing `src/gameplay_logic/ChartRunner.*`: generic pressed-lane tracking and input suppression.
- `RhythmGameQml/Arena/ArenaOverlayPlacementFrame.qml`: normalized geometry, handles, nudge, reset, clamp.
- `RhythmGameQml/Arena/ArenaOverlayResizeHandle.qml`: one application-owned resize-handle primitive.
- Existing `ArenaOverlayHost.qml`, `ArenaGameplayOverlay.qml`, `ArenaGameplayChat.qml`, `ArenaResultOverlay.qml`: F2 host and polished movable surfaces.
- `ArenaRosterView.qml`, `ArenaChatView.qml`, `ArenaSelectionSummary.qml`: shared model-only content.
- `ArenaLegacySelectOverlay.qml`: LR2/Beatoraja/unknown-theme select fallback.
- `share/RhythmGame/themes/Default/scripts/select/ArenaSelectPanel.qml`: native Default room panel.
- `share/RhythmGame/themes/Default/scripts/result/ArenaResultPanel.qml`: native Default winner/standings panel.
- Existing Default Select/Gameplay/Result and ContentFrame: capability/wiring integration.
- `test/qml/*`: Qt Quick Test component/focus/layout coverage.
- `test/arena/*`: theme-vars persistence, runner/session gate, and policy contracts.

### RhythmGame-IR / Arena server

- `arena-server/src/transport/client-address.ts`: canonical proxy-chain resolution.
- `arena-server/src/transport/connection-admission.ts`: per-address upgrade/concurrency/hello bounds.
- `arena-server/src/observability/operational-metrics.ts`: fixed counters/gauges/histograms and Prometheus rendering.
- Existing config/application/gateway/domain files: narrow instrumentation and shutdown ordering.
- `arena-server/ops/coolify.md` and `production.env.example`: exact single-replica deployment contract.
- `arena-server/scripts/phase4-production-smoke.ts` and `phase4-load.ts`: credential-free public checks and bounded soak.
- Unit/integration tests plus verification reports: release evidence.

---

### Task 1: Seed normalized placement in existing profile theme vars

**Files:**
- Modify: `T:/RG/.worktrees/online-arena/src/resource_managers/Vars.h`
- Modify: `T:/RG/.worktrees/online-arena/src/resource_managers/Vars.cpp`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaThemeVars.test.cpp`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: existing `Vars::loadedThemeVars`, theme families/screens, and
  existing `<theme-family>-vars.json` persistence.
- Produces hidden `arenaOverlay{K5,K7,K10,K14,Result}{X,Y,Width,Height}Normalized`
  properties on relevant nested `QQmlPropertyMap`s. `-1` means unset.
- Produces profile-wide `GeneralVars::arenaOverlayHintVersion` for the F2 hint.

- [ ] **Step 1: Write failing theme-vars tests**

Create Catch2 coverage for K5/K7 aliasing with distinct keys, result fields,
unsupported screens, profile A/B isolation, existing-value retention, normal
theme-vars file persistence, and no sentinel room/member/chat/token values.

- [ ] **Step 2: Run RED**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test -j 2
ctest --preset dev-rel -R ArenaThemeVars --output-on-failure
```

Expected: the hidden fields are absent from otherwise-empty theme maps.

- [ ] **Step 3: Seed application-owned hidden fields before map freeze**

After reading existing values and before `writeThemeVars`/
`populateThemePropertyMap`, insert only missing keys. Seed K5 and K7 separately
inside a shared aliased map, likewise K10/K14, and seed Result only on result
screens. Do not expose the fields in imported skin settings UI and do not add
an Arena-specific QObject/file.

- [ ] **Step 4: Verify existing persistence/profile behavior**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test RhythmGame_exe -j 2
ctest --preset dev-rel -R ArenaThemeVars --output-on-failure
git -C T:/RG/.worktrees/online-arena add src/resource_managers/Vars.* test/arena/ArenaThemeVars.test.cpp test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: persist Arena placement in theme vars"
```

---

### Task 2: Build the application-owned placement frame and Qt Quick Test gate

**Required sub-skill:** Read and apply `qt-qml` before editing QML.

**Files:**
- Create: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaOverlayPlacementFrame.qml`
- Create: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaOverlayResizeHandle.qml`
- Create: `T:/RG/.worktrees/online-arena/test/qml/ArenaQmlTestMain.cpp`
- Create: `T:/RG/.worktrees/online-arena/test/qml/tst_ArenaOverlayPlacement.qml`
- Create: `T:/RG/.worktrees/online-arena/test/qml/FakeArenaThemeVars.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/CMakeLists.txt`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: Task 1's resolved theme-vars map and canonical placement/layout
  strings.
- Produces:

```qml
Item {
    id: root
    required property var themeVars
    required property Item viewport
    required property string placementKind
    required property string layoutVariant
    required property bool customizeMode
    readonly property bool forcedVisible: true
    property size minimumPixelSize: Qt.size(280, 160)
    default property alias contentData: contentHost.data
    readonly property bool interactionActive: moveHandler.active
        || resizeInteractionCount > 0

    signal requestExitCustomization()
    signal interactionStateChanged(bool active)
    signal placementCommitted()
}
```

- Produces: one CTest-discovered `RhythmGame_arena_qml_test` linked to
  `Qt6::QuickTest` and the built RhythmGame QML modules.

- [ ] **Step 1: Add failing geometry and interaction QML tests**

Test exact default dimensions, stored conversion, 24 px safe clamp, 280x160
minimum, safe shrink on a smaller viewport, no persistence on passive clamp,
one commit at drag/resize end, all eight handles, Arrow/Shift movement,
Alt/Alt+Shift resize, R reset, Enter/Escape request, profile/theme-map
replacement, and the forced-visible property remaining read-only while active.

Use these viewports and assert every resolved edge remains inside the safe
rectangle: `1024x768`, `1280x720`, `1920x1080`, `2560x1080`, and `3840x2160`.

- [ ] **Step 2: Add the Quick Test runner and confirm RED**

```cpp
#include <QtQuickTest/quicktest.h>
QUICK_TEST_MAIN(arena_qml)
```

Configure the test's import paths to the build-tree `RhythmGameQml` module and
run:

```powershell
cmake --preset dev-rel
cmake --build --preset dev-rel --target RhythmGame_arena_qml_test -j 2
ctest --preset dev-rel -R ArenaQml --output-on-failure
```

Expected: component types are not found.

- [ ] **Step 3: Implement pure geometry helpers and defaults**

Keep placement math in side-effect-free QML functions:

```qml
function defaultPixelRect() {
    const safeW = Math.max(1, viewport.width - 48);
    const safeH = Math.max(1, viewport.height - 48);
    const result = placementKind === "resultStandings";
    const w = Math.min(safeW, Math.max(result ? 360 : 320,
        Math.min(result ? 560 : 420, viewport.width * (result ? 0.40 : 0.30))));
    const h = Math.min(safeH, Math.max(result ? 260 : 240,
        viewport.height * (result ? 0.60 : 0.44)));
    return Qt.rect(Math.max(0, viewport.width - 24 - w),
                   Math.min(24, Math.max(0, viewport.height - h)), w, h);
}
```

Convert normalized records to pixels, clamp into the safe rectangle, and keep
the original normalized record separate from the transient resolved rectangle.

- [ ] **Step 4: Implement move, resize, keyboard, and commit boundaries**

Use `DragHandler` for movement and eight small
`ArenaOverlayResizeHandle` instances with cursor shapes. Update pixels during
the interaction and assign the four layout-specific hidden theme vars only
when active changes to false. Keyboard operations commit after the accepted
key event. Reset assigns `-1` to the exact layout's four fields and recomputes
the default. Make every handle/control at least 32 logical px and assign
`Accessible.name`.

- [ ] **Step 5: Enforce forced visibility and input transparency**

Bind frame visibility to its active host, not to stored/user state. Do not
serialize visibility. Outside customize mode, pointer handling exists only on
real overlay controls/content; decorative frame and handles are disabled.

- [ ] **Step 6: Run GREEN, lint, and commit**

```powershell
cmake --build --preset dev-rel --target RhythmGame_arena_qml_test RhythmGame_qml_qmllint -j 2
ctest --preset dev-rel -R ArenaQml --output-on-failure
git -C T:/RG/.worktrees/online-arena add RhythmGameQml/Arena/ArenaOverlayPlacementFrame.qml RhythmGameQml/Arena/ArenaOverlayResizeHandle.qml RhythmGameQml/CMakeLists.txt test/qml test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: customize Arena overlay geometry"
```

---

### Task 3: Coordinate F2 with Default and suppress Arena runner input safely

**Required sub-skill:** Read and apply `qt-qml` before editing QML.

**Files:**
- Modify: `T:/RG/.worktrees/online-arena/src/gameplay_logic/ChartRunner.h`
- Modify: `T:/RG/.worktrees/online-arena/src/gameplay_logic/ChartRunner.cpp`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaSession.h`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaSession.cpp`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaOverlayHost.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/ContentFrame.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaOverlayCustomization.test.cpp`
- Create: `T:/RG/.worktrees/online-arena/test/qml/tst_ArenaCustomizationHost.qml`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: Phase 3 host/current `arenaRunner` and Task 2 frame.
- Produces:

```cpp
// ChartRunner, generic and not Arena-aware
[[nodiscard]] auto inputSuppressed() const -> bool;
void setInputSuppressed(bool suppressed);

// ArenaSession presentation policy
Q_PROPERTY(bool overlayCustomizationActive
           READ overlayCustomizationActive
           NOTIFY overlayCustomizationActiveChanged FINAL)
Q_INVOKABLE void setOverlayCustomizationActive(bool active);
```

- Produces: optional Default-screen capability
  `setArenaCustomizeMode(active)`; legacy wrappers deliberately omit it.

- [ ] **Step 1: Write failing runner/session gate tests**

Cover a held lane released exactly once on suppression, subsequent press/release
ignored, timer/status/elapsed continuing, idempotent restore, input accepted
after restore, no-op without current Arena runner, replacement/destruction,
round finalization, leave/profile change/exit cleanup, and unchanged ordinary
non-Arena runner behavior.

- [ ] **Step 2: Write failing QML shortcut/priority tests**

Test one F2 activation, Default local shortcut disabled in Arena, Default
`customizeMode` synchronized, legacy pointer shield active only in mode, F2
exit, Enter/Escape exit, chat closed before mode entry, and screen destruction
calling `setOverlayCustomizationActive(false)`.

- [ ] **Step 3: Run RED**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test RhythmGame_arena_qml_test -j 2
ctest --preset dev-rel -R "ArenaOverlayCustomization|ArenaQml" --output-on-failure
```

- [ ] **Step 4: Implement pressed-lane tracking and suppression**

Track mapped physical lane state inside `ChartRunner::passKey`. On the false-to-
true transition, send releases only for currently pressed playable keys using
`steady_clock::now()` converted to the same millisecond epoch expected by
`passKey`, clear the tracked state, then reject chart events until restored.
Start/Select remain excluded. `finish()` and destruction clear state. Do not
stop `propertyUpdateTimer` or change runner status.

- [ ] **Step 5: Implement Session's idempotent policy gate**

Only the current `arenaRunner` receives suppression. If the runner changes
while the property is true, restore the old runner before suppressing the new
one. Every Phase 3 cleanup path calls the same `false` transition before
discarding the runner.

- [ ] **Step 6: Make ArenaOverlayHost the single Arena F2 owner**

The host owns a Window/Application shortcut enabled only for active Arena
gameplay or result presentation. Its transition order is:

```qml
function setCustomizeMode(active) {
    if (active && session.gameplayChatOpen)
        session.setGameplayChatOpen(false);
    session.setOverlayCustomizationActive(active && session.arenaGameplayActive);
    customizeMode = active;
    const screen = sceneStack.currentItem;
    if (screen && typeof screen.setArenaCustomizeMode === "function")
        screen.setArenaCustomizeMode(active);
}
```

Default's own F2 remains enabled only when the current runner is not the Arena
runner. For a screen without the capability, show a full-screen transparent
pointer shield below the frame while customizing. Escape leaves customization
before Phase 3 abort handling.

- [ ] **Step 7: Verify and commit**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test RhythmGame_arena_qml_test RhythmGame_exe -j 2
ctest --preset dev-rel -R "ArenaOverlayCustomization|ArenaQml|ArenaSession" --output-on-failure
git -C T:/RG/.worktrees/online-arena add src/gameplay_logic/ChartRunner.* src/arena/ArenaSession.* RhythmGameQml/Arena/ArenaOverlayHost.qml RhythmGameQml/ContentFrame.qml RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml test/arena/ArenaOverlayCustomization.test.cpp test/qml/tst_ArenaCustomizationHost.qml test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: coordinate Arena overlay customization"
```

---

### Task 4: Create shared room views and the native Default select panel

**Required sub-skill:** Read and apply `qt-qml` before editing QML.

**Files:**
- Create: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaRosterView.qml`
- Create: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaChatView.qml`
- Create: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaSelectionSummary.qml`
- Create: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/select/ArenaSelectPanel.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/select/Select.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaSelectStrip.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/ContentFrame.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/CMakeLists.txt`
- Create: `T:/RG/.worktrees/online-arena/test/qml/tst_ArenaDefaultSelect.qml`

**Interfaces:**
- Consumes: Phase 1-3 `members`, `chat`, owner/self IDs, room/selection/ready/
  availability/round values, `lastResult`, `kickMember`, `setReady`,
  `sendChat`, and `leaveRoom`.
- Produces shared components with model-only contracts:

```qml
ArenaRosterView {
    required property var session
    required property bool moderationEnabled
    property bool compact: false
    signal kickRequested(string memberId)
}

ArenaChatView {
    required property var session
    required property var chatModel
    property bool inputEnabled: true
    property int unreadCount: 0
    signal sent()
}

ArenaSelectionSummary {
    required property var session
    property bool compact: false
}
```

- Produces: `Select.qml` capability
  `readonly property bool arenaNativeSelectPresentation: true`.

- [ ] **Step 1: Write failing shared-view and native-select tests**

Use QML fake value-row models to test 16 members, owner/local/winner markers,
connected/reserved/waiting/loading/ready/inventory states, owner-only kick,
selection/options/sync/cancellation, ready/unready disabled reasons, last joint
winners/wins, chat backlog/tail-follow/send, leave, focus order, and every
remote string remaining plain text.

- [ ] **Step 2: Run RED**

```powershell
cmake --build --preset dev-rel --target RhythmGame_arena_qml_test -j 2
ctest --preset dev-rel -R ArenaQml --output-on-failure
```

- [ ] **Step 3: Implement the shared views without networking logic**

Roster delegates read roles only and emit `kickRequested`; the owning panel
calls `session.kickMember(memberId)`. Chat calls only the existing
`sendChat(text)`, clears on accepted local submission, follows initial/model-
reset/rows-inserted tail only when already at the tail, and keeps scroll
position when reading history. Selection summary formats only existing
QML-facing values and never reconstructs a wire snapshot.

- [ ] **Step 4: Mount the Default panel in its authored layout**

Declare the native capability on the outer `FocusScope`. Mount
`ArenaSelectPanel` in the 640x480 left information region used by StageFile,
keeping a bounded StageFile header and the normal right song list/search/sort/
options. The panel is active only while Session is seated. It keeps roster plus
room/selection/ready summary visible and switches the lower area between
member detail and chat.

- [ ] **Step 5: Preserve Phase 2 policy gates**

Assert the native panel does not instantiate/enable profile switch, local
battle, course, replay, autoplay, or ranking-play paths. Keep chart activation
and unavailable decoration in their Phase 2 functions. ContentFrame suppresses
the minimal strip only when the current item reports native select
presentation.

- [ ] **Step 6: Verify and commit**

```powershell
cmake --build --preset dev-rel --target RhythmGame_arena_qml_test RhythmGame_qml_qmllint RhythmGame_exe -j 2
ctest --preset dev-rel -R "ArenaQml|ArenaSelect|ArenaSession" --output-on-failure
git -C T:/RG/.worktrees/online-arena add RhythmGameQml/Arena/ArenaRosterView.qml RhythmGameQml/Arena/ArenaChatView.qml RhythmGameQml/Arena/ArenaSelectionSummary.qml RhythmGameQml/Arena/ArenaSelectStrip.qml RhythmGameQml/ContentFrame.qml RhythmGameQml/CMakeLists.txt share/RhythmGame/themes/Default/scripts/select/ArenaSelectPanel.qml share/RhythmGame/themes/Default/scripts/select/Select.qml test/qml/tst_ArenaDefaultSelect.qml
git -C T:/RG/.worktrees/online-arena commit -m "feat: integrate Arena into Default select"
```

---

### Task 5: Replace the minimal legacy select strip with the full LR2/Beatoraja overlay

**Required sub-skill:** Read and apply `qt-qml` before editing QML.

**Files:**
- Create: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaLegacySelectOverlay.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaOverlayHost.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaSelectStrip.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/ContentFrame.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml`
- Create: `T:/RG/.worktrees/online-arena/test/qml/tst_ArenaLegacySelect.qml`

**Interfaces:**
- Consumes: Task 4 shared views and current-screen native capability.
- Produces: one fallback overlay for LR2, Beatoraja, and unknown third-party
  screens; it adds no skin-format command/value and does not alter Phase 2
  selection decoration.

- [ ] **Step 1: Write failing fallback-selection tests**

Test native capability suppresses fallback, absent/false capability enables it,
LR2/Beatoraja family labels do not change behavior, compact header remains
visible, expand/collapse, 16-row scrolling, owner kick, ready/unready, sync/
loading/cancellation, leave, chat, last winner/wins, 420 px width cap, 24 px
safe margins, and no pointer capture outside the panel.

- [ ] **Step 2: Run RED**

```powershell
cmake --build --preset dev-rel --target RhythmGame_arena_qml_test -j 2
ctest --preset dev-rel -R ArenaQml --output-on-failure
```

- [ ] **Step 3: Implement the compact/expanded overlay**

Compact content is exactly room name, selected/sync phase, ready state, and
connected/reserved count. Expanded content instantiates Task 4 roster,
selection summary, chat, ready/leave, and last-result winner summary. Bound
width to `Math.min(420, viewport.width - 48)` and height to
`viewport.height - 48`; use internal Flickables rather than expanding beyond
the viewport.

- [ ] **Step 4: Route fallback by declared capability**

ContentFrame/host examine only
`sceneStack.currentItem.arenaNativeSelectPresentation === true`. Do not compare
theme path or literal `Default`. `Lr2SkinScreenWrapper` remains without native
capability. An unknown theme therefore receives the safe fallback.

- [ ] **Step 5: Verify LR2/Beatoraja selection semantics**

Run existing Phase 2 select tests proving LR2 still prefixes
`(arena unavailable)`, Beatoraja still uses unavailable body/title types,
table-missing remains separate, and unavailable activation cannot replace the
server selection.

- [ ] **Step 6: Verify and commit**

```powershell
cmake --build --preset dev-rel --target RhythmGame_arena_qml_test RhythmGame_qml_qmllint RhythmGame_exe -j 2
ctest --preset dev-rel -R "ArenaQml|ArenaSelect" --output-on-failure
git -C T:/RG/.worktrees/online-arena add RhythmGameQml/Arena/ArenaLegacySelectOverlay.qml RhythmGameQml/Arena/ArenaOverlayHost.qml RhythmGameQml/Arena/ArenaSelectStrip.qml RhythmGameQml/ContentFrame.qml RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml test/qml/tst_ArenaLegacySelect.qml
git -C T:/RG/.worktrees/online-arena commit -m "feat: show complete Arena legacy select overlay"
```

---

### Task 6: Place the forced live leaderboard and attach gameplay chat safely

**Required sub-skill:** Read and apply `qt-qml` before editing QML.

**Files:**
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaOverlayHost.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaGameplayOverlay.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaGameplayChat.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaOverlayPlacementFrame.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/ContentFrame.qml`
- Create: `T:/RG/.worktrees/online-arena/test/qml/tst_ArenaGameplayPresentation.qml`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaOverlayPolicy.test.cpp`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: Tasks 1-3 placement/F2, Phase 3 live model/chat action, and
  canonical gameplay descriptor.
- Produces: forced `gameplayLeaderboard` frame plus deterministic adjacent
  chat placement; no persistent expanded/chat/unread state.

- [ ] **Step 1: Write failing forced-visibility/row tests**

Cover active runner detection, no overlay for a noncurrent/wrong runner,
compact always instantiated, no hide property/action/record, rank/dash,
zero/no-data, name/EX/progress/state, expanded BP/combo/six judgements/gauge/
options, 16-row scrolling, local row, opponent target, DNF/finished, and cleanup.

- [ ] **Step 2: Write failing chat attachment and priority tests**

For leaderboard rectangles near every viewport edge, assert right/left/below/
above/largest-safe fallback order, no overlap with the leaderboard, safe
clipping, F8 open, incoming closed-message unread increment, open clears unread,
no auto-open, Enter/Shift+Enter, focused keyboard isolation, controller
continuation, Escape chat/customize/abort priority, and round cleanup.

- [ ] **Step 3: Run RED**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test RhythmGame_arena_qml_test -j 2
ctest --preset dev-rel -R "ArenaOverlayPolicy|ArenaQml" --output-on-failure
```

- [ ] **Step 4: Wrap Phase 3 gameplay content in the placement frame**

Resolve the current screen/theme property map and `layoutVariant`, pass that
existing theme-vars map to the frame, and bind host lifetime to the current
Arena runner. Keep compact content independent of `expanded`; only detail
delegates change. The frame z is above the skin and below application dialogs.

- [ ] **Step 5: Implement deterministic drawer geometry and unread state**

Use available safe rectangles in the specification's right/left/below/above
order. Target width is `clamp(max(leaderboard.width, 320), 320, 420)` and target
height is `min(360, viewport.height - 48)` before fallback. Track unread from
`ArenaChatModel.rowsInserted` only while chat is closed and the same round is
active. Reset on open, round change, leave, and result transition. Do not write
drawer geometry or unread count to Profile/session/server.

- [ ] **Step 6: Add and persist the first-use hint**

Show translated `Press F2 to move Arena standings` for six seconds only when
`generalVars.arenaOverlayHintVersion < 1`. Dismiss/F2 raises that profile-wide
version to `1`. The hint has no focus steal and cannot cover the active
placement handles.

- [ ] **Step 7: Verify and commit**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test RhythmGame_arena_qml_test RhythmGame_qml_qmllint RhythmGame_exe -j 2
ctest --preset dev-rel -R "ArenaOverlay|ArenaQml|ArenaSessionCompetition" --output-on-failure
git -C T:/RG/.worktrees/online-arena add RhythmGameQml/Arena/ArenaOverlayHost.qml RhythmGameQml/Arena/ArenaGameplayOverlay.qml RhythmGameQml/Arena/ArenaGameplayChat.qml RhythmGameQml/Arena/ArenaOverlayPlacementFrame.qml RhythmGameQml/ContentFrame.qml test/qml/tst_ArenaGameplayPresentation.qml test/arena/ArenaOverlayPolicy.test.cpp test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: place Arena live standings and chat"
```

---

### Task 7: Integrate Default results and movable LR2/Beatoraja result standings

**Required sub-skill:** Read and apply `qt-qml` before editing QML.

**Files:**
- Create: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/result/ArenaResultPanel.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/result/Result.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/result/Side.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/result/ScoreColumn.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/result/RankingPosition.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaResultOverlay.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaOverlayHost.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SkinValueResolver.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml`
- Create: `T:/RG/.worktrees/online-arena/test/qml/tst_ArenaResultPresentation.qml`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaResultPresentation.test.cpp`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: Phase 3 `presentedResult`, transient Default source, and legacy
  numbers 179/180; Tasks 1-2 `resultStandings` placement.
- Produces: Default capability
  `readonly property bool arenaNativeResultPresentation: true`; fallback result
  is movable for every screen without it.

- [ ] **Step 1: Write failing Default native-result tests**

Cover pending immediately, every joint winner listed first, all-DNF/no-winner,
local row marker, competition `1,1,3`, DNF dash, participant count including
DNF, lobbyWinsAfter including removed-seat null, compact/detail collapse,
Arena source default, forward/reverse provider cycle, and unchanged saved
provider/online queries/IR status independence.

- [ ] **Step 2: Write failing legacy-result tests**

Cover absent native capability, compact winner/rank/count always visible,
pending/DNF, expanded full standings, `resultStandings` descriptor, independent
gameplay/result records, F2 move/resize/reset, and values 179/180 unchanged
inside/outside Arena whether the skin renders them or not.

- [ ] **Step 3: Run RED**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test RhythmGame_arena_qml_test -j 2
ctest --preset dev-rel -R "ArenaResult|ArenaQml" --output-on-failure
```

- [ ] **Step 4: Mount the native Default result panel**

Declare the capability on Result. Add an always-visible winner/pending banner
near the existing chart header and a bounded standings detail panel beside the
single-player score layout. The panel reads `presentedResult`; it does not
query or mutate online ranking models. Keep Phase 3's screen-local Arena source
cycle and immediate normal ranking query behavior.

- [ ] **Step 5: Wrap legacy result content in the placement frame**

When native capability is absent, instantiate `ArenaResultOverlay` with kind
`resultStandings`, the resolved result skin ID, and layout `result`. Compact
mode contains every winner plus local rank/count or DNF; expanded mode contains
complete final rows/wins. Pending remains visible while waiting.

- [ ] **Step 6: Preserve narrow legacy resolver behavior**

Keep number 179 as finalized local rank or zero and number 180 as frozen
participant count only for the matching active Arena result presentation.
Outside it, use existing internet rank/player-count values. Do not add Arena to
`OnlineRankingModel`.

- [ ] **Step 7: Verify and commit**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test RhythmGame_arena_qml_test RhythmGame_qml_qmllint RhythmGame_exe -j 2
ctest --preset dev-rel -R "ArenaResult|ArenaQml|ArenaSessionCompetition" --output-on-failure
git -C T:/RG/.worktrees/online-arena add share/RhythmGame/themes/Default/scripts/result RhythmGameQml/Arena/ArenaResultOverlay.qml RhythmGameQml/Arena/ArenaOverlayHost.qml RhythmGameQml/Lr2/Lr2SkinValueResolver.qml RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml test/qml/tst_ArenaResultPresentation.qml test/arena/ArenaResultPresentation.test.cpp test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: polish Arena result presentation"
```

---

### Task 8: Complete accessibility, supported localization, and responsive presentation gates

**Required sub-skill:** Read and apply `qt-qml` before editing QML.

**Files:**
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaRosterView.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaChatView.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaSelectionSummary.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaLegacySelectOverlay.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaOverlayPlacementFrame.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaGameplayOverlay.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaGameplayChat.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaResultOverlay.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/select/ArenaSelectPanel.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/result/ArenaResultPanel.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/translations/Default_en.ts`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/translations/Default_pl.ts`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/translations/Default_jp.ts`
- Modify: `T:/RG/.worktrees/online-arena/cmake/translations.cmake`
- Create: `T:/RG/.worktrees/online-arena/test/qml/tst_ArenaAccessibility.qml`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaTranslationContract.test.cpp`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: Tasks 2 and 4-7 complete presentation surfaces.
- Produces: deterministic focus/accessibility/scale behavior and a translation
  contract limited to Arena contexts; no new advertised locale.

- [ ] **Step 1: Add failing accessibility/focus tests**

For native and legacy surfaces, test Tab/Backtab reaches every enabled action in
document order, focus remains visible, disabled kick/ready is skipped, list
delegates expose meaningful accessible names, all 32/40 px target minimums,
status is present as text/icon as well as color, and reconnect/winner/DNF uses
one bounded status announcer. Inspect every remote-text property and assert the
owning `Text`/`TextInput` uses plain-text semantics.

- [ ] **Step 2: Add failing responsive tests**

Instantiate 16-member/select/gameplay/result surfaces at 1024x768, 1280x720,
1920x1080, 2560x1080, and 3840x2160 with scale multipliers 1.0, 1.5, and 2.0.
Assert interactive items stay in bounds, essential labels retain positive
width/height, overflow uses Flickable/ScrollView, and focus items remain
reachable.

- [ ] **Step 3: Add the translation contract test and run RED**

Parse TS XML with `QXmlStreamReader`. For contexts whose name or source path
contains `Arena`, assert every Polish translation is present and not unfinished;
assert every source string appears in English/Polish/Japanese catalogs after
extraction; allow Japanese translation fallback because it is not advertised
in `theme.json`.

```powershell
cmake --build --preset dev-rel --target Default_translations RhythmGame_test RhythmGame_arena_qml_test -j 2
ctest --preset dev-rel -R "ArenaTranslation|ArenaQml" --output-on-failure
```

- [ ] **Step 4: Apply accessibility and contrast rules**

Set `Accessible.role/name/description`, active focus visuals, explicit
`Text.PlainText`, elide/tooltips, and text/icon state labels. Application-owned
palettes use at least 4.5:1 foreground/background and 3:1 focus/control contrast;
test the fixed palette constants rather than sampling rendered screenshots.

- [ ] **Step 5: Extract and finish supported copy**

Run lupdate, translate every Arena-context Polish entry naturally, keep English
source entries valid, and leave Japanese Arena entries as source fallback where
no reviewed translation exists. Do not add `jp` to Default `theme.json` in this
phase.

```powershell
cmake --build --preset dev-rel --target Default_translations release_translations -j 2
```

- [ ] **Step 6: Run GREEN and commit**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test RhythmGame_arena_qml_test RhythmGame_qml_qmllint RhythmGame_exe Default_translations release_translations -j 2
ctest --preset dev-rel -R "ArenaTranslation|ArenaQml|ArenaOverlay|ArenaResult|ArenaSelect" --output-on-failure
git -C T:/RG/.worktrees/online-arena add RhythmGameQml/Arena share/RhythmGame/themes/Default/scripts/select/ArenaSelectPanel.qml share/RhythmGame/themes/Default/scripts/result/ArenaResultPanel.qml share/RhythmGame/themes/Default/translations cmake/translations.cmake test/qml/tst_ArenaAccessibility.qml test/arena/ArenaTranslationContract.test.cpp test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: polish Arena accessibility and localization"
```

---

### Task 9: Add trusted-proxy address resolution and bounded connection admission

**Files:**
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/transport/client-address.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/transport/connection-admission.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/config.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/transport/start-server.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/application/delivery.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/.env.example`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/client-address.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/connection-admission.test.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/integration/websocket.test.ts`

**Interfaces:**
- Consumes: direct Bun peer address and request headers before WebSocket
  application allocation.
- Produces:

```ts
export type AddressKey = string & { readonly __addressKey: unique symbol };

export interface ClientAddressResolver {
  resolve(input: Readonly<{
    directPeer: string;
    forwardedFor: string | null;
  }>): AddressKey;
}

export interface ConnectionAdmission {
  attemptUpgrade(address: AddressKey, nowMs: number):
    | Readonly<{ accepted: true; leaseId: string }>
    | Readonly<{ accepted: false; status: 429 | 503 }>;
  markHello(leaseId: string): void;
  release(leaseId: string): void;
  sweep(nowMs: number): readonly Readonly<{
    leaseId: string;
    reason: 'hello_timeout';
  }>[];
}
```

- Produces validated defaults:

```text
TRUSTED_PROXY_CIDRS=
UPGRADE_ATTEMPTS_PER_ADDRESS_PER_MINUTE=120
MAX_CONNECTIONS_PER_ADDRESS=20
CLIENT_HELLO_TIMEOUT_MS=10000
MAX_TRACKED_ADDRESSES=20000
```

- [ ] **Step 1: Write failing canonical-address tests**

Cover IPv4/IPv6 canonicalization, CIDR exact/inside/outside, empty trust list,
trusted single/multiple proxy chain, right-to-left stripping, untrusted peer
ignoring headers, eight/nine entries, 512/513 bytes, whitespace, invalid IP,
ports, zone IDs, hostnames, empty entries, duplicate CIDRs, rejected
`0.0.0.0/0`/`::/0`, and no raw input in thrown/public errors.

- [ ] **Step 2: Write failing admission lifecycle tests**

Cover 120/121 attempts in a rolling minute, 20/21 concurrent leases, independent
addresses, exact 10,000 ms hello boundary, hello cancellation, release on every
path, duplicate release, 120-timestamp cap, idle-entry eviction, the
20,000/20,001 tracked-address boundary, global 5,000-connection capacity
precedence, shutdown, and process-salted HMAC keys differing across resolver
instances.

- [ ] **Step 3: Run RED**

```powershell
bun --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server test tests/unit/client-address.test.ts tests/unit/connection-admission.test.ts
```

- [ ] **Step 4: Implement strict proxy-chain resolution**

Parse only `X-Forwarded-For`, at most eight entries/512 bytes, only when the
direct peer belongs to a validated configured CIDR. Walk right-to-left while
addresses are trusted and select the first untrusted address; any malformed or
ambiguous chain falls back to direct peer. Return
`HMAC-SHA-256(processRandomSalt, canonicalAddress)`, never the address itself.

- [ ] **Step 5: Implement admission before upgrade/application state**

Reserve address/global counts before `server.upgrade`. If upgrade fails,
release immediately. Associate lease ID with socket data; `client_hello`
completion marks it. A one-shot earliest-deadline timer closes expired sockets
with 1008/`hello_timeout`. Normal close, fatal close, failed handshake, 1013,
shutdown, and thrown receive all use the same idempotent release. Retain at most
120 rolling timestamps per address, evict idle expired entries, and return 503
for a new address when 20,000 live entries remain after sweeping.

- [ ] **Step 6: Verify real WebSocket behavior and commit**

```powershell
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server verify
git -C T:/RhythmGame-IR/.worktrees/online-arena add arena-server/src/transport/client-address.ts arena-server/src/transport/connection-admission.ts arena-server/src/transport/start-server.ts arena-server/src/application/delivery.ts arena-server/src/config.ts arena-server/.env.example arena-server/tests/unit/client-address.test.ts arena-server/tests/unit/connection-admission.test.ts arena-server/tests/integration/websocket.test.ts
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "feat: bound Arena proxy connection admission"
```

---

### Task 10: Add privacy-safe metrics, graceful deployment, and Coolify artifacts

**Files:**
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/observability/operational-metrics.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/config.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/main.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/transport/start-server.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/application/arena-application.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/room-directory.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/inventory/inventory-upload-manager.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/Dockerfile`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/.dockerignore`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/.env.example`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/package.json`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/README.md`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/ops/coolify.md`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/ops/production.env.example`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/scripts/phase4-production-smoke.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/operational-metrics.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/integration/metrics.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/integration/shutdown.test.ts`

**Interfaces:**
- Consumes: existing domain/application/gateway transitions through narrow
  counter calls; no metrics module receives protocol bodies.
- Produces:

```ts
export interface OperationalMetrics {
  connectionOpened(): void;
  connectionClosed(closeClass: 'normal' | 'policy' | 'overload' | 'restart' | 'error'): void;
  setRooms(value: number): void;
  setReservedSeats(value: number): void;
  setRoundsActive(value: number): void;
  roundStarted(): void;
  roundFinalized(): void;
  roundCancelled(reason: LaunchCancelReason): void;
  authFailure(reason: AuthFailureMetricReason): void;
  commandRejected(code: MetricCommandCode): void;
  setInventoryCommittedBytes(value: number): void;
  observeInventoryUpload(seconds: number): void;
  standingsDropped(): void;
  renderPrometheus(): string;
}
```

- Produces config `METRICS_ENABLED=false`, mandatory 32-byte-or-longer
  `METRICS_BEARER_TOKEN` when enabled, and `SHUTDOWN_DRAIN_MS=8000`.

- [ ] **Step 1: Write failing metric type/label/transition tests**

Assert every exact metric name, TYPE line, fixed histogram bucket, monotonic
counter, current gauge, room/seat/round transition, upload duration/count/sum,
closed known labels, unknown mapped to `other`, and absence of injected room/
round/user/IP/hash/chat/score/token strings in rendered output.

- [ ] **Step 2: Write failing HTTP metric authorization tests**

Cover disabled 404, enabled-with-short-token startup failure, missing/malformed/
wrong bearer 401, correct bearer 200 Prometheus content type and no-store, only
exact GET `/metrics`, constant-time helper for equal-length bytes, and no token/
Authorization header in logs.

- [ ] **Step 3: Write failing graceful-shutdown tests**

With fake time and real sockets, assert repeated SIGTERM is idempotent, upgrades
become 503 first, going-away precedes close, new mutations stop, reliable sends
drain up to exactly 8,000 ms, remaining sockets close 1012, timers/uploads/
rooms/address leases release, `/healthz` remains liveness-only, and process exit
does not wait on IR/JWKS.

- [ ] **Step 4: Run RED**

```powershell
bun --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server test tests/unit/operational-metrics.test.ts tests/integration/metrics.test.ts tests/integration/shutdown.test.ts
```

- [ ] **Step 5: Implement bounded metrics and narrow instrumentation**

Use integers/number sums, fixed arrays/maps initialized from closed enums, and
no general `labels: Record<string,string>` API. RoomDirectory publishes aggregate
gauge values after transitions; gateway/application call event counters with
closed reasons only. Render a fresh bounded string on authorized request and
never retain a scrape response.

- [ ] **Step 6: Implement the shutdown state machine and container metadata**

Add one idempotent shutdown promise with the exact sequence from the spec. Add
`STOPSIGNAL SIGTERM`, OCI source/revision/version labels, preserve pinned Bun/
non-root, and verify operation under `--read-only --tmpfs /tmp`. Keep health at
`/healthz` and do not copy ops/tests/environment secrets into the runtime image.

- [ ] **Step 7: Write Coolify artifacts and credential-free smoke**

`ops/coolify.md` specifies Dockerfile base `/arena-server`, port 3001, one
replica, no volume, public `arena.rhythmgame.eu`, WSS `/ws`, health `/healthz`,
15-second stop grace, at least five-minute idle timeout, actual private Traefik
CIDR, token-private metrics, resource ceilings, and forbidden IR secrets.
`production.env.example` contains keys but no metric token value. The smoke
accepts only HTTPS/WSS origin, performs health/anonymous directory/query-string/
certificate checks, and has no credential argument/environment path.

- [ ] **Step 8: Verify and commit**

```powershell
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server verify
docker build --pull --tag rhythmgame-arena:phase4 T:/RhythmGame-IR/.worktrees/online-arena/arena-server
docker run --rm --detach --name rhythmgame-arena-phase4-ops --read-only --tmpfs /tmp --publish 127.0.0.1:3001:3001 rhythmgame-arena:phase4
Invoke-RestMethod http://127.0.0.1:3001/healthz
docker stop rhythmgame-arena-phase4-ops
git -C T:/RhythmGame-IR/.worktrees/online-arena add arena-server
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "feat: harden Arena production operations"
```

If Docker is unavailable, run all Bun gates and record the Docker commands as
an environmental blocker; do not claim the image checks passed.

---

### Task 11: Run malformed/load/privacy/skin release gates and record traceability

**Files:**
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/integration/phase4-malformed.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/scripts/phase4-load.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/package.json`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/docs/phase4-verification.md`
- Create: `T:/RG/.worktrees/online-arena/docs/arena/phase4-verification.md`
- Modify only when a release gate proves a defect: files owned by Tasks 1-10.

**Interfaces:**
- Consumes: complete Phase 4 client/server plus all earlier smoke/golden suites.
- Produces: deterministic malformed corpus, bounded 200-client soak, container/
  WSS proof, representative-skin matrix, and exit-criteria traceability.

- [ ] **Step 1: Freeze both repository heads and prove protocol non-drift**

Record both SHAs. Compare Phase 1/2/3 JSON and binary fixture SHA-256 values
between repositories. Run the Phase 1, Phase 2, and Phase 3 smoke scripts before
the new load test. Any unexplained protocol fixture drift blocks release.

- [ ] **Step 2: Implement and run the malformed corpus**

Drive HTTP method/path/query/header limits, invalid/trusted forwarding chains,
hello timeout, malformed/oversized JSON, each malformed `RGA1` field, transfer
order/digest/count/budget errors, stale generations/revisions/rounds, telemetry
regression/rate close, final conflict, slow reader, metrics auth, and shutdown
during each room phase. After every case assert a control room remains correct,
partial memory/counters return to baseline, and sentinel secrets never appear
in response/log/metric text.

```powershell
bun --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server test tests/integration/phase4-malformed.test.ts
```

- [ ] **Step 3: Implement the bounded load/soak script**

With an injected in-memory verifier/fake clock for setup and real WebSockets for
traffic, create 200 authenticated sockets in 25 rooms of eight. Commit partly
overlapping inventories, select/ready/start, send five-hertz telemetry for 30
seconds, send chat/selection below limits, reconnect one seat per room, finalize
all rooms, leave, and destroy them. Record RSS, event-loop delay distribution,
current/peak buffered bytes, dropped ephemeral standings, inventory bytes, and
post-cleanup gauges. Assert no missed finalization, unbounded queue, leaked room/
seat/transfer/limiter, or nonzero current gauges after cleanup; report timings
without a machine-dependent pass threshold.

```powershell
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server load:phase4
```

- [ ] **Step 4: Run complete automated client/server gates**

```powershell
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server verify
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server smoke:phase1
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server smoke:phase2
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server smoke:phase3
cmake --preset dev-rel
cmake --build --preset dev-rel --target RhythmGame_test RhythmGame_arena_qml_test RhythmGame_qml_qmllint RhythmGame_exe Default_translations release_translations -j 2
ctest --preset dev-rel -R "Arena|ChartLoader|GeneratePermutation|SongDbScanner" --output-on-failure
```

Also run the IR Better Auth ticket test and root Svelte check; an unavailable
database is recorded explicitly.

- [ ] **Step 5: Run Docker/Linux and public-origin gates**

Build/run the image non-root and read-only, enable token metrics, drive the
malformed/load smoke through it, send SIGTERM, inspect layers for tests/env/VCS/
IR secrets, and run the credential-free production smoke against the official
origin only when DNS/certificate/Coolify access exists.

```powershell
docker build --pull --tag rhythmgame-arena:phase4 T:/RhythmGame-IR/.worktrees/online-arena/arena-server
docker run --rm --detach --read-only --tmpfs /tmp --publish 127.0.0.1:3001:3001 --name rhythmgame-arena-phase4 rhythmgame-arena:phase4
Invoke-RestMethod http://127.0.0.1:3001/healthz
docker stop rhythmgame-arena-phase4
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server smoke:production -- https://arena.rhythmgame.eu
```

- [ ] **Step 6: Perform the representative-skin matrix**

Use two local app instances and Default, one LR2 skin, and one Beatoraja skin.
For k5/k7/k10/k14 plus result, verify select roster/chat/ready/kick/wins,
unavailable charts, live compact/expanded standings, strongest opponent, F2
drag/resize/nudge/reset, restart/profile/skin/layout isolation, clamp at 4:3/
16:9/21:9, 100/150/200% scale, first hint, chat flip/unread/no pause, input
suppression/restore, reconnect/DNF/ties/winners, Default provider cycle, legacy
179/180, and normal offline/local-battle regressions.

- [ ] **Step 7: Write exact traceability and fix only proven defects**

The RG report maps every client/skin/accessibility/translation exit criterion
to an automated command or named manual observation. The IR report records
protocol digests, malformed corpus, soak metrics, privacy search, image
inspection, shutdown, Coolify settings, and public smoke. Mark Docker/Netcup/
DNS/Coolify limitations as blockers with the failing command and output; do not
convert them into passes.

- [ ] **Step 8: Commit integration-owned reports/scripts separately**

```powershell
git -C T:/RhythmGame-IR/.worktrees/online-arena add arena-server/tests/integration/phase4-malformed.test.ts arena-server/scripts/phase4-load.ts arena-server/package.json arena-server/docs/phase4-verification.md
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "test: verify Arena production release"
git -C T:/RG/.worktrees/online-arena add docs/arena/phase4-verification.md
git -C T:/RG/.worktrees/online-arena commit -m "docs: verify Arena phase 4"
```

---

## Final review gate

After Task 11, run independent reviews for:

1. Placement file validation/atomicity/profile isolation and no remote data.
2. QML binding/focus/pointer ownership, forced visibility, F2 coordination,
   viewport clamp, chat priority, and Default/LR2/Beatoraja capability fallback.
3. ChartRunner input-suppression timing/held-key release and every session
   cleanup race without non-Arena regression.
4. Native Default ranking/result semantics, legacy 179/180, accessibility,
   plain text, localization, and IR-upload independence.
5. Trusted-proxy parsing, address/hello bounds, metrics label/privacy policy,
   backpressure, shutdown, and resource release.
6. Whole-branch protocol non-drift, malformed/load/skin evidence, Docker/Linux,
   and Coolify runbook accuracy.

Critical or Important findings are fixed and re-reviewed. The feature is
release-ready only after all automated gates pass and every unavailable
external deployment/visual gate is named explicitly rather than inferred.

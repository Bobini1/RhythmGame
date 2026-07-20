# Arena Skin Score-Target Compatibility Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Present the strongest valid Arena opponent through the ordinary Default, LR2, and Beatoraja score-target interfaces without enabling ghost-battle or saved-rival modes.

**Architecture:** Keep opponent selection in `ArenaOpponentTarget` and adapt only presentation state. The LR2 wrapper translates target identity and change notifications into its cached text, number, bargraph, and timer mechanisms; the Default theme carries target availability to its existing ghost-score renderer.

**Tech Stack:** Qt 6, QML, LR2 CSV compatibility, Beatoraja skin properties, CMake, CTest.

## Global Constraints

- Do not change Arena protocol or opponent-selection logic.
- Do not enable LR2 option `623`, option `625`, or online option `51` for Arena.
- Do not project or interpolate remote scores.
- Do not change Arena result-skin semantics or add a private Arena skin option.
- Preserve all unrelated tracked and untracked workspace changes.

---

### Task 1: LR2 and Beatoraja target adapter

**Files:**
- Modify: `RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml`

**Interfaces:**
- Consumes: `arenaSession.opponentTarget.{available,displayName,exScore}` and existing `gameplayTargetScorePoints()`, `gameplayTargetFinalPoints()`, renderer revision, text registry, and gameplay timer helpers.
- Produces: Arena-aware `lr2TargetText()`, fresh target values after every accepted snapshot, and Beatoraja timer `352` transitions.

- [x] **Step 1:** Override `lr2TargetText()` only while `arenaGameplayOwned` is true, returning the opponent display name when available and an empty string otherwise.
- [x] **Step 2:** Add an `opponentTarget.changed` connection that refreshes both cached gameplay-number sides, target text IDs `1` and `3`, and gameplay bargraphs without rebuilding runtime options.
- [x] **Step 3:** Add a transition-based Beatoraja timer `352` updater. Gate Arena on target availability, compare local EX against `gameplayTargetFinalPoints(1)`, preserve the first activation time, and clear the timer when the condition becomes false.
- [x] **Step 4:** Invoke the timer updater from target changes, score changes, target-setting changes, gameplay status changes, and saved-score refreshes; reset its local timestamp with other gameplay timers.
- [x] **Step 5:** Run `qmllint` on `Lr2SkinScreenWrapper.qml`; expect exit code `0`.

### Task 2: Delay-safe Default ghost score

**Files:**
- Modify: `share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml`
- Modify: `share/RhythmGame/themes/Default/scripts/gameplay/Side.qml`
- Modify: `share/RhythmGame/themes/Default/scripts/gameplay/PlayArea.qml`

**Interfaces:**
- Consumes: the existing `arenaOpponentTargetAvailable`, `targetPoints1`, and `targetFinalPoints1` properties.
- Produces: a `pointTargetAvailable` presentation property propagated from `Gameplay` through `Side` to `PlayArea`.

- [x] **Step 1:** Add `pointTargetAvailable` with a non-Arena-compatible default of `true` to `Side` and `PlayArea`, and bind it through the existing `PlayArea` instance.
- [x] **Step 2:** Pass `!arenaGameplayOwned || arenaOpponentTargetAvailable` to both Default gameplay sides so DP uses the same Arena target availability.
- [x] **Step 3:** Keep the ghost-score color transparent while `pointTargetAvailable` is false; leave ordinary, battle, and customization behavior unchanged.
- [x] **Step 4:** Run `qmllint` on all three Default gameplay files; expect exit code `0`.

### Task 3: Verification and scoped commit

**Files:**
- Verify: all implementation files from Tasks 1 and 2.
- Verify: `docs/superpowers/specs/2026-07-20-arena-skin-score-target-compatibility-design.md`
- Verify: `docs/superpowers/plans/2026-07-20-arena-skin-score-target-compatibility.md`

**Interfaces:**
- Consumes: the existing configured build tree and focused Arena Catch2 tests.
- Produces: fresh lint, build, test, diff, review, and commit evidence.

- [x] **Step 1:** Build `RhythmGame_qml`, `RhythmGame_lr2_qml`, and `RhythmGame_test`; expect exit code `0`.
- [x] **Step 2:** Run the focused `ArenaSessionCompetition` opponent-target cases; expect all selected cases to pass.
- [x] **Step 3:** Run the full available CTest suite with failure output; expect zero failed tests.
- [x] **Step 4:** Run `git diff --check` on the scoped files and inspect their complete diff, confirming no option `623`, `625`, or `51` behavior changed.
- [x] **Step 5:** Run the repository code-review workflow, address any high-confidence finding, repeat affected verification, and commit only the scoped files.

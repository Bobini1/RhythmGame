# Per-Key Input Debounce Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the one-sided release-to-press lockout with zero-press-latency, independently scheduled debounce state for every `BmsKey`.

**Architecture:** `InputTranslator` will keep physical, debounced, and pending-release state per logical key and own one precise single-shot release timer per key. Digital releases are committed only after their own timer expires; a same-key re-press cancels only that pending release, while synthetic analog/configuration releases bypass debounce.

**Tech Stack:** C++20, Qt 6 `QObject`/`QTimer`, Catch2 3, CMake/MSBuild.

## Global Constraints

- Preserve the timestamps supplied by keyboard and SDL input paths.
- Do not add press latency.
- Do not share pending state or timers between keys.
- Keep analog scratch direction and configuration-driven releases immediate.
- Match the existing 5 ms UI/runtime default when resetting the property.

---

### Task 1: Public-path debounce regression tests

**Files:**
- Modify: `test/input/InputTranslator.test.cpp`

**Interfaces:**
- Consumes: `InputTranslator::handleKeyEvent(quint32, bool, int64_t)`, button state properties, `buttonPressed`, and `buttonReleased`.
- Produces: deterministic regression coverage for the production debounce path.

- [ ] **Step 1: Add a Qt event-loop test harness**

Create an in-memory `properties` database before constructing
`InputTranslator`, bind scan codes 42 and 43 to `Col11` and `Col12`, and add:

```cpp
void runEventLoopFor(int milliseconds)
{
    auto loop = QEventLoop{};
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}
```

- [ ] **Step 2: Add failing behavior tests**

Exercise these exact sequences with `debounceMs = 10`:

```cpp
translator.handleKeyEvent(42, true, 1'000);
translator.handleKeyEvent(42, false, 1'001);
translator.handleKeyEvent(42, true, 1'002);
runEventLoopFor(25);
```

Assert one press, no release, and `col11() == true`. Then assert a later stable
release emits once with its original timestamp. Add `up-down-up`, two-key
independence, zero-debounce, and reset-to-5-ms cases.

- [ ] **Step 3: Build and verify the tests fail for the diagnosed reason**

Run:

```powershell
cmake --build --preset dev-rel --target RhythmGame_test --config Release -- /m:1 /v:minimal
T:\RG\build\dev-rel\test\bin\RhythmGame_test.exe "[debounce]" --reporter compact
```

Expected: the current implementation fails because releases are emitted
immediately and same-key restoring presses are discarded.

---

### Task 2: Independent per-key release debounce

**Files:**
- Modify: `src/input/InputTranslator.h`
- Modify: `src/input/InputTranslator.cpp`
- Test: `test/input/InputTranslator.test.cpp`

**Interfaces:**
- Consumes: existing physical input handlers and existing button signals.
- Produces: unchanged public signals/properties with corrected debounce semantics.

- [ ] **Step 1: Replace the conflated boolean state**

Use one record per `BmsKey`:

```cpp
struct ButtonState
{
    bool physicallyPressed{};
    bool pressed{};
    std::optional<int64_t> pendingReleaseTime;
};

std::array<ButtonState, magic_enum::enum_count<BmsKey>()> buttons;
std::array<QTimer, magic_enum::enum_count<BmsKey>()> debounceTimers;
```

Change property getters to read `ButtonState::pressed`.

- [ ] **Step 2: Add per-key scheduling helpers**

Add these private interfaces:

```cpp
void releaseButton(BmsKey button,
                   int64_t time,
                   bool applyDebounce = true);
void commitPendingRelease(BmsKey button);
void finishRelease(BmsKey button, int64_t time);
auto debounceInterval() const -> std::chrono::milliseconds;
```

Configure every debounce timer as precise and single-shot. Its timeout commits
only the pending release at the matching array index.

- [ ] **Step 3: Implement the state transitions**

On press, ignore a duplicate physical down; cancel the same key's pending
release without emitting another hit; otherwise emit the existing press path.
On digital release, store the original timestamp and start only that key's
timer. On timeout, emit the existing release path once if that key is still
physically released. With zero debounce or `applyDebounce == false`, release
immediately.

- [ ] **Step 4: Preserve synthetic release behavior**

Pass `applyDebounce = false` from key unbinding, analog direction changes, and
analog scratch timeout call sites. Leave keyboard and gamepad button release
handlers on the default debounced path.

- [ ] **Step 5: Run the focused tests**

Run:

```powershell
cmake --build --preset dev-rel --target RhythmGame_test --config Release -- /m:1 /v:minimal
T:\RG\build\dev-rel\test\bin\RhythmGame_test.exe "[debounce]" --reporter compact
```

Expected: all debounce assertions pass with one release per stable transition
and independent state for `Col11` and `Col12`.

---

### Task 3: Property consistency and final verification

**Files:**
- Modify: `src/input/InputTranslator.cpp`
- Test: `test/input/InputTranslator.test.cpp`

**Interfaces:**
- Consumes: `debounceMs` property persistence.
- Produces: a 5 ms reset and safe handling of pending releases when the value changes.

- [ ] **Step 1: Correct reset and live-setting behavior**

Change `resetDebounceMs()` to `setDebounceMs(5.0)`. When the value becomes zero,
commit pending releases immediately; for another positive value, restart each
pending key's precise timer with the new interval.

- [ ] **Step 2: Format and inspect the diff**

Run:

```powershell
cmake --build --preset dev-rel --target format-fix --config Release
git diff --check
git diff -- src/input/InputTranslator.h src/input/InputTranslator.cpp test/input/InputTranslator.test.cpp
```

Expected: formatting succeeds, no whitespace errors, and no unrelated files are changed.

- [ ] **Step 3: Run focused and full verification**

Run:

```powershell
cmake --build --preset dev-rel --target RhythmGame_test --config Release -- /m:1 /v:minimal
T:\RG\build\dev-rel\test\bin\RhythmGame_test.exe "[debounce]" --reporter compact
ctest --test-dir T:\RG\build\dev-rel -C Release --output-on-failure
```

Expected: the focused debounce tests and complete CTest suite pass.

- [ ] **Step 4: Commit only scoped files**

```powershell
git add docs/superpowers/specs/2026-07-19-input-debounce-design.md docs/superpowers/plans/2026-07-19-input-debounce.md src/input/InputTranslator.h src/input/InputTranslator.cpp test/input/InputTranslator.test.cpp
git commit -m "fix: debounce input independently per key"
```

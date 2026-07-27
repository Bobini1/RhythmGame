# Emscripten Playable Slice Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a Chromium-first Emscripten build that launches directly into a real, locally configured BMS chart and lets a player judge input latency and audio feel using the production parser, note model, randomization, referee, scoring, gauges, and keysound semantics.

**Architecture:** Keep one production codebase by extracting portable gameplay seams from the desktop object graph, then compile those same sources into a small `tools/web-playtest` target. The web target materializes an explicitly configured chart package from QRC into MEMFS, decodes its keysounds before countdown, captures `KeyboardEvent.code` timestamps on the browser thread, forwards timestamped commands to a dedicated gameplay pthread, and schedules PCM through an Emscripten Wasm AudioWorklet. QML polls immutable snapshots only; judgement and audio scheduling never run on the UI thread.

**Tech Stack:** C++20, Qt 6.11.1 Quick/QML, Emscripten 4.0.7 pthreads and Wasm Audio Worklets, vcpkg manifest mode, Catch2 3, Python `unittest` source-contract tests, JavaScript Playwright/Chromium launch tooling, CMake/Ninja.

**Design source:** `docs/superpowers/specs/2026-07-27-emscripten-playable-slice-design.md`

## Global Constraints

- Keep native and web gameplay behavior in shared production sources. Platform branches are limited to I/O, threading, browser input, audio backend, and startup/packaging.
- Do not compile a fake scorer, fake note stream, silent autoplay surrogate, or ScriptProcessor audio path.
- Do not remove desktop functionality. Native `Sound::play()` remains valid and delegates to immediate playback.
- Preserve exact nanosecond input timestamps through the browser bridge, gameplay core, hit rules, score, and scheduled sound calls.
- No allocation, Qt calls, logging, filesystem access, decoding, mutexes, or blocking waits in the AudioWorklet render callback.
- Decode every referenced gameplay sound before the countdown can start. A missing or unsupported asset is a visible terminal load error.
- Preserve BMS extension fallback and case-insensitive resolution; the initial Dstorv chart declares `.wav` while shipping `.ogg`.
- Never commit the developer's local chart or absolute chart path. They are configure-time private inputs.
- Keep the existing Gate 1B probe qualification closure intact. The playtest gets a separate vcpkg installation root and is explicitly non-qualification in the wrapper.
- Every task begins from a clean task base, adds a failing focused test first, runs the smallest relevant verification, commits only its scoped files, and receives a fresh task review before the next task.

---

### Task 1: Make Chart Loading Portable and Byte-Addressable

**Files:**

- Create: `src/resource_managers/ChartPlayOptions.h`
- Modify: `src/resource_managers/Vars.h`
- Modify: `src/resource_managers/ChartPlayConfig.h`
- Modify: `src/gameplay_logic/BmsLiveScore.h`
- Modify: `src/gameplay_logic/BmsResult.h`
- Modify: `src/gameplay_logic/BmsResult.cpp`
- Modify: `src/gameplay_logic/BmsResultCourse.h`
- Modify: `src/gameplay_logic/BmsGaugeHistory.h`
- Modify: `src/gameplay_logic/BmsGaugeHistory.cpp`
- Modify: `src/gameplay_logic/BmsScore.h`
- Modify: `src/gameplay_logic/BmsScore.cpp`
- Modify: `src/gameplay_logic/ChartData.h`
- Modify: `src/gameplay_logic/ChartData.cpp`
- Create: `src/gameplay_logic/persistence/BmsResultPersistence.cpp`
- Create: `src/gameplay_logic/persistence/BmsGaugeHistoryPersistence.cpp`
- Create: `src/gameplay_logic/persistence/BmsScorePersistence.cpp`
- Create: `src/gameplay_logic/persistence/ChartDataPersistence.cpp`
- Modify: `src/support/GeneratePermutation.h`
- Modify: `src/resource_managers/ChartDataFactory.h`
- Modify: `src/resource_managers/ChartDataFactory.cpp`
- Modify: `src/charts/BmsNotesData.cpp`
- Modify: `CMakeLists.txt`
- Test: `test/resource_managers/ChartDataFactory.test.cpp`
- Modify: `test/CMakeLists.txt`

**Interfaces:**

```cpp
// src/resource_managers/ChartPlayOptions.h
namespace resource_managers {
namespace note_order_algorithm {
Q_NAMESPACE
enum class NoteOrderAlgorithm {
    Normal,
    Mirror,
    Random,
    SRandom,
    RRandom,
    RandomPlus,
    SRandomPlus,
    BeatorajaRandom,
    BeatorajaRandomEx,
    Lr2Random,
    Lr2RandomEx,
};
Q_ENUM_NS(NoteOrderAlgorithm)
}
using namespace note_order_algorithm;

namespace dp_options {
Q_NAMESPACE
enum class DpOptions { Off, Flip, Battle, Lr2Flip };
Q_ENUM_NS(DpOptions)
}
using namespace dp_options;
}
```

```cpp
// src/resource_managers/ChartDataFactory.h
auto loadChartData(std::string_view chartBytes,
                   const std::filesystem::path& logicalChartPath,
                   RandomGenerator randomGenerator,
                   int64_t directory = 0) const -> ChartComponents;
```

The existing path overload remains public and delegates to the byte overload after acquiring the bytes. Acquisition is:

- Windows: current Win32 mapping.
- Linux: current LLFIO mapping.
- Emscripten: `QFile` read into a local `QByteArray`, then invoke the byte overload while the array is alive.

The byte overload owns parsing, SHA-256/MD5, `#RANDOM` recording, metadata creation, WAV/BMP path construction relative to `logicalChartPath.parent_path()`, `BmsNotesData::fromParsedChart`, and `buildChartComponents`. This ensures byte and path inputs cannot drift.

- [ ] Add `ChartDataFactory.test.cpp` with a self-contained UTF-8 BMS fixture containing metadata, BPM change, scroll change, normal notes, LN start/end, mine, BGM, and two `#RANDOM` branches.
- [ ] Assert fixed random choices produce the same chart metadata, hash, random sequence, note timestamps/types, BGM timestamps, WAV paths, and histogram through path and byte overloads.
- [ ] Assert an incomplete or out-of-range `ExactRandomSequence` is rejected by the existing deterministic chart-load caller.
- [ ] Run the focused test before implementation and capture the expected compile/test failure.
- [ ] Extract `NoteOrderAlgorithm` and `DpOptions` without changing enum values or Qt metatype names.
- [ ] Replace portable headers' inclusion of `Vars.h` with `ChartPlayOptions.h`; keep `Vars.h` including the new header for desktop settings behavior.
- [ ] Replace `db/SqliteCppDb.h` includes in gameplay headers with `namespace db { class SqliteCppDb; }`.
- [ ] Move only `ChartData::save`, `BmsResult::save`, `BmsGaugeHistory::save`, and `BmsScore::save` into the four `persistence/*.cpp` files. Native `RhythmGame_lib` compiles them; the later portable web target does not. Keep constructors, getters, trace/result creation, and JSON conversion in the original portable translation units.
- [ ] Remove the unnecessary `sounds/SoundBuffer.h` include from `BmsNotesData.cpp`.
- [ ] Add the byte overload and Emscripten `QFile` acquisition branch.
- [ ] Run:

```powershell
cmake --build build --target RhythmGame_test --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo -R ChartDataFactory --output-on-failure
```

- [ ] Run portable-header dependency checks:

```powershell
rg -n "Vars\.h|SQLiteCpp|MiniaudioBackend|llfio|wil/resource" `
  src/resource_managers/ChartPlayOptions.h `
  src/resource_managers/ChartPlayConfig.h `
  src/gameplay_logic/ChartData.h `
  src/gameplay_logic/BmsResult.h `
  src/gameplay_logic/BmsGaugeHistory.h `
  src/gameplay_logic/BmsScore.h `
  src/gameplay_logic/BmsLiveScore.h `
  src/support/GeneratePermutation.h
```

Expected result: no match.

- [ ] Commit:

```text
refactor: make production chart loading portable
```

---

### Task 2: Preserve Exact Scheduled Sound Timestamps

**Files:**

- Modify: `src/sounds/Sound.h`
- Modify: `src/gameplay_logic/rules/HitRules.cpp`
- Modify: `src/gameplay_logic/BmsGameReferee.h`
- Modify: `src/gameplay_logic/BmsGameReferee.cpp`
- Test: `test/gameplay_logic/ScheduledSound.test.cpp`
- Modify: `test/CMakeLists.txt`

**Interface:**

```cpp
class Sound
{
  public:
    virtual ~Sound() = default;
    virtual void play() = 0;
    virtual void playAt(std::chrono::nanoseconds chartTime) { play(); }
    virtual void stop() = 0;
    virtual void stopAt(std::chrono::nanoseconds chartTime) { stop(); }
    // existing volume and state API remains unchanged
};
```

`playAt` and `stopAt` are chart-relative, use `std::chrono::nanoseconds`, and default to `play()`/`stop()` so all native sound implementations remain source-compatible.

Add an explicit one-shot BGM policy:

```cpp
class BmsGameReferee
{
  public:
    void preScheduleBgm();
};
```

`preScheduleBgm()` calls `playAt(authoredTimestamp)` for every BGM exactly once and marks the BGM stream externally scheduled. Subsequent `update()` calls advance/consume BGM cursors without playing those sounds again. Native `ChartRunner` never calls this method and retains schedule-on-advance behavior. The web gameplay worker calls it once after the AudioContext establishes `chartStartFrame` and before countdown advancement begins.

- [ ] Add a `RecordingSound` test double that records `playAt`/`stopAt` values and counts legacy `play()`/`stop()` calls.
- [ ] Build a minimal `HitRules`/`BmsGameReferee` fixture with a normal note, LN begin/end, mine sound, and BGM.
- [ ] Assert a press keysound receives the original press timestamp, release retains current no-play semantics, a mine sound receives the collision timestamp, and BGM receives its authored note timestamp rather than the later `update()` timestamp.
- [ ] Assert an LN miss calls `stopAt` with the exact authored miss/judgement boundary rather than the later `update()` timestamp.
- [ ] Assert `preScheduleBgm()` schedules each authored event once, a second call is idempotent, and later `update()` calls do not double-trigger BGM.
- [ ] Assert the default native path that never calls `preScheduleBgm()` still triggers BGM during `update()`.
- [ ] Assert a subclass that overrides only `play()` still receives immediate native playback through default `playAt`.
- [ ] Run the focused test and capture the expected failure before editing production code.
- [ ] Make `Sound.h` a backend-neutral interface: include `<chrono>` only, and remove its unused `QObject`, `spdlog`, and `MiniaudioBackend.h` includes.
- [ ] Replace all judgement-triggered `Sound::play()` calls in `HitRules.cpp` with `playAt(offsetFromStart)` using the exact occurrence supplied to that rule.
- [ ] Replace timing-driven `Sound::stop()` calls in `HitRules.cpp` with `stopAt(exactBoundary)`; do not add a release keysound where production currently has none.
- [ ] Replace BGM `Sound::play()` in `BmsGameReferee::update()` with `playAt(bgm.first)`.
- [ ] Preserve the referee's current suppression behavior after `lastUpdate`.
- [ ] Run:

```powershell
cmake --build build --target RhythmGame_test --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo -R ScheduledSound --output-on-failure
ctest --test-dir build -C RelWithDebInfo -R "BmsChart|InputTranslator" --output-on-failure
```

- [ ] Commit:

```text
refactor: carry chart time into sound playback
```

---

### Task 3: Extract the Production Single-Player Builder

**Files:**

- Create: `src/gameplay_logic/SinglePlayerChartBuilder.h`
- Create: `src/gameplay_logic/SinglePlayerChartBuilder.cpp`
- Modify: `src/resource_managers/ChartFactory.cpp`
- Modify: `CMakeLists.txt`
- Test: `test/gameplay_logic/SinglePlayerChartBuilder.test.cpp`
- Modify: `test/CMakeLists.txt`

**Interfaces:**

```cpp
namespace gameplay_logic {

struct PlayerChartBuildOptions
{
    resource_managers::NoteOrderAlgorithm noteOrderP1{
      resource_managers::NoteOrderAlgorithm::Normal };
    resource_managers::NoteOrderAlgorithm noteOrderP2{
      resource_managers::NoteOrderAlgorithm::Normal };
    resource_managers::DpOptions dpOptions{
      resource_managers::DpOptions::Off };
    std::uint64_t randomSeed{};
    bool usePre130{};
};

struct PlayerChartBuildResult
{
    std::unique_ptr<BmsNotes> notes;
    std::unique_ptr<GameplayState> state;
    std::array<support::ShuffleResult, 2> shuffleResults;
    std::array<std::vector<charts::BmsNotesData::Note>,
               charts::BmsNotesData::columnNumber> rawNotes;
    resource_managers::DpOptions effectiveDpOptions;
    ChartData::Keymode effectiveKeymode;

    [[nodiscard]] auto storedPermutation() const -> QList<int>;
    [[nodiscard]] auto storedSeed() const -> std::uint64_t;
};

auto buildPlayerChart(
  const charts::BmsNotesData& notesData,
  const ChartData& chartData,
  const PlayerChartBuildOptions& options) -> PlayerChartBuildResult;

}
```

Move the production `RandomizedData` lane/state portion, column selection, DP flip/battle handling, legacy pre-1.3.0 5K behavior, note-order permutation, and effective stored-seed logic currently private to `ChartFactory.cpp` into this module. Score construction stays with the caller because desktop replay/profile/gauge ownership differs from the headless core. `ChartFactory::createChart()` calls `buildPlayerChart()` independently for player 1 and optional player 2, then continues its desktop-only Profile/BGA/audio/ChartRunner wiring. The headless core calls the same function for one player.

- [ ] Add table-driven tests for 7K SP Normal, Mirror, seeded Random, S-Random, DP Flip, LR2 Flip, Battle constraints, pre-1.3.0 5K replay behavior, and optional player 2 with DP forced Off.
- [ ] Include normal, invisible, mine, LN begin/end, scratch, BPM, scroll, BGM, and bar-line fixture data.
- [ ] Before extraction, lock current behavior into exact fixture expectations for keymode, lane contents, both `ShuffleResult` values, effective stored seed, stored permutation, BPM/scroll data, and note counts. After extraction, both `ChartFactory` players and direct builder tests must satisfy those same expectations.
- [ ] Run the focused test before implementation and capture the expected compile failure.
- [ ] Extract the algorithm without changing permutation generation or ChartFactory public API.
- [ ] Make `ChartFactory.cpp` delegate to `buildPlayerChart` for player 1 and optional player 2, and construct `BmsLiveScore` from `effectiveDpOptions`, `effectiveKeymode`, `storedPermutation()`, and `storedSeed()` exactly as before.
- [ ] Verify there is exactly one implementation of column randomization:

```powershell
rg -n "struct RandomizedData|getComponentsForPlayer|applyOrder" `
  src/resource_managers/ChartFactory.cpp `
  src/gameplay_logic/SinglePlayerChartBuilder.cpp
```

Expected result: implementation definitions only in `SinglePlayerChartBuilder.cpp`.

- [ ] Run:

```powershell
cmake --build build --target RhythmGame_test --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo -R "SinglePlayerChartBuilder|GeneratePermutation|ChartPlayConfig" --output-on-failure
```

- [ ] Commit:

```text
refactor: extract production single-player chart builder
```

---

### Task 4: Add a Deterministic Headless Gameplay Core

**Files:**

- Create: `src/gameplay_logic/SinglePlayerGameplayCore.h`
- Create: `src/gameplay_logic/SinglePlayerGameplayCore.cpp`
- Create: `src/gameplay_logic/GameplayTrace.h`
- Create: `src/gameplay_logic/GameplayTrace.cpp`
- Modify: `src/gameplay_logic/BmsLiveScore.h`
- Modify: `src/gameplay_logic/BmsLiveScore.cpp`
- Modify: `CMakeLists.txt`
- Create: `tools/web-playtest-native/TraceRunner.cpp`
- Test: `test/gameplay_logic/SinglePlayerGameplayCore.test.cpp`
- Modify: `test/CMakeLists.txt`
- Fixture: `testOnlyAssets/webPlaytest/core-fixture.bms`

**Interfaces:**

```cpp
namespace gameplay_logic {

enum class GameplayKeyAction { Press, Release };

struct GameplayCoreConfig
{
    resource_managers::ChartPlayConfig play;
    std::int64_t savedTimestampSeconds;
    QString scoreGuid;
    double maxHitValue;
};

struct GameplaySnapshot
{
    struct VisibleNote
    {
        std::uint32_t stableId;
        std::uint8_t column;
        charts::BmsNotesData::NoteType type;
        std::int64_t chartTimeNs;
        double beatPosition;
        bool removed;
        bool holding;
    };

    std::int64_t chartTimeNs;
    double beatPosition;
    double scrollPosition;
    double points;
    double maxPointsNow;
    double gauge;
    int combo;
    int maxCombo;
    int mineHits;
    std::optional<Judgement> latestJudgement;
    std::optional<std::int64_t> latestDeviationNs;
    std::array<bool, charts::BmsNotesData::columnNumber> pressedColumns;
    std::vector<VisibleNote> visibleNotes;
    bool finished;
};

class SinglePlayerGameplayCore
{
  public:
    static auto create(std::string_view chartBytes,
                       const std::filesystem::path& logicalChartPath,
                       GameplayCoreConfig config,
                       std::unordered_map<std::uint64_t,
                         std::shared_ptr<sounds::Sound>> sounds)
      -> std::unique_ptr<SinglePlayerGameplayCore>;

    void advanceTo(std::chrono::nanoseconds chartTime);
    void passKey(input::BmsKey key,
                 GameplayKeyAction action,
                 std::chrono::nanoseconds chartTime);
    void preScheduleBgm();
    [[nodiscard]] auto snapshot() const -> GameplaySnapshot;
    [[nodiscard]] auto finishTrace() const -> QByteArray;
};

}
```

Construction uses `ChartDataFactory`, `buildPlayerChart`, LR2 NORMAL gauge, `Lr2TimingWindows`, `Lr2HitValues`, `HitRules`, `BmsLiveScore`, and `BmsGameReferee`. It does not use `ChartRunner`, `ChartFactory`, `Profile`, SQLite, BGA, `AudioEngine`, `Vars`, or QML.

`visibleNotes` contains only notes intersecting the configured render window around the current chart position. Stable IDs, removed/holding state, pressed columns, and latest judgement are captured on the gameplay worker, then copied as immutable data; QML never reads referee, score, or note objects directly.

`BmsLiveScore` already accepts saved timestamp and GUID. Ensure the deterministic constructor path always supplies fixed nonzero values so `finishTrace()` cannot consult wall-clock time or generate a UUID. Trace JSON uses fixed key ordering and integer nanoseconds; it includes chart hashes, random sequence, permutation, every input, every judgement, score totals, gauge samples, and result metadata.

- [ ] Add a compact fixture covering normal note, early/late judgement, LN press/release, miss, mine, scratch, BGM, BPM change, stop, scroll change, and deterministic `#RANDOM`.
- [ ] Add a fake sound bank that records scheduled timestamps.
- [ ] Write one canonical input sequence with fixed random choices, seed, timestamp, and GUID.
- [ ] Assert exact snapshots at countdown, first note, LN hold, miss boundary, BPM change, and result.
- [ ] Assert snapshots expose stable visible-note IDs, current removed/holding state, pressed-column state, latest judgement, and latest deviation without leaking mutable production objects.
- [ ] Assert `preScheduleBgm()` is one-shot and produces no duplicate when the core later advances through every BGM timestamp.
- [ ] Assert `finishTrace()` is byte-identical across two runs and matches an inline canonical JSON string.
- [ ] Assert inputs passed out of timestamp order fail with `std::invalid_argument`.
- [ ] Add native executable target `RhythmGame_web_playtest_trace` using `TraceRunner.cpp`; it accepts `--chart`, `--input`, and `--output`, runs the deterministic headless core, and writes the exact `finishTrace()` bytes. It must not construct the desktop application object graph.
- [ ] Add a portable-target source contract that fails if core sources include or link `SQLiteCpp`, `Vars`, `MiniaudioBackend`, `AudioEngine`, `Profile`, `ChartRunner`, BGA, Qt Multimedia, LLFIO, or WIL.
- [ ] Run failing tests before implementation.
- [ ] Implement the core and deterministic trace.
- [ ] Run:

```powershell
cmake --build build --target RhythmGame_test --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo -R "SinglePlayerGameplayCore|ScheduledSound|SinglePlayerChartBuilder" --output-on-failure
cmake --build build --target RhythmGame_web_playtest_trace --config RelWithDebInfo
```

- [ ] Commit:

```text
feat: add deterministic single-player gameplay core
```

---

### Task 5: Add Realtime-Safe PCM, Commands, and Asset Resolution

**Files:**

- Create: `src/web_playtest/audio/AudioCommand.h`
- Create: `src/web_playtest/audio/SpscRing.h`
- Create: `src/web_playtest/audio/PcmSoundBank.h`
- Create: `src/web_playtest/audio/PcmSoundBank.cpp`
- Create: `src/web_playtest/audio/RealtimeMixer.h`
- Create: `src/web_playtest/audio/RealtimeMixer.cpp`
- Create: `src/web_playtest/audio/ScheduledPcmSound.h`
- Create: `src/web_playtest/audio/ScheduledPcmSound.cpp`
- Create: `src/resource_managers/BmsAssetResolver.h`
- Create: `src/resource_managers/BmsAssetResolver.cpp`
- Modify: `src/resource_managers/loadBmsSounds.cpp`
- Modify: `CMakeLists.txt`
- Test: `test/web_playtest/RealtimeMixer.test.cpp`
- Test: `test/resource_managers/BmsAssetResolver.test.cpp`
- Modify: `test/CMakeLists.txt`

**Interfaces:**

```cpp
namespace web_playtest {

enum class AudioCommandType : std::uint8_t { Start, Stop, SetMasterGain };

struct AudioCommand
{
    AudioCommandType type;
    std::uint64_t sequenceId;
    std::uint64_t sourceInputId;
    std::uint32_t soundId;
    std::uint64_t targetFrame;
    std::int64_t sourceEventMonotonicUs;
    std::int64_t publishedMonotonicUs;
    float value;
};

struct AudioAcknowledgement
{
    std::uint64_t sequenceId;
    std::uint64_t sourceInputId;
    std::uint64_t dequeuedFrame;
    std::uint64_t firstNonZeroFrame;
    std::uint32_t lateByFrames;
};

template<typename T, std::size_t Capacity>
class SpscRing
{
  public:
    [[nodiscard]] auto tryPush(const T& value) noexcept -> bool;
    [[nodiscard]] auto tryPop(T& value) noexcept -> bool;
};

struct PcmClip
{
    std::vector<float> interleavedStereo;
    std::uint32_t sampleRate;
};

class RealtimeMixer
{
  public:
    void render(float* left, float* right, std::uint32_t frameCount) noexcept;
    [[nodiscard]] auto renderedFrames() const noexcept -> std::uint64_t;
};

}
```

`ScheduledPcmSound::playAt(chartTime)` converts the chart-relative nanosecond timestamp into a target output frame using a fixed chart-start frame and sample rate, then pushes a `Start` command. `stopAt(chartTime)` pushes a timestamped `Stop` command. A retrigger of the same sound ID stops/restarts that sound's existing voice to match `NormalSound`.

The gameplay-to-worklet command ring and worklet-to-gameplay acknowledgement ring are both preallocated SPSC queues. Every input-derived command carries the originating input ID and monotonic timestamps. The mixer acknowledges command dequeue, first non-zero output frame, and target-frame lateness without logging or allocating; the gameplay worker drains acknowledgements into bounded telemetry records.

`BmsAssetResolver` takes chart directory entries once, builds a lowercase relative-path index, and resolves in this order: declared path, `.wav`, `.flac`, `.ogg`, `.mp3`, with case-insensitive fallback. Both native `loadBmsSounds.cpp` and the web loader call this shared resolver.

- [ ] Add lock-free ring wraparound, full/empty, and producer/consumer ordering tests.
- [ ] Add sample-accurate mixer tests for future command silence, mid-buffer start, overlapping different sounds, same-sound retrigger, stop, clip end, gain, and rendered-frame monotonicity.
- [ ] Add acknowledgement tests proving input/command correlation, exact dequeue frame, exact first non-zero frame, and late-frame accounting for commands that arrive after their target.
- [ ] Add asset resolver tests using `testOnlyAssets/bmsFallbackExtensions` plus mixed-case temporary fixture names.
- [ ] Assert a declared `sample.wav` resolves to `sample.ogg` when that is the only shipped asset.
- [ ] Assert `..`/absolute/root-escaping references are rejected, duplicate case-folded relative names produce a deterministic terminal diagnostic, and CP932-decoded/non-ASCII names resolve through the indexed chart directory.
- [ ] Assert resolution never probes the ambient filesystem outside the pre-indexed chart root.
- [ ] Run failing focused tests.
- [ ] Implement the ring with acquire/release atomics and no dynamic allocation in `tryPush`/`tryPop`.
- [ ] Implement the mixer with all voice storage allocated before render begins.
- [ ] Extract native extension/case fallback into `BmsAssetResolver`.
- [ ] Run:

```powershell
cmake --build build --target RhythmGame_test --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo -R "RealtimeMixer|BmsAssetResolver|loadBmsSounds" --output-on-failure
```

- [ ] Commit:

```text
feat: add realtime web playtest audio core
```

---

### Task 6: Create the Isolated Web Playtest Build and Private Chart Package

**Files:**

- Create: `tools/web-playtest/CMakeLists.txt`
- Create: `tools/web-playtest/CMakePresets.json`
- Create: `tools/web-playtest/vcpkg.json`
- Create: `tools/web-playtest/cmake/WebPlaytestInputDigest.cpp.in`
- Create: `tools/web-playtest/cmake/WebPlaytestChartManifest.json.in`
- Create: `tools/web-playtest/src/WebPlaytestChartInstaller.h`
- Create: `tools/web-playtest/src/WebPlaytestChartInstaller.cpp`
- Create: `tools/web-playtest/src/main.cpp`
- Create: `tools/web-playtest/qml/Main.qml`
- Create: `tools/web-playtest/input-manifest.txt`
- Create: `tools/web-playtest/tests/test_web_playtest_source_contract.py`
- Modify: `tools/wasm-probe/scripts/Invoke-WithToolchains.ps1`
- Modify: `.gitignore`

The target remains internally named `RhythmGameWasmProbe` so the already strict packager and HTTPS server can be reused without weakening their ten-role artifact contract.

**CMake inputs:**

```cmake
set(RG_WEB_PLAYTEST_CHART_DIR "" CACHE PATH
    "Directory of the private chart package embedded in this playtest build")
set(RG_WEB_PLAYTEST_CHART_RELATIVE_PATH "" CACHE STRING
    "BMS/BME/BML/PMS path relative to RG_WEB_PLAYTEST_CHART_DIR")
```

Configuration must canonicalize both paths, reject a selected chart outside the root, reject missing files/reparse points/symlinks, accept only `.bms`, `.bme`, `.bml`, or `.pms`, and add all regular files under the chart root through:

```cmake
qt_add_resources(RhythmGameWasmProbe "web_playtest_chart"
    PREFIX "/web-playtest/chart"
    BASE "${RG_WEB_PLAYTEST_CHART_DIR}"
    FILES ${RG_WEB_PLAYTEST_CHART_FILES})
```

At runtime, `WebPlaytestChartInstaller::install()` recursively copies those QRC files to `/playtest/chart`, then returns `/playtest/chart/<configured-relative-path>`.

At configure time, generate a host-path-free `web-playtest-chart-manifest.json` containing:

```json
{
  "schema": 1,
  "selectedVirtualPath": "/playtest/chart/Dstorv_act1_evo.bme",
  "selectedChartSha256": "<64 lowercase hex>",
  "files": [
    {
      "virtualPath": "/playtest/chart/<relative path>",
      "sha256": "<64 lowercase hex>",
      "size": 123
    }
  ]
}
```

Sort files by normalized relative path. Embed this manifest as a resource and have `WebPlaytestChartInstaller` verify each materialized file against it before returning the selected virtual path. No generated source, compiled string, runtime manifest, JS, Wasm, or packaged asset may contain `RG_WEB_PLAYTEST_CHART_DIR`.

Task 6 supplies a minimal boot stub in `src/main.cpp` and `qml/Main.qml` that installs the chart, shows the selected MEMFS path, and exits cleanly on initialization error. Task 8 replaces that stub with the gameplay runtime; this makes the target linkable and testable before the AudioWorklet and worker exist.

- [ ] Add source-contract tests for all required files, exact target name, required cache variables, QRC packaging, path containment checks, Wasm flags, sorted/unique input manifest, and independent `.web-playtest-vcpkg` state.
- [ ] Add tests for the host-path-free chart manifest schema, deterministic ordering/digests, materialization verification, and absence of the configured host root from generated sources and packaged runtime artifacts.
- [ ] Add a CMake script-mode test that proves missing variables, parent traversal, absolute relative paths, unsupported extension, symlink/reparse input, and selected path outside root all fail.
- [ ] Run:

```powershell
python -m unittest -v tools.web-playtest.tests.test_web_playtest_source_contract
```

and capture the expected failure before adding the project.

- [ ] Copy the authenticated compiler/linker launcher and exact-toolchain validation from `tools/wasm-probe`.
- [ ] Preserve:

```text
-pthread
-fwasm-exceptions
-sSUPPORT_LONGJMP=wasm
-sJSPI
-sAUDIO_WORKLET=1
-sWASM_WORKERS=1
-sPTHREAD_POOL_SIZE=4
-sPTHREAD_POOL_SIZE_STRICT=2
-sALLOW_BLOCKING_ON_MAIN_THREAD=0
-sDYNAMIC_EXECUTION=0
-sEMBIND_AOT=1
-sINITIAL_MEMORY=268435456
-sALLOW_MEMORY_GROWTH=1
-sMAXIMUM_MEMORY=1073741824
```

The initial heap is 256 MiB, growth is permitted only while the chart is installing/decoding, and the explicit maximum is 1 GiB. Decode and resample one clip at a time, release compressed and source-rate scratch storage after each clip, capture heap size at `Ready`, and require it to remain unchanged through countdown/gameplay/retry. The real-chart qualification records peak heap and fails if `Ready` exceeds 512 MiB.

- [ ] Set `NO_WASM_DEFAULT_FILES TRUE`.
- [ ] Link only Qt Core/Quick/QuickControls2 plus the portable production sources and web-playtest sources. Do not link root `RhythmGame_lib`.
- [ ] Add `boost-headers`, `boost-icl`, `fmt`, `foonathan-lexy`, `libiconv`, `magic-enum`, `spdlog`, `stb`, `zstd`, `qtbase`, `qtdeclarative`, and `qtshadertools` to the web manifest. Use `stb_vorbis` for the initial OGG decoder path. Do not add SQLiteCpp, miniaudio, libsndfile, SDL, QtKeychain, LLFIO, WIL, OpenImageIO, or the native scanner graph.
- [ ] Generalize `Invoke-WithToolchains.ps1` to allow `tools/web-playtest/build/wasm-release`; only `tools/wasm-probe/build/wasm-release` triggers the Gate 1B qualification closure.
- [ ] Keep chart files and their absolute root out of `input-manifest.txt`.
- [ ] Scan generated C++ plus the packaged JS/Wasm/manifests for the UTF-8 and UTF-16 forms of `RG_WEB_PLAYTEST_CHART_DIR`; fail the build if found.
- [ ] Run focused source-contract tests plus the existing 62 probe contract tests:

```powershell
python -m unittest -v `
  tools.web-playtest.tests.test_web_playtest_source_contract `
  tools.wasm-probe.tests.test_probe_source_contract `
  tools.wasm-probe.tests.test_gate1b_source_contract
```

- [ ] Configure with the real stress chart:

```powershell
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
  -VcpkgStateRoot .web-playtest-vcpkg -- `
  cmake --preset wasm-release -S tools/web-playtest `
  -DRG_WEB_PLAYTEST_CHART_DIR='T:\BMSTEST-DstorvEvo\Dstorv' `
  -DRG_WEB_PLAYTEST_CHART_RELATIVE_PATH='Dstorv_act1_evo.bme'
```

- [ ] Commit:

```text
build: add isolated web playtest target
```

---

### Task 7: Connect the Emscripten Wasm AudioWorklet

**Files:**

- Create: `src/web_playtest/audio/EmscriptenAudioWorklet.h`
- Create: `src/web_playtest/audio/EmscriptenAudioWorklet.cpp`
- Create: `src/web_playtest/audio/BrowserAudioClock.h`
- Create: `src/web_playtest/audio/BrowserAudioClock.cpp`
- Create: `tools/web-playtest/tests/test_audio_worklet_source_contract.py`
- Modify: `tools/web-playtest/CMakeLists.txt`
- Modify: `tools/web-playtest/input-manifest.txt`

Use Emscripten's C WebAudio API:

- `emscripten_create_audio_context`
- `emscripten_start_wasm_audio_worklet_thread_async`
- `emscripten_create_wasm_audio_worklet_processor_async`
- `emscripten_create_wasm_audio_worklet_node`
- `emscripten_audio_node_connect`
- `emscripten_resume_audio_context_sync`
- `emscripten_audio_context_state`

The context is created suspended during load. Worklet stack, both SPSC rings, PCM bank, mixer, voice slots, and output buffers are allocated before the Start control is enabled. The trusted Start click calls `emscripten_resume_audio_context_sync()` only to initiate resume. An asynchronous state machine then waits for `AUDIO_CONTEXT_STATE_RUNNING` and at least one advancing worklet quantum before atomically sampling browser monotonic time and rendered frame to establish:

```text
chartStartFrame = currentRenderedFrame + countdownFrames
chartTimeNs = browserEventMonotonicNs - chartStartBrowserMonotonicNs
targetFrame = chartStartFrame + round(chartTimeNs * sampleRate / 1e9)
```

`browserEventMonotonicNs` and `chartStartBrowserMonotonicNs` are both derived from Chromium's `performance.now()` time origin. Do not add `performance.timeOrigin` or mix epoch time, Qt elapsed time, wall-clock time, and rendered-frame time.

Emscripten 4.0.7 has no C sample-rate getter. Add a narrowly scoped `EM_JS` bridge that returns `emscriptenGetAudioObject(audioContextHandle).sampleRate`; validate it is finite and in 8,000–192,000 Hz before decoding. Do not assume 44.1 or 48 kHz.

- [ ] Add source-contract tests proving use of the Wasm AudioWorklet API and forbidding `ScriptProcessor`, `emscripten_sleep`, Asyncify, `std::mutex`, Qt, logging, filesystem, allocation, and exceptions in the render callback translation unit.
- [ ] Assert the callback only reads/writes preallocated POD state and calls `RealtimeMixer::render`.
- [ ] Assert render quantum size comes from the callback parameters rather than a hardcoded 128.
- [ ] Assert context sample rate is read from the created context and PCM is resampled before Start when source rate differs.
- [ ] Assert a resume request cannot establish countdown anchors until the context reports Running and `renderedFrames()` has advanced by at least one quantum; suspend/resume delay must not alter the browser-time/audio-frame mapping.
- [ ] Run the contract test first and capture failure.
- [ ] Implement async worklet initialization with explicit error states propagated to the main runtime.
- [ ] Initiate resume synchronously from the trusted Start click, then complete startup asynchronously after Running plus one advancing quantum.
- [ ] Read the actual sample rate through the tested `EM_JS` bridge before decode/resample.
- [ ] Expose atomics for rendered frame, underruns, dropped commands, active voices, last render duration, late commands, last acknowledged sequence, first-nonzero frame, current heap size, and Ready heap size.
- [ ] Drain the preallocated acknowledgement ring on the gameplay worker and retain a bounded correlation report that can be downloaded by the browser driver.
- [ ] Build:

```powershell
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
  -VcpkgStateRoot .web-playtest-vcpkg -- `
  cmake --build tools/web-playtest/build/wasm-release `
  --target RhythmGameWasmProbe --verbose
```

- [ ] Inspect the final link command:

```powershell
rg -n "AUDIO_WORKLET=1|WASM_WORKERS=1|ALLOW_BLOCKING_ON_MAIN_THREAD=0|ASYNCIFY|ScriptProcessor" `
  tools/web-playtest/build/wasm-release/compile_commands.json `
  tools/web-playtest/build/wasm-release/build.ninja
```

Expected: all required flags present; `ASYNCIFY` and `ScriptProcessor` absent.

- [ ] Commit:

```text
feat: drive playtest audio from a Wasm worklet
```

---

### Task 8: Add Timestamped Browser Input, Gameplay Worker, and Snapshot Shell

**Files:**

- Create: `src/web_playtest/WebPlaytestRuntime.h`
- Create: `src/web_playtest/WebPlaytestRuntime.cpp`
- Create: `src/web_playtest/GameplayWorker.h`
- Create: `src/web_playtest/GameplayWorker.cpp`
- Create: `src/web_playtest/InputEvent.h`
- Create: `src/web_playtest/WebPlaytestSnapshot.h`
- Create: `src/web_playtest/WebPlaytestSnapshot.cpp`
- Modify: `tools/web-playtest/src/main.cpp`
- Modify: `tools/web-playtest/qml/Main.qml`
- Create: `tools/web-playtest/browser/web/bootstrap.mjs`
- Create: `tools/web-playtest/browser/web/playtest.css`
- Create: `tools/web-playtest/tests/test_runtime_source_contract.py`
- Modify: `tools/web-playtest/CMakeLists.txt`
- Modify: `tools/web-playtest/input-manifest.txt`

**Fixed 7K mapping by `KeyboardEvent.code`:**

| Browser code | BMS key |
|---|---|
| `ShiftLeft`, `ControlLeft` | `Col1sUp`, `Col1sDown` respectively |
| `KeyS` | `Col11` |
| `KeyD` | `Col12` |
| `KeyF` | `Col13` |
| `Space` | `Col14` |
| `KeyJ` | `Col15` |
| `KeyK` | `Col16` |
| `KeyL` | `Col17` |
| `Enter` | Start/resume |
| `Escape` | Abort |

`bootstrap.mjs` installs capture-phase `keydown`/`keyup` listeners before Qt loads, suppresses repeat presses, calls `preventDefault()` only for mapped gameplay keys, keeps a pressed-code set, synthesizes releases on `blur`/`visibilitychange`, and forwards Chromium's `event.timeStamp` as integer microseconds in the same time-origin domain as `performance.now()`. C++ converts it once to the common monotonic chart clock.

The gameplay pthread owns `SinglePlayerGameplayCore` and consumes input/advance commands. The main thread sees only a double-buffered immutable `WebPlaytestSnapshot` copied at frame refresh. QML calls `snapshot.refresh()` from `FrameAnimation`.

```cpp
struct InputEvent
{
    std::uint64_t sequenceId;
    input::BmsKey key;
    gameplay_logic::GameplayKeyAction action;
    std::int64_t browserMonotonicUs;
};
```

The input sequence ID is copied into every input-derived `AudioCommand` and returned by `AudioAcknowledgement`, providing a complete browser-event → worker-consumption → command-publication → first-nonzero-output correlation chain.

- [ ] Add source-contract tests for every fixed mapping, repeat suppression, release synthesis, integer timestamp forwarding, mapped-key-only default prevention, and no direct QML call into referee/score/audio.
- [ ] Add native C++ tests for ordered input queue behavior, press/release deduplication, pre-start input rejection, monotonic clock conversion, worker shutdown, and snapshot generation.
- [ ] Run failing tests before implementation.
- [ ] Implement runtime phases: `InstallingChart`, `Decoding`, `Ready`, `Countdown`, `Playing`, `Finished`, `Aborted`, `Error`.
- [ ] Decode all resolved OGG assets and resample them to the actual AudioContext rate before phase `Ready`.
- [ ] Decode/resample one asset at a time, release compressed/source-rate scratch buffers after insertion into `PcmSoundBank`, capture heap size on entering `Ready`, and treat any later heap growth as a qualification failure.
- [ ] After the Start click establishes `chartStartFrame`, send a `PreScheduleBgm` command to the gameplay worker. It calls `SinglePlayerGameplayCore::preScheduleBgm()` exactly once before countdown advancement; the referee then consumes BGM timestamps without replaying them. Preserve judgement-triggered keysound scheduling.
- [ ] Keep Qt/QML objects on the main thread and gameplay core on one worker only.
- [ ] Build a minimal qualification screen showing:
  - chart title/artist/BPM and current phase;
  - loading asset count and terminal error;
  - countdown;
  - seven lanes plus scratch with note positions derived from snapshots;
  - combo, latest judgement, score, NORMAL gauge, and elapsed time;
  - input-to-command and command-to-render latency telemetry;
  - underruns, dropped commands, active voices, and frame-time percentile;
  - Start/Retry and Abort controls plus key legend.
- [ ] Use unversioned QML imports, `QtQuick.Controls.Basic`, `qsTr()` for user-visible strings, and no animated `Canvas`.
- [ ] Run:

```powershell
python -m unittest -v `
  tools.web-playtest.tests.test_runtime_source_contract `
  tools.web-playtest.tests.test_audio_worklet_source_contract

cmake --build build --target RhythmGame_test --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo -R "WebPlaytest|SinglePlayerGameplayCore" --output-on-failure
```

- [ ] Commit:

```text
feat: add browser gameplay runtime and playtest shell
```

---

### Task 9: Package and Launch the Strict Chromium Runtime

**Files:**

- Create: `tools/web-playtest/browser/web/RhythmGameWasmProbe.html.in`
- Create: `tools/web-playtest/browser/web/preflight-worker.mjs`
- Create: `tools/web-playtest/browser/manual-launch.mjs`
- Create: `tools/web-playtest/browser/test-playtest.mjs`
- Create: `tools/web-playtest/browser/playtest-driver.mjs`
- Create: `tools/web-playtest/tests/fixtures/canonical-input.json`
- Modify: `tools/web-playtest/CMakeLists.txt`
- Modify: `tools/web-playtest/input-manifest.txt`
- Modify: `tools/web-playtest/tests/test_web_playtest_source_contract.py`

Reuse `tools/wasm-probe/scripts/package_runtime_artifacts.py`, `tools/wasm-probe/browser/server/probe-server.mjs`, and its exact HTTPS/COOP/COEP behavior. Preserve generated artifact roles:

```text
audioWorklet
bootstrap
css
html
mainJs
media
preflightWorker
qtloader
wasm
wasmWorker
```

Retain post-build target names `RhythmGameWasmProbeRuntimePackage` and `RhythmGameWasmProbeRuntimeVerify`.

- [ ] Extend source-contract tests to prove the bootstrap uses `qtLoad`, maps Wasm/AudioWorklet/worker assets through `locateFile`, and contains no autoplay, COOP/COEP, origin, CORS, or browser-security bypass.
- [ ] Add `manual-launch.mjs` using `startProbeServer` and `launchExternalLifecycleBrowser`; it opens headed installed Chrome/Chromium, prints the strict local URL, and remains alive until Ctrl+C.
- [ ] Add automated Playwright coverage that:
  - loads over the strict HTTPS origin;
  - observes cross-origin isolation;
  - reaches `Ready`;
  - requires a trusted click to resume audio;
  - starts countdown;
  - injects mapped press/release pairs through real browser keyboard events;
  - observes combo/judgement/snapshot progression;
  - aborts and retries without reload;
  - reports zero uncaught exceptions, QML errors, dropped input events, and AudioWorklet initialization failures.
- [ ] Add `--input-sequence <json>` and `--trace-output <host-path>` to `playtest-driver.mjs`. The page exposes the completed canonical gameplay trace as bytes through a test-only browser bridge; Playwright retrieves those bytes and writes the requested host file. Browser Wasm never assumes it can write into the host runtime directory.
- [ ] Add negative server tests for unhashed assets, path traversal, missing manifest roles, non-HTTPS access, and missing isolation headers.
- [ ] Build/package:

```powershell
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
  -VcpkgStateRoot .web-playtest-vcpkg -- `
  cmake --build tools/web-playtest/build/wasm-release `
  --target RhythmGameWasmProbeRuntimePackage --verbose
```

- [ ] Run:

```powershell
node tools/web-playtest/browser/test-playtest.mjs `
  --runtime-directory tools/web-playtest/build/wasm-release/runtime
```

- [ ] Commit:

```text
test: package and exercise Chromium web playtest
```

---

### Task 10: Qualify the Real Dstorv Feel Build

**Files:**

- Create: `tools/web-playtest/README.md`
- Create: `docs/superpowers/evidence/emscripten-playable-slice.json`
- Create: `tools/web-playtest/scripts/compare_gameplay_traces.py`
- Create: `tools/web-playtest/tests/test_trace_equivalence.py`
- Modify: `tools/web-playtest/input-manifest.txt`

The local qualification input is:

```text
Chart root: T:\BMSTEST-DstorvEvo\Dstorv
Selected chart: Dstorv_act1_evo.bme
```

The README must explain the private chart arguments, configure/build/package commands, manual launch command, controls, expected loading behavior, how to open Chromium diagnostics, and which production features are deliberately preview-deferred rather than removed.

- [ ] Run all Python source-contract tests:

```powershell
python -m unittest -v `
  tools.web-playtest.tests.test_web_playtest_source_contract `
  tools.web-playtest.tests.test_audio_worklet_source_contract `
  tools.web-playtest.tests.test_runtime_source_contract `
  tools.wasm-probe.tests.test_probe_source_contract `
  tools.wasm-probe.tests.test_gate1b_source_contract
```

- [ ] Run focused native tests:

```powershell
cmake --build build --target RhythmGame_test --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo `
  -R "ChartDataFactory|ScheduledSound|SinglePlayerChartBuilder|SinglePlayerGameplayCore|RealtimeMixer|BmsAssetResolver|WebPlaytest" `
  --output-on-failure
```

- [ ] Reconfigure, build, and package the real Dstorv chart from a clean playtest build directory.
- [ ] Run the same canonical gameplay input sequence through the explicit native trace executable:

```powershell
build\bin\RelWithDebInfo\RhythmGame_web_playtest_trace.exe `
  --chart 'T:\BMSTEST-DstorvEvo\Dstorv\Dstorv_act1_evo.bme' `
  --input tools/web-playtest/tests/fixtures/canonical-input.json `
  --output tools/web-playtest/build/native-trace.json
```

- [ ] Run it through the Wasm worker and retrieve bytes through the browser driver:

```powershell
node tools/web-playtest/browser/playtest-driver.mjs `
  --runtime-directory tools/web-playtest/build/wasm-release/runtime `
  --input-sequence tools/web-playtest/tests/fixtures/canonical-input.json `
  --trace-output tools/web-playtest/build/wasm-trace.json
```
- [ ] Compare canonical JSON byte-for-byte with:

```powershell
python tools/web-playtest/scripts/compare_gameplay_traces.py `
  --native tools/web-playtest/build/native-trace.json `
  --wasm tools/web-playtest/build/wasm-trace.json
```

- [ ] Run automated Chromium play ten times in one browser process and ten fresh browser processes.
- [ ] Require on every run: chart reaches Ready, all 630 declared WAV IDs resolve to shipped OGG assets where applicable, no unsupported asset, no stuck key, no missed release, zero command drops, zero mixer underruns after countdown, clean console, result reached or explicit abort accepted, and retry succeeds.
- [ ] Require Ready heap at or below 512 MiB and exactly unchanged heap size from Ready through countdown, gameplay, result, abort, and retry.
- [ ] Capture latency evidence:
  - browser event timestamp to gameplay worker consumption;
  - worker consumption to audio command publication;
  - target frame to actual render frame;
  - `AudioContext.baseLatency` and `outputLatency` where supported;
  - p50/p95/p99 UI frame time;
  - peak active voices and render callback duration.
- [ ] Record exact toolchain, dependency, input-manifest, executable, Wasm, JS, AudioWorklet, worker, chart, and runtime manifest hashes in `emscripten-playable-slice.json`.
- [ ] Scan every packaged JS/JSON/manifest/text artifact directly and scan Wasm raw bytes for UTF-8 and UTF-16 forms of `T:\BMSTEST-DstorvEvo\Dstorv`; require no match.
- [ ] Run `git diff --check` and `git status --short`.
- [ ] Request two fresh adversarial reviews:
  1. gameplay parity/timestamp/audio scheduling review;
  2. Emscripten isolation/build reproducibility/browser-runtime review.
- [ ] Fix every accepted P0/P1/P2 finding and rerun the affected checks plus the complete focused suite.
- [ ] Manually launch headed Chromium for user handoff:

```powershell
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
  node tools/web-playtest/browser/manual-launch.mjs `
  --runtime-directory tools/web-playtest/build/wasm-release/runtime
```

- [ ] Commit:

```text
docs: qualify playable Emscripten slice
```

## Completion Gate

The slice is ready for the user's feel test only when all of the following are evidenced in the current checkout:

- A strict Chromium runtime loads the configured real chart and reaches `Ready`.
- A trusted click starts the AudioContext and countdown.
- Physical keyboard presses and releases are judged by the production referee with preserved timestamps.
- Keysounds and BGM are audible through the Wasm AudioWorklet with zero post-countdown underruns or dropped commands in the automated qualification.
- Production native and Wasm canonical gameplay traces are byte-identical.
- The real Dstorv chart resolves its `.wav` declarations to shipped `.ogg` files.
- Abort and retry work without a page reload.
- Existing Gate 1B source-contract tests still pass.
- No local chart asset or absolute local chart path is tracked by git.
- The final headed launch command and controls are documented for immediate user testing.

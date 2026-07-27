# Emscripten Playable Slice Design

**Date:** 2026-07-27

**Status:** Approved for implementation

**Product decision:** The web client may be released under GPLv3. Licensing
qualification is not a gate for this milestone.

## Outcome

Build the earliest Chromium artifact that can answer one question honestly:

> Does real RhythmGame BMS gameplay feel acceptable in a browser?

The artifact launches one configured chart directly and uses the production
chart parser, note model, judgement rules, scoring, gauge, replay event model,
and keysound semantics. It is not a timing animation or Web Audio click demo.

The first hand-testable build intentionally uses a small qualification shell
instead of the complete application navigation. This shell is another
executable over the shared production core, not a fork of the game. Desktop
targets and all existing source remain present. Browser capabilities that have
not reached the slice are reported as unavailable in this preview rather than
silently succeeding.

After the feel decision, the same browser runtime, chart workspace, input, and
audio adapters become the platform implementations used by the complete web
application.

## Milestone boundary

### Required

- Chromium desktop on Windows 11.
- HTTPS or a loopback secure context with cross-origin isolation.
- Qt 6.11.1 built for Emscripten 4.0.7.
- Native Wasm exceptions, pthreads, JSPI, WebGL2, Wasm AudioWorklet, and no
  main-thread blocking.
- One configure-time selected BMS/BME/BML chart directory packaged into the
  runtime and materialized at a stable virtual path.
- Direct launch after a trusted click enables audio.
- Production BMS parsing and chart construction.
- Production judgement, scoring, combo, gauge, and long-note behavior.
- Capture-phase physical keyboard input using `KeyboardEvent.code`.
- Input occurrence timestamps normalized to integer microseconds in the
  `performance.timeOrigin` domain.
- A dedicated gameplay worker and shared input/audio command rings.
- An owned Wasm AudioWorklet using the `AudioContext` sample rate.
- All referenced keysounds and BGM decoded before countdown.
- A minimal Qt Quick playfield displaying the real chart state, pressed lanes,
  judgement, combo, gauge, title, and live latency telemetry.
- Deterministic native-versus-Wasm trace comparison.
- A local launcher that keeps Chromium open for subjective testing.

### Explicitly deferred from this preview

- Full ContentFrame navigation and the production song-select screen.
- Persistent profiles, settings, scores, and replays.
- Login, Internet Ranking, tables, Arena, and server synchronization.
- Whole-library scanning and retained File System Access handles.
- WebHID, WebMIDI, and Gamepad input.
- BGA/video and the complete image/codec compatibility matrix.
- Custom, LR2, and Beatoraja gameplay themes.

These capabilities are not removed. They remain native functionality and web
port milestones. The preview shell must label them as outside the current
qualification boundary.

## Source integration

Before implementation, merge the current product branch into
`codex/emscripten-web`. This prevents the web work from preserving obsolete
Qt Interface Framework and direct pending-reply dependencies and keeps current
Arena, MIDI, cancellation, and theme work in the single codebase.

The qualified probe remains intact. Its pinned toolchain, triplet, vcpkg
overlays, browser server, Chromium installation, strict headers, and runtime
tests are the source of truth for the new target.

## Build structure

Add a web-playtest subtree rather than making the desktop dependency graph
cross-compile immediately:

```text
tools/web-playtest/
  CMakeLists.txt
  CMakePresets.json
  vcpkg.json
  cmake/
  src/
  qml/
  browser/
  tests/
```

The subtree links selected production sources into
`RhythmGameWebPlaytest`. It must not copy gameplay algorithms. CMake selects
portable production source files and browser adapters explicitly so desktop
QtKeychain, SQLite, SDL, LLFIO, OpenImageIO, and native device startup are not
part of this first link.

The target inherits the exact qualified runtime contract:

```text
-pthread
-fwasm-exceptions
-sSUPPORT_LONGJMP=wasm
-sJSPI
-sAUDIO_WORKLET=1
-sWASM_WORKERS=1
-sALLOW_BLOCKING_ON_MAIN_THREAD=0
```

Build input includes:

- `RHYTHMGAME_WEB_PLAYTEST_CHART`: host path to the selected chart file.
- the selected chart's parent directory, preloaded at `/playtest/chart`;
- a generated manifest naming the virtual chart path and content digest.

Host paths never enter committed runtime source or browser reports. Packaging
fails when the chart is outside its packaged directory, the extension is not
supported, or the manifest cannot reproduce the selected file.

The first local target chart is expected to be:

```text
T:\BMSTEST-DstorvEvo\Dstorv\Dstorv_act1_evo.bme
```

Its assets are local build inputs and are never committed.

## Runtime lifetime

The web entry point heap-owns a `PlaytestRuntime`, begins asynchronous startup,
and returns `0`. Destruction at page exit is not relied upon.

Startup phases are:

1. publish browser preflight state;
2. construct Qt Quick with WebGL2 and the basic render loop;
3. load the generated chart manifest;
4. parse chart metadata and enumerate required assets;
5. decode and resample assets in bounded workers;
6. show a trusted `Start playtest` control;
7. create or resume the interactive `AudioContext`;
8. initialize the AudioWorklet and shared rings;
9. start the gameplay worker and countdown;
10. publish immutable gameplay snapshots until result or abort.

Every failed phase produces a terminal, visible error with a stable code.
There is no fallback that claims the test is valid without the worker,
AudioWorklet, or required assets.

## Chart workspace

`PlaytestChartWorkspace` owns the stable virtual chart root and generated
manifest. For this milestone its implementation reads preloaded files from
Emscripten's virtual filesystem.

The production parser already accepts chart bytes. Replace LLFIO-only reads at
the selected production seam with a portable bounded byte source, retaining
the same decoder and random-sequence behavior.

Asset resolution retains existing BMS behavior:

- chart-relative paths;
- recursive and case-insensitive fallback where the current loader supports it;
- extension fallback;
- CP932 and non-ASCII chart names;
- duplicate-name diagnostics;
- no path escape outside the packaged root.

No filesystem work occurs after countdown starts.

## Input and common clock

Browser JavaScript captures `keydown` and `keyup` in the capture phase.
Mapped gameplay keys use `KeyboardEvent.code`; layout-dependent `key` and Qt
Wasm's empty native scan code do not identify controls.

Each edge contains:

```cpp
struct InputEdge {
    std::uint32_t code;
    std::uint8_t action;
    std::uint8_t reserved[3];
    std::int64_t occurrenceUs;
    std::uint64_t sequence;
};
```

The adapter ignores repeats, prevents browser defaults only for active mapped
controls, and synthesizes releases for every held control on blur, hidden
visibility, or capture loss.

The common clock is integer microseconds in the Window
`performance.timeOrigin` domain. Worker origins are normalized explicitly.
`Date.now()` is forbidden. The gameplay worker consumes occurrence timestamps;
render cadence never determines judgement time.

The first 7-key mapping is fixed for reproducible testing:

```text
Scratch left/right: ShiftLeft / ControlLeft
Keys 1-7: KeyS / KeyD / KeyF / Space / KeyJ / KeyK / KeyL
Start: Enter
Abort: Escape
```

Bindings are displayed before start.

## Gameplay session

`PlaytestSession` owns the production referee, score, gauge, and chart clock on
a dedicated pthread. It receives normalized `InputEdge` values and emits:

- production input events to the existing chart runner/referee seam;
- frame-indexed sound commands;
- immutable presentation snapshots;
- deterministic trace records.

The existing 1 ms `QTimer` may wake presentation code but is not clock
authority. BGM commands are scheduled against the audio-frame timeline rather
than started when a timer happens to observe their timestamp.

The slice must not create a substitute judgement implementation. If a
production class needs a small interface extraction to run without Profile or
database services, native and web must call the same extracted implementation,
and native regression tests must cover the move.

## Audio

Use Emscripten's Wasm AudioWorklet API directly. Do not use Qt Multimedia,
`QSoundEffect`, `QAudioSink`, miniaudio's ScriptProcessor fallback, or
miniaudio 0.11.25's Asyncify-dependent AudioWorklet initialization for the
acceptability artifact.

`PlaytestAudio` owns:

- an `AudioContext({latencyHint: "interactive"})` created from the start click;
- the context's actual sample rate;
- preallocated shared PCM storage;
- fixed voice and command pools;
- a single-producer/single-consumer command ring;
- frame mapping between the common clock and the audio sample timeline;
- underrun, late-command, callback-time, and render-load counters.

The callback performs no allocation, decoding, filesystem access, Qt calls,
mutex acquisition, network access, or logging.

Keysound commands originate from the production hit-rule path. BGM commands
carry scheduled sample frames. The AudioWorklet reports the first rendered
non-zero frame for latency measurement.

## Presentation

The QML shell is deliberately small and uses only immutable snapshots. It
contains:

- browser/runtime readiness;
- chart title and BPM;
- seven lanes plus scratch;
- note positions from production chart state;
- pressed-lane feedback;
- latest judgement and combo;
- gauge value;
- countdown/result/abort state;
- current and percentile latency counters;
- explicit preview limitations.

QML neither judges input nor schedules audio. FrameAnimation requests the
latest snapshot; it does not advance gameplay time.

## Telemetry

Record monotonic timestamps for:

- DOM occurrence to ring write;
- ring write to session dequeue;
- dequeue to judgement;
- judgement to audio command publication;
- publication to AudioWorklet dequeue;
- dequeue to first non-zero generated sample;
- DOM occurrence to visible pressed-lane snapshot;
- AudioWorklet callback duration, late commands, and underruns;
- Qt render-frame interval and main-thread stalls.

The shell displays rolling p50, p95, and p99 values and can download a bounded
JSON report after a run. Reports contain the generated chart digest, build
identity, browser identity, runtime capabilities, sample rate, and aggregate
timings, never the host chart path or song assets.

## Verification

### Source and build contracts

- Web target consumes the qualified Qt/Emscripten toolchain without host-target
  leakage.
- Link response files contain the complete runtime contract.
- Runtime package inventory is content-addressed and reproducible.
- No selected native-only dependency appears in the web link graph.

### Core equivalence

Feed the same chart digest, random sequence, configuration, and timestamped
input trace to native and Wasm. Require identical ordered hit events,
judgement offsets, score, combo, gauge history, long-note state, and final
result.

### Asset readiness

Every referenced keysound and BGM resolves and decodes before countdown. A
missing or unsupported required sound is terminal. Instrumentation proves no
decode or filesystem operation occurs during play.

### Software budgets

- occurrence to session dequeue: p95 at most 2 ms and p99 at most 4 ms while
  idle;
- occurrence to first generated sample: p50 at most 4 ms, p95 at most 8 ms,
  and p99 at most 12 ms while idle;
- judgement timestamp error at most 0.1 ms;
- visible pressed-lane response p95 at most two display frames;
- zero AudioWorklet underruns in a complete reference-chart run.

### Adversarial runtime

Exercise dense keysounds, 120/144 Hz, fullscreen transitions, resize,
visibility loss, held-key focus loss, repeated keys, artificial main-thread
stalls, CPU throttling, shared-ring pressure, and clean abort. Judgements stay
deterministic and stalls do not stop AudioWorklet rendering.

### Physical feel

Run native and Chromium on the same wired keyboard/controller and audio output.
The human A/B test is the milestone decision. Where measurement equipment is
available, target:

- physical median at most 25 ms;
- physical p95 at most 35 ms;
- Chromium no more than 8 ms median and 12 ms p95 slower than native.

Software telemetry is necessary diagnostics but does not replace the physical
test.

## First handoff

The first handoff is a single command that:

1. packages the configured local chart directory;
2. builds the web-playtest target;
3. starts the strict local server;
4. opens the pinned Chromium-for-Testing profile;
5. leaves the browser open;
6. writes the downloaded telemetry report to a documented local directory.

The handoff is ready only after a complete chart can be started, played,
finished or aborted, and repeated without console errors, missing assets,
stuck keys, AudioWorklet underruns, or divergence from the deterministic
native trace.

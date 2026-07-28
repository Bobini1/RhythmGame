#include "WebPlaytestRuntime.h"

#include "audio/RealtimeMixer.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/heap.h>
#endif

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <string>
#include <system_error>
#include <utility>

namespace web_playtest {
namespace {

constexpr auto countdownSecondsValue = 2.5;
constexpr auto targetReadyHeapBytes = std::size_t{ 512 } * 1024 * 1024;

auto
phaseName(RuntimePhase phase) -> QString
{
    switch (phase) {
        case RuntimePhase::InstallingChart:
            return WebPlaytestRuntime::tr("Installing chart");
        case RuntimePhase::Decoding:
            return WebPlaytestRuntime::tr("Decoding keysounds");
        case RuntimePhase::Ready:
            return WebPlaytestRuntime::tr("Ready");
        case RuntimePhase::Countdown:
            return WebPlaytestRuntime::tr("Countdown");
        case RuntimePhase::Playing:
            return WebPlaytestRuntime::tr("Playing");
        case RuntimePhase::Finished:
            return WebPlaytestRuntime::tr("Finished");
        case RuntimePhase::Aborted:
            return WebPlaytestRuntime::tr("Aborted");
        case RuntimePhase::Error:
            return WebPlaytestRuntime::tr("Error");
    }
    return WebPlaytestRuntime::tr("Error");
}

} // namespace

auto
WebPlaytestRuntime::createProcessLifetime(QString installedChartPath,
                                          QString initializationError,
                                          QObject* qmlParent)
  -> WebPlaytestRuntime*
{
    return new WebPlaytestRuntime{ std::move(installedChartPath),
                                   std::move(initializationError),
                                   qmlParent };
}

WebPlaytestRuntime::WebPlaytestRuntime(QString installedChartPath,
                                       QString initializationError,
                                       QObject* qmlParent)
  : QObject(qmlParent)
  , visibleNotes(this)
{
    if (!initializationError.isEmpty()) {
        setTerminalError(std::move(initializationError), false);
        return;
    }

    worklet = EmscriptenAudioWorklet::createProcessLifetime();
    worker = new GameplayWorker{ std::move(installedChartPath) };
    if (worklet == nullptr) {
        setTerminalError(tr("Could not allocate the browser audio runtime"),
                         true);
        return;
    }

    try {
        gameplayThread = new std::thread{ [pageLifetimeWorker = worker] {
            pageLifetimeWorker->run();
        } };
    } catch (const std::system_error& error) {
        setTerminalError(tr("Could not create the gameplay worker: %1")
                           .arg(QString::fromUtf8(error.what())),
                         true);
        return;
    } catch (const std::exception& error) {
        setTerminalError(tr("Could not allocate the gameplay worker: %1")
                           .arg(QString::fromUtf8(error.what())),
                         true);
        return;
    }

#if defined(__EMSCRIPTEN__)
    if (!installBrowserInputCallbacks()) {
        setTerminalError(tr("Could not install browser input callbacks"), true);
        return;
    }
#endif
    if (!worklet->createContextForDecode()) {
        setTerminalError(tr("Could not create the browser AudioContext"), true);
        return;
    }

    pollTimer.setInterval(8);
    pollTimer.setTimerType(Qt::PreciseTimer);
    connect(&pollTimer, &QTimer::timeout, this, &WebPlaytestRuntime::poll);
    pollTimer.start();
}

auto
WebPlaytestRuntime::phase() const noexcept -> int
{
    return static_cast<int>(displayedPhase);
}

auto
WebPlaytestRuntime::phaseText() const -> QString
{
    return phaseName(displayedPhase);
}

auto
WebPlaytestRuntime::canStart() const noexcept -> bool
{
    return !waitingForAnchor && !startCommandInFlight &&
           !abortCommandInFlight && errorMessage.isEmpty() &&
           (displayedPhase == RuntimePhase::Ready ||
            displayedPhase == RuntimePhase::Finished ||
            displayedPhase == RuntimePhase::Aborted);
}

auto
WebPlaytestRuntime::canAbort() const noexcept -> bool
{
    return !abortCommandInFlight && errorMessage.isEmpty() &&
           (waitingForAnchor || startCommandInFlight ||
            displayedPhase == RuntimePhase::Countdown ||
            displayedPhase == RuntimePhase::Playing);
}

auto
WebPlaytestRuntime::startPending() const noexcept -> bool
{
    return waitingForAnchor || startCommandInFlight;
}

auto
WebPlaytestRuntime::inputPreset() const noexcept -> int
{
    return static_cast<int>(selectedInputPreset);
}

void
WebPlaytestRuntime::setInputPreset(int preset)
{
    if (presetIsLocked || (preset != static_cast<int>(InputPreset::Native) &&
                           preset != static_cast<int>(InputPreset::Lr2))) {
        return;
    }
    const auto selected = static_cast<InputPreset>(preset);
    if (selected == selectedInputPreset) {
        return;
    }
    inputDeduplicator.clear();
    selectedInputPreset = selected;
    emit inputPresetChanged();
}

auto
WebPlaytestRuntime::inputPresetLocked() const noexcept -> bool
{
    return presetIsLocked;
}

auto
WebPlaytestRuntime::noteModel() noexcept -> QAbstractItemModel*
{
    return &visibleNotes;
}

auto
WebPlaytestRuntime::chartTitle() const -> QString
{
    return title;
}

auto
WebPlaytestRuntime::chartArtist() const -> QString
{
    return artist;
}

auto
WebPlaytestRuntime::chartBpm() const noexcept -> double
{
    return initialBpm;
}

auto
WebPlaytestRuntime::decodedAssets() const noexcept -> int
{
    return lastDecodedAssets;
}

auto
WebPlaytestRuntime::totalAssets() const noexcept -> int
{
    return lastTotalAssets;
}

auto
WebPlaytestRuntime::countdownSeconds() const noexcept -> double
{
    return snapshotCountdownSeconds;
}

auto
WebPlaytestRuntime::elapsedSeconds() const noexcept -> double
{
    return snapshotElapsedSeconds;
}

auto
WebPlaytestRuntime::currentScrollPosition() const noexcept -> double
{
    return snapshotScrollPosition;
}

auto
WebPlaytestRuntime::score() const noexcept -> double
{
    return snapshotScore;
}

auto
WebPlaytestRuntime::maxScoreNow() const noexcept -> double
{
    return snapshotMaxScoreNow;
}

auto
WebPlaytestRuntime::gauge() const noexcept -> double
{
    return snapshotGauge;
}

auto
WebPlaytestRuntime::combo() const noexcept -> int
{
    return snapshotCombo;
}

auto
WebPlaytestRuntime::pressedLaneMask() const noexcept -> int
{
    return snapshotPressedLaneMask;
}

auto
WebPlaytestRuntime::latestJudgement() const -> QString
{
    return snapshotLatestJudgement;
}

auto
WebPlaytestRuntime::latestDeviationMs() const noexcept -> double
{
    return snapshotLatestDeviationMs;
}

auto
WebPlaytestRuntime::inputLatencyAvailable() const noexcept -> bool
{
    return snapshotInputLatencyAvailable;
}

auto
WebPlaytestRuntime::inputLatencyMs() const noexcept -> double
{
    return snapshotInputLatencyMs;
}

auto
WebPlaytestRuntime::lateInputClampMs() const noexcept -> double
{
    return snapshotLateInputClampMs;
}

auto
WebPlaytestRuntime::lateFrames() const noexcept -> qulonglong
{
    return snapshotLateFrames;
}

auto
WebPlaytestRuntime::droppedInputs() const noexcept -> qulonglong
{
    return snapshotDroppedInputs;
}

auto
WebPlaytestRuntime::activeVoices() const noexcept -> int
{
    return snapshotActiveVoices;
}

auto
WebPlaytestRuntime::underrunTelemetryAvailable() const noexcept -> bool
{
    return audioUnderrunTelemetryAvailable;
}

auto
WebPlaytestRuntime::underruns() const noexcept -> qulonglong
{
    return audioUnderruns;
}

auto
WebPlaytestRuntime::heapBytes() const noexcept -> qulonglong
{
    return observedHeapBytes;
}

auto
WebPlaytestRuntime::readyHeapBytes() const noexcept -> qulonglong
{
    return sealedHeapBytes;
}

auto
WebPlaytestRuntime::terminalError() const -> QString
{
    return errorMessage;
}

void
WebPlaytestRuntime::startFromTrustedGesture()
{
    if (!canStart() || worklet == nullptr ||
        !worklet->readyForTrustedResume()) {
        return;
    }
    const auto sampleRate = worklet->outputSampleRate();
    const auto countdownFrames = static_cast<std::uint64_t>(
      std::llround(static_cast<double>(sampleRate) * countdownSecondsValue));
    const auto generation = nextSessionGeneration++;
    if (!worklet->resumeFromTrustedGesture(generation, countdownFrames)) {
        setTerminalError(tr("Chromium rejected the trusted audio resume"),
                         true);
        return;
    }
    pendingSessionGeneration = generation;
    waitingForAnchor = true;
    startCommandInFlight = false;
    abortCommandInFlight = false;
    presetIsLocked = true;
    emit inputPresetChanged();
    emit phaseChanged();
}

void
WebPlaytestRuntime::abort()
{
    if (!canAbort() || worker == nullptr) {
        return;
    }
    const auto timestamp = browserMonotonicNowUs();
    synthesizeReleases(timestamp);
    if (!errorMessage.isEmpty()) {
        return;
    }
    const auto generation = pendingSessionGeneration != 0
                              ? pendingSessionGeneration
                              : nextSessionGeneration - 1;
    if (!worker->tryEnqueue({ .type = RuntimeCommandType::Abort,
                              .sequenceId = nextInputSequence++,
                              .sessionGeneration = generation,
                              .browserMonotonicUs = timestamp })) {
        setTerminalError(tr("The gameplay command queue overflowed"), true);
        return;
    }
    waitingForAnchor = false;
    startCommandInFlight = false;
    abortCommandInFlight = true;
    presetIsLocked = false;
    emit inputPresetChanged();
    emit phaseChanged();
}

void
WebPlaytestRuntime::refreshSnapshot()
{
    if (worker == nullptr) {
        return;
    }
    const auto* snapshot = worker->snapshots().tryAcquireLatest();
    if (snapshot == nullptr) {
        return;
    }
    const auto sessionIsActive = waitingForAnchor || startCommandInFlight ||
                                 displayedPhase == RuntimePhase::Countdown ||
                                 displayedPhase == RuntimePhase::Playing;
    if (sessionIsActive && pendingSessionGeneration != 0 &&
        snapshot->telemetry.sessionGeneration != pendingSessionGeneration) {
        worker->snapshots().releaseReading();
        return;
    }
    try {
        visibleNotes.apply(snapshot->gameplay);
        snapshotCountdownSeconds = snapshot->countdownSeconds;
        snapshotElapsedSeconds =
          (std::max)(0.0,
                     static_cast<double>(snapshot->gameplay.chartTimeNs) /
                       1.0e9);
        snapshotScrollPosition = snapshot->gameplay.scrollPosition;
        snapshotScore = snapshot->gameplay.points;
        snapshotMaxScoreNow = snapshot->gameplay.maxPointsNow;
        snapshotGauge = snapshot->gameplay.gauge;
        snapshotCombo = snapshot->gameplay.combo;
        snapshotPressedLaneMask = {};
        if (snapshot->gameplay.pressedColumns[7]) {
            snapshotPressedLaneMask |= 1;
        }
        for (auto column = std::size_t{}; column < 7; ++column) {
            if (snapshot->gameplay.pressedColumns[column]) {
                snapshotPressedLaneMask |= 1 << static_cast<int>(column + 1);
            }
        }
        snapshotLatestJudgement =
          judgementName(snapshot->gameplay.latestJudgement);
        snapshotLatestDeviationMs =
          snapshot->gameplay.latestDeviationNs
            ? static_cast<double>(*snapshot->gameplay.latestDeviationNs) / 1.0e6
            : 0.0;
        snapshotInputLatencyAvailable =
          snapshot->telemetry.firstNonZeroInputLatencyAvailable;
        snapshotInputLatencyMs =
          static_cast<double>(snapshot->telemetry.firstNonZeroInputLatencyNs) /
          1.0e6;
        snapshotLateInputClampMs =
          static_cast<double>(snapshot->telemetry.lateInputClampNs) / 1.0e6;
        snapshotLateFrames = snapshot->telemetry.lateByFrames;
        snapshotDroppedInputs = snapshot->telemetry.droppedInputCommands;
        snapshotActiveVoices =
          static_cast<int>(snapshot->telemetry.activeVoices);
        emit snapshotChanged();
    } catch (...) {
        worker->snapshots().releaseReading();
        setTerminalError(tr("Could not apply the gameplay snapshot"), true);
        return;
    }
    worker->snapshots().releaseReading();
}

void
WebPlaytestRuntime::poll()
{
    if (!errorMessage.isEmpty() || worklet == nullptr || worker == nullptr) {
        return;
    }
    if (worklet->terminalError() != AudioWorkletError::None) {
        setTerminalError(tr("The AudioWorklet entered terminal state %1")
                           .arg(static_cast<int>(worklet->terminalError())),
                         true);
        return;
    }
    if (worker->handoffState() == DecodeHandoffState::Failed) {
        setTerminalError(worker->errorText(), false);
        return;
    }

    if (!decodeRequested && worklet->contextReadyForDecode()) {
        if (!worker->requestDecode(worklet->outputSampleRate())) {
            setTerminalError(tr("Could not start chart decoding"), true);
            return;
        }
        decodeRequested = true;
    }
    if (!audioInitializationRequested &&
        worker->handoffState() == DecodeHandoffState::Decoded) {
        initializeDecodedAudio();
        if (!errorMessage.isEmpty()) {
            return;
        }
    }
    if (!heapSealed &&
        worker->handoffState() == DecodeHandoffState::GameplayReady &&
        worklet->lifecycleState() ==
          AudioWorkletLifecycleState::GraphReadyUnsealed) {
        sealReadyHeap();
        if (!errorMessage.isEmpty()) {
            return;
        }
    }

    if (waitingForAnchor) {
        auto anchor = BrowserAudioAnchor{};
        if (worklet->pollForAnchor(anchor) &&
            anchor.sessionGeneration == pendingSessionGeneration) {
            const auto started = worker->tryEnqueue(
              { .type = RuntimeCommandType::StartSession,
                .sequenceId = nextInputSequence++,
                .sessionGeneration = anchor.sessionGeneration,
                .chartStartFrame = anchor.chartStartFrame,
                .browserMonotonicUs = browserMonotonicNowUs(),
                .outputSampleRate = anchor.sampleRate });
            if (!started) {
                setTerminalError(tr("The gameplay command queue overflowed"),
                                 true);
                return;
            }
            waitingForAnchor = false;
            startCommandInFlight = true;
            emit phaseChanged();
        }
    }

    const auto workerPhase = worker->phase();
    if (workerPhase == RuntimePhase::Countdown ||
        workerPhase == RuntimePhase::Playing) {
        if (!enqueueTick()) {
            setTerminalError(tr("The gameplay command queue overflowed"), true);
            return;
        }
    }

    const auto decoded = static_cast<int>(worker->decodedAssetCount());
    const auto total = static_cast<int>(worker->totalAssetCount());
    if (decoded != lastDecodedAssets || total != lastTotalAssets) {
        lastDecodedAssets = decoded;
        lastTotalAssets = total;
        emit loadProgressChanged();
    }
    const auto audioTelemetry = worklet->telemetry();
    const auto newUnderrunAvailability = audioTelemetry.playbackStatsAvailable;
    const auto newUnderruns =
      static_cast<qulonglong>(audioTelemetry.underrunCount);
    const auto newObserved =
      static_cast<qulonglong>(audioTelemetry.observedHeapBytes);
    const auto newSealed =
      static_cast<qulonglong>(audioTelemetry.sealedHeapBytes);
    if (newUnderrunAvailability != audioUnderrunTelemetryAvailable ||
        newUnderruns != audioUnderruns || newObserved != observedHeapBytes ||
        newSealed != sealedHeapBytes) {
        audioUnderrunTelemetryAvailable = newUnderrunAvailability;
        audioUnderruns = newUnderruns;
        observedHeapBytes = newObserved;
        sealedHeapBytes = newSealed;
        emit telemetryChanged();
    }
    if (heapSealed && !worklet->verifySealedHeapOnMainThread()) {
        setTerminalError(tr("The Wasm heap grew after Ready"), true);
        return;
    }
    synchronizePhase();
}

void
WebPlaytestRuntime::initializeDecodedAudio()
{
    auto* decoded = worker->decodedChart();
    if (decoded == nullptr || decoded->soundBank == nullptr) {
        setTerminalError(tr("The decoded sound bank handoff is incomplete"),
                         true);
        return;
    }
    const auto authored = decoded->authoredBgmEventCount;
    if (authored >= AudioTransport::commandCapacity) {
        setTerminalError(tr("The chart has too many authored BGM events"),
                         true);
        return;
    }
    const auto config = RealtimeMixer::Config{
        .outputSampleRate = worklet->outputSampleRate(),
        .voiceCapacity = decoded->soundBank->voiceCount(),
        .scheduledEventCapacity = AudioTransport::commandCapacity,
        .authoredBgmEventCount = authored,
        .liveCommandHeadroom = AudioTransport::commandCapacity - authored,
    };
    if (!worklet->initializeWorklet(std::move(*decoded->soundBank), config)) {
        setTerminalError(tr("Could not initialize the Wasm AudioWorklet"),
                         true);
        return;
    }
    decoded->soundBank.reset();
    title = decoded->title;
    artist = decoded->artist;
    initialBpm = decoded->initialBpm;
    emit metadataChanged();

    if (!worker->publishAudioRuntime(&worklet->transport(),
                                     &worklet->audioClock())) {
        setTerminalError(tr("Could not transfer audio ownership"), true);
        return;
    }
    audioInitializationRequested = true;
}

void
WebPlaytestRuntime::sealReadyHeap()
{
    visibleNotes.reserve(worker->visibleNoteCapacity());
#if defined(__EMSCRIPTEN__)
    const auto before = emscripten_get_heap_size();
    if (before < targetReadyHeapBytes &&
        emscripten_resize_heap(targetReadyHeapBytes) == 0) {
        setTerminalError(tr("Could not pre-grow the Wasm heap to 512 MiB"),
                         true);
        return;
    }
    const auto after = emscripten_get_heap_size();
    if (after > targetReadyHeapBytes) {
        setTerminalError(tr("Ready heap exceeds the 512 MiB qualification cap"),
                         true);
        return;
    }
    if (after < targetReadyHeapBytes) {
        setTerminalError(tr("Wasm heap did not reach its 512 MiB seal size"),
                         true);
        return;
    }
#endif
    if (!worklet->sealReadyHeap()) {
        setTerminalError(tr("Could not seal the Ready audio heap"), true);
        return;
    }
    heapSealed = true;
    worker->setReadyAfterHeapSeal();
}

void
WebPlaytestRuntime::synchronizePhase()
{
    const auto next =
      errorMessage.isEmpty() ? worker->phase() : RuntimePhase::Error;
    if (next == displayedPhase) {
        return;
    }
    displayedPhase = next;
    if ((next == RuntimePhase::Countdown || next == RuntimePhase::Playing) &&
        startCommandInFlight) {
        if (worker->sessionGeneration() != pendingSessionGeneration) {
            setTerminalError(
              tr("The gameplay worker accepted the wrong audio session"), true);
            return;
        }
        startCommandInFlight = false;
    }
    if (next == RuntimePhase::Finished || next == RuntimePhase::Aborted) {
        startCommandInFlight = false;
        abortCommandInFlight = false;
        presetIsLocked = false;
        inputDeduplicator.clear();
        emit inputPresetChanged();
    }
    emit phaseChanged();
}

void
WebPlaytestRuntime::setTerminalError(QString error, bool notifyWorker)
{
    if (error.isEmpty()) {
        error = tr("Unknown web playtest failure");
    }
    if (!errorMessage.isEmpty()) {
        return;
    }
    const auto droppedBeforeTerminal =
      worker != nullptr
        ? static_cast<qulonglong>(worker->droppedInputCommands())
        : qulonglong{};
    const auto droppedChanged = droppedBeforeTerminal > snapshotDroppedInputs;
    snapshotDroppedInputs =
      (std::max)(snapshotDroppedInputs, droppedBeforeTerminal);
    errorMessage = std::move(error);
    displayedPhase = RuntimePhase::Error;
    waitingForAnchor = false;
    startCommandInFlight = false;
    abortCommandInFlight = false;
    presetIsLocked = false;
    if (worklet != nullptr) {
        worklet->enterTerminalSilence();
    }
    if (notifyWorker && worker != nullptr) {
        const auto utf8 = errorMessage.toUtf8();
        worker->failFromMain(
          { utf8.constData(), static_cast<std::size_t>(utf8.size()) });
    }
    emit inputPresetChanged();
    emit phaseChanged();
    if (droppedChanged) {
        emit snapshotChanged();
    }
}

auto
WebPlaytestRuntime::enqueueTick() noexcept -> bool
{
    return worker->tryEnqueue(
      { .type = RuntimeCommandType::Tick,
        .sequenceId = nextInputSequence++,
        .sessionGeneration = pendingSessionGeneration,
        .browserMonotonicUs = browserMonotonicNowUs() });
}

auto
WebPlaytestRuntime::enqueueInput(const BrowserKeyTransition& transition,
                                 std::int64_t browserMonotonicUs) noexcept
  -> bool
{
    const auto sequence = nextInputSequence++;
    return worker->tryEnqueue(
      { .type = RuntimeCommandType::Input,
        .input = { .sequenceId = sequence,
                   .key = transition.key,
                   .action = transition.action,
                   .browserMonotonicUs = browserMonotonicUs },
        .sequenceId = sequence,
        .sessionGeneration = pendingSessionGeneration,
        .browserMonotonicUs = browserMonotonicUs });
}

void
WebPlaytestRuntime::synthesizeReleases(std::int64_t browserMonotonicUs) noexcept
{
    auto releases = std::array<BrowserKeyTransition,
                               BrowserInputDeduplicator::gameplayCodeCount>{};
    const auto count =
      inputDeduplicator.synthesizeReleases(selectedInputPreset, releases);
    if (displayedPhase != RuntimePhase::Countdown &&
        displayedPhase != RuntimePhase::Playing) {
        return;
    }
    for (auto index = std::size_t{}; index < count; ++index) {
        if (!enqueueInput(releases[index], browserMonotonicUs)) {
            setTerminalError(tr("The gameplay command queue overflowed"), true);
            return;
        }
    }
}

auto
WebPlaytestRuntime::browserMonotonicNowUs() const noexcept -> std::int64_t
{
#if defined(__EMSCRIPTEN__)
    return static_cast<std::int64_t>(
      std::llround(emscripten_get_now() * 1'000.0));
#else
    return 0;
#endif
}

auto
WebPlaytestRuntime::judgementName(
  const std::optional<gameplay_logic::Judgement>& judgement) -> QString
{
    if (!judgement) {
        return {};
    }
    switch (*judgement) {
        case gameplay_logic::Judgement::Poor:
            return tr("POOR");
        case gameplay_logic::Judgement::EmptyPoor:
            return tr("EMPTY POOR");
        case gameplay_logic::Judgement::Bad:
            return tr("BAD");
        case gameplay_logic::Judgement::Good:
            return tr("GOOD");
        case gameplay_logic::Judgement::Great:
            return tr("GREAT");
        case gameplay_logic::Judgement::Perfect:
            return tr("PERFECT");
        case gameplay_logic::Judgement::MineHit:
            return tr("MINE");
        case gameplay_logic::Judgement::MineAvoided:
            return tr("MINE AVOIDED");
        case gameplay_logic::Judgement::LnEndSkip:
            return tr("LN END");
        case gameplay_logic::Judgement::LnBeginHit:
            return tr("LN BEGIN");
    }
    return {};
}

#if defined(__EMSCRIPTEN__)
auto
WebPlaytestRuntime::onKeyboard(int eventType,
                               const EmscriptenKeyboardEvent* event,
                               void* userData) noexcept -> EM_BOOL
{
    if (event == nullptr || userData == nullptr) {
        return EM_FALSE;
    }
    return static_cast<WebPlaytestRuntime*>(userData)->handleKeyboard(eventType,
                                                                      *event);
}

auto
WebPlaytestRuntime::onBlur(int,
                           const EmscriptenFocusEvent*,
                           void* userData) noexcept -> EM_BOOL
{
    if (userData != nullptr) {
        auto* runtime = static_cast<WebPlaytestRuntime*>(userData);
        runtime->synthesizeReleases(runtime->browserMonotonicNowUs());
    }
    return EM_FALSE;
}

auto
WebPlaytestRuntime::onVisibilityChange(
  int,
  const EmscriptenVisibilityChangeEvent* event,
  void* userData) noexcept -> EM_BOOL
{
    if (event != nullptr && event->hidden && userData != nullptr) {
        auto* runtime = static_cast<WebPlaytestRuntime*>(userData);
        runtime->synthesizeReleases(runtime->browserMonotonicNowUs());
    }
    return EM_FALSE;
}

auto
WebPlaytestRuntime::handleKeyboard(
  int eventType,
  const EmscriptenKeyboardEvent& event) noexcept -> EM_BOOL
{
    const auto mapping = mapBrowserCode(event.code, selectedInputPreset);
    if (!mapping) {
        return EM_FALSE;
    }
    const auto pressed = eventType == EMSCRIPTEN_EVENT_KEYDOWN;
    if (mapping->control == BrowserControl::Start) {
        if (pressed && !event.repeat) {
            startFromTrustedGesture();
        }
        return EM_TRUE;
    }
    if (mapping->control == BrowserControl::Abort) {
        if (pressed && !event.repeat) {
            abort();
        }
        return EM_TRUE;
    }
    if (displayedPhase != RuntimePhase::Countdown &&
        displayedPhase != RuntimePhase::Playing) {
        return EM_TRUE;
    }
    const auto transition =
      inputDeduplicator.apply(*mapping, pressed, event.repeat);
    if (!transition) {
        return EM_TRUE;
    }
    auto timestampUs = browserMonotonicNowUs();
    constexpr auto maximumTimestampMilliseconds =
      static_cast<double>((std::numeric_limits<std::int64_t>::max)() / 2'000);
    if (std::isfinite(event.timestamp) && event.timestamp >= 0.0 &&
        event.timestamp <= maximumTimestampMilliseconds) {
        timestampUs =
          static_cast<std::int64_t>(std::llround(event.timestamp * 1'000.0));
    }
    if (!enqueueInput(*transition, timestampUs)) {
        setTerminalError(tr("The gameplay command queue overflowed"), true);
    }
    return EM_TRUE;
}

auto
WebPlaytestRuntime::installBrowserInputCallbacks() noexcept -> bool
{
    return emscripten_set_keydown_callback(
             EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EM_TRUE, &onKeyboard) ==
             EMSCRIPTEN_RESULT_SUCCESS &&
           emscripten_set_keyup_callback(
             EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EM_TRUE, &onKeyboard) ==
             EMSCRIPTEN_RESULT_SUCCESS &&
           emscripten_set_blur_callback(
             EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EM_TRUE, &onBlur) ==
             EMSCRIPTEN_RESULT_SUCCESS &&
           emscripten_set_visibilitychange_callback(
             this, EM_TRUE, &onVisibilityChange) == EMSCRIPTEN_RESULT_SUCCESS;
}
#endif

} // namespace web_playtest

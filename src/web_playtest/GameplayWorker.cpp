#include "GameplayWorker.h"

#include "OggVorbisDecoder.h"
#include "audio/AudioCommand.h"
#include "audio/ScheduledPcmSound.h"
#include "gameplay_logic/SinglePlayerGameplayCore.h"
#include "resource_managers/BmsAssetResolver.h"
#include "resource_managers/ChartDataFactory.h"
#include "support/PathToUtfString.h"
#include "support/QStringToPath.h"

#include <QFile>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <exception>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>

namespace web_playtest {
namespace {

using namespace std::chrono_literals;

constexpr auto fixedRandomSequence =
  std::array<qint64, 8>{ 1, 1, 1, 1, 1, 1, 1, 1 };
constexpr auto resetAcknowledgementTimeout = 2s;

auto
asQList(const std::array<qint64, 8>& values) -> QList<qint64>
{
    auto result = QList<qint64>{};
    result.reserve(static_cast<qsizetype>(values.size()));
    for (const auto value : values) {
        result.push_back(value);
    }
    return result;
}

auto
chartView(const QByteArray& bytes) -> std::string_view
{
    return { bytes.constData(), static_cast<std::size_t>(bytes.size()) };
}

auto
isOgg(const std::filesystem::path& path) -> bool
{
    auto extension = support::pathToUtfString(path.extension());
    std::ranges::transform(extension, extension.begin(), [](char value) {
        return static_cast<char>(
          std::tolower(static_cast<unsigned char>(value)));
    });
    return extension == ".ogg";
}

auto
saturatingMicrosecondsToNanoseconds(std::int64_t microseconds) noexcept
  -> std::int64_t
{
    constexpr auto scale = std::int64_t{ 1'000 };
    if (microseconds > (std::numeric_limits<std::int64_t>::max)() / scale) {
        return (std::numeric_limits<std::int64_t>::max)();
    }
    if (microseconds < (std::numeric_limits<std::int64_t>::min)() / scale) {
        return (std::numeric_limits<std::int64_t>::min)();
    }
    return microseconds * scale;
}

} // namespace

GameplayWorker::GameplayWorker(QString installedChartPath)
  : chartPath(std::move(installedChartPath))
{
}

void
GameplayWorker::run() noexcept
{
    try {
        auto sampleRate = requestedSampleRate.load(std::memory_order_acquire);
        while (sampleRate == 0) {
            if (state.load(std::memory_order_acquire) ==
                DecodeHandoffState::Failed) {
                return;
            }
            requestedSampleRate.wait(0, std::memory_order_relaxed);
            if (state.load(std::memory_order_acquire) ==
                DecodeHandoffState::Failed) {
                return;
            }
            sampleRate = requestedSampleRate.load(std::memory_order_acquire);
        }
        if (state.load(std::memory_order_acquire) ==
            DecodeHandoffState::Failed) {
            return;
        }

        decodeChart(sampleRate);
        auto expected = DecodeHandoffState::Decoding;
        if (!state.compare_exchange_strong(expected,
                                           DecodeHandoffState::Decoded,
                                           std::memory_order_release,
                                           std::memory_order_acquire)) {
            return;
        }
        state.notify_all();

        auto handoff = state.load(std::memory_order_acquire);
        while (handoff == DecodeHandoffState::Decoded) {
            state.wait(handoff, std::memory_order_relaxed);
            handoff = state.load(std::memory_order_acquire);
        }
        if (handoff != DecodeHandoffState::AudioTransferred) {
            return;
        }

        core = createCore();
        const auto capacity = core->snapshotVisibleNoteCapacity();
        snapshotMailbox.reserveVisibleNotes(capacity);
        snapshotCapacity.store(capacity, std::memory_order_release);
        auto expectedReady = DecodeHandoffState::AudioTransferred;
        if (!state.compare_exchange_strong(expectedReady,
                                           DecodeHandoffState::GameplayReady,
                                           std::memory_order_release,
                                           std::memory_order_acquire)) {
            return;
        }
        state.notify_all();
        commandLoop();
    } catch (const std::exception& error) {
        setFailure(error.what());
    } catch (...) {
        setFailure("Unknown gameplay-worker failure");
    }
}

auto
GameplayWorker::requestDecode(std::uint32_t outputSampleRate) noexcept -> bool
{
    if (outputSampleRate < minimumOutputSampleRate ||
        outputSampleRate > maximumOutputSampleRate ||
        state.load(std::memory_order_acquire) !=
          DecodeHandoffState::AwaitingSampleRate) {
        return false;
    }
    requestedSampleRate.store(outputSampleRate, std::memory_order_release);
    requestedSampleRate.notify_one();
    return true;
}

auto
GameplayWorker::decodedChart() const noexcept -> DecodedChart*
{
    const auto handoff = state.load(std::memory_order_acquire);
    return handoff == DecodeHandoffState::Decoded ||
               handoff == DecodeHandoffState::AudioTransferred ||
               handoff == DecodeHandoffState::GameplayReady
             ? product.get()
             : nullptr;
}

auto
GameplayWorker::publishAudioRuntime(AudioTransport* transport,
                                    const BrowserAudioClock* clock) noexcept
  -> bool
{
    if (transport == nullptr || clock == nullptr ||
        state.load(std::memory_order_acquire) != DecodeHandoffState::Decoded) {
        return false;
    }
    audioTransport.store(transport, std::memory_order_relaxed);
    audioClock.store(clock, std::memory_order_relaxed);
    auto expected = DecodeHandoffState::Decoded;
    if (!state.compare_exchange_strong(expected,
                                       DecodeHandoffState::AudioTransferred,
                                       std::memory_order_release,
                                       std::memory_order_acquire)) {
        return false;
    }
    state.notify_one();
    return true;
}

void
GameplayWorker::failFromMain(std::string_view message) noexcept
{
    setFailure(message);
}

void
GameplayWorker::setReadyAfterHeapSeal() noexcept
{
    if (state.load(std::memory_order_acquire) ==
        DecodeHandoffState::GameplayReady) {
        auto expected = RuntimePhase::Decoding;
        (void)currentPhase.compare_exchange_strong(expected,
                                                   RuntimePhase::Ready,
                                                   std::memory_order_release,
                                                   std::memory_order_acquire);
    }
}

auto
GameplayWorker::tryEnqueue(const RuntimeCommand& command) noexcept -> bool
{
    return commands.tryPush(command);
}

auto
GameplayWorker::handoffState() const noexcept -> DecodeHandoffState
{
    return state.load(std::memory_order_acquire);
}

auto
GameplayWorker::phase() const noexcept -> RuntimePhase
{
    return currentPhase.load(std::memory_order_acquire);
}

auto
GameplayWorker::sessionGeneration() const noexcept -> std::uint64_t
{
    return currentSessionGeneration.load(std::memory_order_acquire);
}

auto
GameplayWorker::decodedAssetCount() const noexcept -> std::uint32_t
{
    return decodedAssets.load(std::memory_order_acquire);
}

auto
GameplayWorker::totalAssetCount() const noexcept -> std::uint32_t
{
    return totalAssets.load(std::memory_order_acquire);
}

auto
GameplayWorker::visibleNoteCapacity() const noexcept -> std::size_t
{
    return snapshotCapacity.load(std::memory_order_acquire);
}

auto
GameplayWorker::errorText() const -> QString
{
    const auto length = terminalErrorLength.load(std::memory_order_acquire);
    return QString::fromUtf8(
      terminalError.data(),
      static_cast<qsizetype>((std::min)(length, terminalError.size())));
}

auto
GameplayWorker::droppedInputCommands() const noexcept -> std::uint64_t
{
    return commands.droppedCount();
}

auto
GameplayWorker::completedTrace() const noexcept -> const QByteArray*
{
    return publishedCompletedTrace.load(std::memory_order_acquire);
}

auto
GameplayWorker::snapshots() noexcept -> SnapshotMailbox&
{
    return snapshotMailbox;
}

void
GameplayWorker::decodeChart(std::uint32_t outputSampleRate)
{
    auto expected = DecodeHandoffState::AwaitingSampleRate;
    if (!state.compare_exchange_strong(expected,
                                       DecodeHandoffState::Decoding,
                                       std::memory_order_acq_rel,
                                       std::memory_order_acquire)) {
        return;
    }
    auto expectedPhase = RuntimePhase::InstallingChart;
    if (!currentPhase.compare_exchange_strong(expectedPhase,
                                              RuntimePhase::Decoding,
                                              std::memory_order_release,
                                              std::memory_order_acquire)) {
        return;
    }

    const auto logicalPath = support::qStringToPath(chartPath);
    QFile chartFile{ chartPath };
    if (!chartFile.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Could not open the installed chart");
    }
    auto bytes = chartFile.readAll();
    if (chartFile.error() != QFileDevice::NoError || bytes.isEmpty()) {
        throw std::runtime_error("Could not read the installed chart");
    }

    auto components =
      resource_managers::ChartDataFactory{}.loadChartDataWithRandomSequence(
        chartView(bytes), logicalPath, asQList(fixedRandomSequence));
    if (!components) {
        throw std::runtime_error(
          "The fixed Dstorv #RANDOM sequence was rejected");
    }

    auto declarations =
      std::vector<std::pair<std::uint64_t, std::filesystem::path>>{};
    declarations.reserve(components->wavs.size());
    for (const auto& declaration : components->wavs) {
        declarations.push_back(declaration);
    }
    std::ranges::sort(declarations, [](const auto& left, const auto& right) {
        return left.first < right.first;
    });
    if (declarations.empty() ||
        declarations.size() > AudioTransport::voiceCapacity ||
        declarations.size() > (std::numeric_limits<std::uint32_t>::max)()) {
        throw std::runtime_error("The chart has an invalid sound inventory");
    }
    totalAssets.store(static_cast<std::uint32_t>(declarations.size()),
                      std::memory_order_release);

    auto resolver =
      charts::BmsAssetResolver::fromDirectory(logicalPath.parent_path());
    if (!resolver.valid()) {
        throw std::runtime_error(resolver.diagnostic());
    }
    auto bank = std::make_unique<PcmSoundBank>(outputSampleRate);
    auto clips = std::unordered_map<std::string, ClipId>{};
    clips.reserve(declarations.size());
    auto voices = std::vector<std::pair<std::uint64_t, VoiceId>>{};
    voices.reserve(declarations.size());

    for (const auto& [soundId, declaredPath] : declarations) {
        if (state.load(std::memory_order_acquire) ==
            DecodeHandoffState::Failed) {
            return;
        }
        const auto resolved =
          resolver.resolve(support::pathToUtfString(declaredPath));
        if (!resolved) {
            throw std::runtime_error(
              "A declared chart keysound could not be resolved");
        }
        auto clip = clips.find(resolved->relativeUtf8);
        if (clip == clips.end()) {
            if (!isOgg(resolved->actualPath)) {
                throw std::runtime_error(
                  "The initial web playtest only accepts OGG keysounds");
            }
            const auto clipId = bank->addClip(
              resolved->relativeUtf8, decodeOggVorbis(resolved->actualPath));
            clip = clips.emplace(resolved->relativeUtf8, clipId).first;
        }
        voices.emplace_back(soundId, bank->addVoice(soundId, clip->second));
        decodedAssets.fetch_add(1, std::memory_order_release);
    }
    bank->freeze();

    const auto authoredBgm = components->notesData.bgmNotes.size();
    if (authoredBgm > AudioTransport::commandCapacity ||
        authoredBgm == AudioTransport::commandCapacity) {
        throw std::runtime_error(
          "The authored BGM stream leaves no live command headroom");
    }

    product = std::make_unique<DecodedChart>(
      DecodedChart{ .soundBank = std::move(bank),
                    .voiceMapping = std::move(voices),
                    .chartBytes = std::move(bytes),
                    .logicalChartPath = logicalPath,
                    .title = components->chartData->getTitle(),
                    .artist = components->chartData->getArtist(),
                    .initialBpm = components->chartData->getInitialBpm(),
                    .chartLengthNs = components->chartData->getLength(),
                    .authoredBgmEventCount = authoredBgm });
}

auto
GameplayWorker::createCore()
  -> std::unique_ptr<gameplay_logic::SinglePlayerGameplayCore>
{
    auto* transport = audioTransport.load(std::memory_order_acquire);
    if (product == nullptr || transport == nullptr) {
        throw std::logic_error("Audio handoff is incomplete");
    }
    auto sounds =
      std::unordered_map<std::uint64_t, std::shared_ptr<sounds::Sound>>{};
    sounds.reserve(product->voiceMapping.size());
    for (const auto& [soundId, voiceId] : product->voiceMapping) {
        sounds.emplace(
          soundId, std::make_shared<ScheduledPcmSound>(*transport, voiceId));
    }
    auto config = gameplay_logic::GameplayCoreConfig{
        .play = { .randomSequence = asQList(fixedRandomSequence),
                  .noteOrderP1 = resource_managers::NoteOrderAlgorithm::Normal,
                  .noteOrderP2 = resource_managers::NoteOrderAlgorithm::Normal,
                  .dpMode = resource_managers::DpOptions::Off,
                  .laneSeed = 1 },
        .savedTimestampSeconds = 1'700'000'000,
        .scoreGuid = QStringLiteral("web-playtest-dstorv"),
        .maxHitValue = 2.0,
    };
    return gameplay_logic::SinglePlayerGameplayCore::create(
      chartView(product->chartBytes),
      product->logicalChartPath,
      std::move(config),
      std::move(sounds));
}

void
GameplayWorker::commandLoop()
{
    for (;;) {
        auto command = RuntimeCommand{};
        auto handled = false;
        while (commands.tryPop(command)) {
            handled = true;
            drainAudioAcknowledgements();
            handleCommand(command);
            drainAudioAcknowledgements();
            if (state.load(std::memory_order_acquire) ==
                DecodeHandoffState::Failed) {
                return;
            }
        }
        if (!handled) {
            commands.waitForData();
        }
    }
}

void
GameplayWorker::handleCommand(const RuntimeCommand& command)
{
    switch (command.type) {
        case RuntimeCommandType::Input:
            if (command.sessionGeneration !=
                currentSessionGeneration.load(std::memory_order_acquire)) {
                return;
            }
            processInput(command.input);
            break;
        case RuntimeCommandType::Tick:
            if (command.sessionGeneration !=
                currentSessionGeneration.load(std::memory_order_acquire)) {
                return;
            }
            processTick(command.browserMonotonicUs);
            break;
        case RuntimeCommandType::StartSession:
            startSession(command);
            break;
        case RuntimeCommandType::Abort:
            abortSession(command);
            break;
    }
}

void
GameplayWorker::startSession(const RuntimeCommand& command)
{
    const auto startingPhase = currentPhase.load(std::memory_order_acquire);
    const auto previousGeneration =
      currentSessionGeneration.load(std::memory_order_acquire);
    if (command.sessionGeneration <= previousGeneration ||
        command.outputSampleRate == 0 ||
        (startingPhase != RuntimePhase::Ready &&
         startingPhase != RuntimePhase::Finished &&
         startingPhase != RuntimePhase::Aborted)) {
        return;
    }
    if (sessionEverStarted) {
        core = createCore();
    }
    publishedCompletedTrace.store(nullptr, std::memory_order_release);
    timestampWatermark.reset();
    telemetry = {};
    telemetry.sessionGeneration = command.sessionGeneration;
    currentSessionGeneration.store(command.sessionGeneration,
                                   std::memory_order_release);

    if (!resetAudioSession(command)) {
        setFailure("Audio reset was not acknowledged for the new session");
        return;
    }
    const auto session =
      audioTransport.load(std::memory_order_acquire)->sessionSnapshot();
    if (session.generation != command.sessionGeneration ||
        session.chartStartFrame != command.chartStartFrame ||
        session.outputSampleRate != command.outputSampleRate) {
        setFailure("Audio reset acknowledgement did not match its session");
        return;
    }

    audioTransport.load(std::memory_order_relaxed)
      ->setNeutralCommandProvenance(browserMonotonicNowUs());
    core->preScheduleBgm();
    audioTransport.load(std::memory_order_relaxed)
      ->setNeutralCommandProvenance(browserMonotonicNowUs());
    auto expectedPhase = startingPhase;
    if (!currentPhase.compare_exchange_strong(expectedPhase,
                                              RuntimePhase::Countdown,
                                              std::memory_order_release,
                                              std::memory_order_acquire)) {
        return;
    }
    sessionEverStarted = true;
}

void
GameplayWorker::abortSession(const RuntimeCommand& command) noexcept
{
    auto phase = currentPhase.load(std::memory_order_acquire);
    const auto generation =
      currentSessionGeneration.load(std::memory_order_acquire);
    const auto generationMatches =
      (phase == RuntimePhase::Ready && generation == 0 &&
       command.sessionGeneration > generation) ||
      ((phase == RuntimePhase::Countdown || phase == RuntimePhase::Playing) &&
       command.sessionGeneration == generation);
    if (!generationMatches ||
        !currentPhase.compare_exchange_strong(phase,
                                              RuntimePhase::Aborted,
                                              std::memory_order_release,
                                              std::memory_order_acquire)) {
        return;
    }

    auto* transport = audioTransport.load(std::memory_order_acquire);
    if (transport != nullptr && generation != 0) {
        const auto sequence = transport->nextSequence();
        const auto session = transport->sessionSnapshot();
        (void)transport->tryPublish(
          { .type = AudioCommandType::StopAll,
            .sessionGeneration = generation,
            .sequenceId = sequence,
            .targetFrame = session.currentOutputFrame,
            .publishedMonotonicUs = browserMonotonicNowUs() });
    }
}

auto
GameplayWorker::resetAudioSession(const RuntimeCommand& command) noexcept
  -> bool
{
    auto* transport = audioTransport.load(std::memory_order_acquire);
    if (transport == nullptr) {
        return false;
    }
    const auto resetSequence = transport->nextSequence();
    if (!transport->tryPublish(
          { .type = AudioCommandType::ResetSession,
            .sessionGeneration = command.sessionGeneration,
            .sequenceId = resetSequence,
            .targetFrame = command.chartStartFrame,
            .publishedMonotonicUs = browserMonotonicNowUs() })) {
        return false;
    }

    const auto deadline =
      std::chrono::steady_clock::now() + resetAcknowledgementTimeout;
    while (std::chrono::steady_clock::now() < deadline) {
        auto acknowledgement = AudioAcknowledgement{};
        auto drained = false;
        while (transport->acknowledgements.tryPop(acknowledgement)) {
            drained = true;
            observeAudioAcknowledgement(acknowledgement);
            if (acknowledgement.sessionGeneration ==
                  command.sessionGeneration &&
                acknowledgement.sequenceId == resetSequence &&
                acknowledgement.phase == AudioAckPhase::Terminal) {
                return acknowledgement.outcome == AudioAckOutcome::Completed;
            }
        }
        if (transport->terminalError().code != AudioTerminalErrorCode::None) {
            return false;
        }
        if (!drained) {
            std::this_thread::sleep_for(1ms);
        }
    }
    return false;
}

void
GameplayWorker::processInput(const InputEvent& inputEvent)
{
    auto phase = currentPhase.load(std::memory_order_acquire);
    if (phase != RuntimePhase::Countdown && phase != RuntimePhase::Playing) {
        return;
    }
    const auto chartTimeNs = mapAndClamp(inputEvent.browserMonotonicUs);
    if (!chartTimeNs || *chartTimeNs < 0) {
        return;
    }
    if (phase == RuntimePhase::Countdown &&
        !currentPhase.compare_exchange_strong(phase,
                                              RuntimePhase::Playing,
                                              std::memory_order_release,
                                              std::memory_order_acquire) &&
        phase != RuntimePhase::Playing) {
        return;
    }
    auto* transport = audioTransport.load(std::memory_order_acquire);
    transport->setCommandProvenance(
      { .sourceInputId = inputEvent.sequenceId,
        .sourceEventMonotonicUs = inputEvent.browserMonotonicUs,
        .publishedMonotonicUs = browserMonotonicNowUs() });
    try {
        core->passKey(inputEvent.key,
                      inputEvent.action,
                      std::chrono::nanoseconds{ *chartTimeNs });
    } catch (...) {
        transport->setNeutralCommandProvenance(browserMonotonicNowUs());
        throw;
    }
    transport->setNeutralCommandProvenance(browserMonotonicNowUs());
    publishSnapshot(*chartTimeNs);
}

void
GameplayWorker::processTick(std::int64_t browserMonotonicUs)
{
    const auto phase = currentPhase.load(std::memory_order_acquire);
    if (phase != RuntimePhase::Countdown && phase != RuntimePhase::Playing) {
        return;
    }
    const auto chartTimeNs = mapAndClamp(browserMonotonicUs);
    if (!chartTimeNs) {
        return;
    }
    core->advanceTo(std::chrono::nanoseconds{ *chartTimeNs });
    if (*chartTimeNs >= 0 && phase == RuntimePhase::Countdown) {
        auto expected = RuntimePhase::Countdown;
        (void)currentPhase.compare_exchange_strong(expected,
                                                   RuntimePhase::Playing,
                                                   std::memory_order_release,
                                                   std::memory_order_acquire);
    }
    publishSnapshot(*chartTimeNs);
}

auto
GameplayWorker::mapAndClamp(std::int64_t browserMonotonicUs) noexcept
  -> std::optional<std::int64_t>
{
    auto mapped = std::int64_t{};
    const auto* clock = audioClock.load(std::memory_order_acquire);
    if (clock == nullptr ||
        !clock->chartTimeForBrowserEventUs(browserMonotonicUs, mapped)) {
        setFailure("The audible browser clock mapping became unavailable");
        return std::nullopt;
    }
    const auto effective = timestampWatermark.clamp(mapped);
    telemetry.lateInputClampNs = timestampWatermark.lateInputClampNs();
    return effective;
}

void
GameplayWorker::drainAudioAcknowledgements() noexcept
{
    auto* transport = audioTransport.load(std::memory_order_acquire);
    if (transport == nullptr) {
        return;
    }
    auto acknowledgement = AudioAcknowledgement{};
    while (transport->acknowledgements.tryPop(acknowledgement)) {
        observeAudioAcknowledgement(acknowledgement);
    }
    if (transport->terminalError().code != AudioTerminalErrorCode::None) {
        setFailure("The real-time audio transport entered a terminal state");
    }
}

void
GameplayWorker::observeAudioAcknowledgement(
  const AudioAcknowledgement& acknowledgement) noexcept
{
    if (acknowledgement.sessionGeneration != telemetry.sessionGeneration) {
        return;
    }
    telemetry.lateByFrames =
      (std::max)(telemetry.lateByFrames, acknowledgement.lateByFrames);
    if (acknowledgement.phase != AudioAckPhase::FirstNonZero ||
        acknowledgement.sourceInputId == neutralSourceInputId) {
        return;
    }

    const auto* clock = audioClock.load(std::memory_order_acquire);
    auto anchor = BrowserAudioAnchor{};
    auto chartTimeNs = std::int64_t{};
    if (clock == nullptr || !clock->tryReadAnchor(anchor) ||
        !clock->chartTimeForRenderedFrame(acknowledgement.observedFrame,
                                          chartTimeNs)) {
        return;
    }
    const auto eventNs = saturatingMicrosecondsToNanoseconds(
      acknowledgement.sourceEventMonotonicUs);
    const auto outputNs = anchor.chartStartBrowserMonotonicNs + chartTimeNs;
    telemetry.firstNonZeroInputLatencyNs =
      outputNs > eventNs ? static_cast<std::uint64_t>(outputNs - eventNs) : 0;
    telemetry.firstNonZeroInputLatencyAvailable = true;
}

void
GameplayWorker::publishSnapshot(std::int64_t chartTimeNs)
{
    auto slot = SnapshotMailbox::invalidSlot;
    auto* payload = snapshotMailbox.tryBeginWrite(slot);
    if (payload == nullptr) {
        return;
    }
    try {
        core->fillSnapshot(payload->gameplay);
        telemetry.droppedInputCommands = commands.droppedCount();
        telemetry.activeVoices = countActiveVoices();
        payload->telemetry = telemetry;
        payload->phase = currentPhase.load(std::memory_order_acquire);
        payload->countdownSeconds =
          chartTimeNs < 0 ? static_cast<double>(-chartTimeNs) / 1.0e9 : 0.0;
        if (payload->gameplay.finished) {
            auto expected = RuntimePhase::Playing;
            if (currentPhase.compare_exchange_strong(
                  expected,
                  RuntimePhase::Finished,
                  std::memory_order_release,
                  std::memory_order_acquire)) {
                // Trace buffers are immutable after publication and retain
                // page lifetime. This keeps the browser copy lock-free while
                // allowing abort/retry sessions without invalidating a reader.
                const auto* completed = new QByteArray{ core->finishTrace() };
                publishedCompletedTrace.store(completed,
                                              std::memory_order_release);
                payload->phase = RuntimePhase::Finished;
            } else {
                payload->phase = expected;
            }
        }
        snapshotMailbox.publishWrite(slot, ++snapshotPublicationSequence);
    } catch (...) {
        snapshotMailbox.cancelWrite(slot);
        throw;
    }
}

void
GameplayWorker::setFailure(std::string_view message) noexcept
{
    if (failureClaimed.test_and_set(std::memory_order_acq_rel)) {
        return;
    }
    const auto length = (std::min)(message.size(), terminalError.size() - 1);
    std::ranges::copy_n(message.begin(), length, terminalError.begin());
    terminalError[length] = '\0';
    terminalErrorLength.store(length, std::memory_order_release);
    state.store(DecodeHandoffState::Failed, std::memory_order_release);
    currentPhase.store(RuntimePhase::Error, std::memory_order_release);
    state.notify_all();
    requestedSampleRate.store((std::numeric_limits<std::uint32_t>::max)(),
                              std::memory_order_release);
    requestedSampleRate.notify_all();
}

auto
GameplayWorker::browserMonotonicNowUs() const noexcept -> std::int64_t
{
#if defined(__EMSCRIPTEN__)
    return static_cast<std::int64_t>(
      std::llround(emscripten_get_now() * 1'000.0));
#else
    return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
#endif
}

auto
GameplayWorker::countActiveVoices() const noexcept -> std::uint32_t
{
    const auto* transport = audioTransport.load(std::memory_order_acquire);
    if (transport == nullptr || product == nullptr) {
        return 0;
    }
    auto active = std::uint32_t{};
    for (const auto& mapping : product->voiceMapping) {
        if (transport->isVoicePlaying(mapping.second)) {
            ++active;
        }
    }
    return active;
}

} // namespace web_playtest

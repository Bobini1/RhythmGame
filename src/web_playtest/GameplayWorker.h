#pragma once

#include "InputEvent.h"
#include "WebPlaytestSnapshot.h"
#include "audio/BrowserAudioClock.h"
#include "audio/PcmSoundBank.h"

#include <QByteArray>
#include <QString>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gameplay_logic {
class SinglePlayerGameplayCore;
}

namespace web_playtest {

class AudioTransport;

enum class DecodeHandoffState : std::uint8_t
{
    AwaitingSampleRate,
    Decoding,
    Decoded,
    AudioTransferred,
    GameplayReady,
    Failed
};

struct DecodedChart
{
    std::unique_ptr<PcmSoundBank> soundBank;
    std::vector<std::pair<std::uint64_t, VoiceId>> voiceMapping;
    QByteArray chartBytes;
    std::filesystem::path logicalChartPath;
    QString title;
    QString artist;
    double initialBpm = {};
    std::int64_t chartLengthNs = {};
    std::size_t authoredBgmEventCount = {};
};

class GameplayWorker final
{
  public:
    static constexpr auto runtimeCommandCapacity = std::size_t{ 4'096 };
    static constexpr auto errorCapacity = std::size_t{ 768 };

    explicit GameplayWorker(QString installedChartPath);
    GameplayWorker(const GameplayWorker&) = delete;
    auto operator=(const GameplayWorker&) -> GameplayWorker& = delete;

    void run() noexcept;

    [[nodiscard]] auto requestDecode(std::uint32_t outputSampleRate) noexcept
      -> bool;
    [[nodiscard]] auto decodedChart() const noexcept -> DecodedChart*;
    [[nodiscard]] auto publishAudioRuntime(
      AudioTransport* transport,
      const BrowserAudioClock* clock) noexcept -> bool;
    void failFromMain(std::string_view message) noexcept;
    void setReadyAfterHeapSeal() noexcept;

    [[nodiscard]] auto tryEnqueue(const RuntimeCommand& command) noexcept
      -> bool;
    [[nodiscard]] auto handoffState() const noexcept -> DecodeHandoffState;
    [[nodiscard]] auto phase() const noexcept -> RuntimePhase;
    [[nodiscard]] auto sessionGeneration() const noexcept -> std::uint64_t;
    [[nodiscard]] auto decodedAssetCount() const noexcept -> std::uint32_t;
    [[nodiscard]] auto totalAssetCount() const noexcept -> std::uint32_t;
    [[nodiscard]] auto visibleNoteCapacity() const noexcept -> std::size_t;
    [[nodiscard]] auto errorText() const -> QString;
    [[nodiscard]] auto droppedInputCommands() const noexcept -> std::uint64_t;
    /**
     * Returns the immutable canonical trace for the most recently completed
     * session. Published trace buffers have page lifetime so browser-side
     * copying never races a retry.
     */
    [[nodiscard]] auto completedTrace() const noexcept -> const QByteArray*;

    [[nodiscard]] auto snapshots() noexcept -> SnapshotMailbox&;

  private:
    void decodeChart(std::uint32_t outputSampleRate);
    [[nodiscard]] auto createCore()
      -> std::unique_ptr<gameplay_logic::SinglePlayerGameplayCore>;
    void commandLoop();
    void handleCommand(const RuntimeCommand& command);
    void startSession(const RuntimeCommand& command);
    void abortSession(const RuntimeCommand& command) noexcept;
    [[nodiscard]] auto resetAudioSession(const RuntimeCommand& command) noexcept
      -> bool;
    void processInput(const InputEvent& inputEvent);
    void processTick(std::int64_t browserMonotonicUs);
    [[nodiscard]] auto mapAndClamp(std::int64_t browserMonotonicUs) noexcept
      -> std::optional<std::int64_t>;
    void drainAudioAcknowledgements() noexcept;
    void observeAudioAcknowledgement(
      const AudioAcknowledgement& acknowledgement) noexcept;
    void publishSnapshot(std::int64_t chartTimeNs);
    void setFailure(std::string_view message) noexcept;
    [[nodiscard]] auto browserMonotonicNowUs() const noexcept -> std::int64_t;
    [[nodiscard]] auto countActiveVoices() const noexcept -> std::uint32_t;

    QString chartPath;
    std::unique_ptr<DecodedChart> product;
    std::unique_ptr<gameplay_logic::SinglePlayerGameplayCore> core;
    RuntimeCommandQueue<runtimeCommandCapacity> commands;
    SnapshotMailbox snapshotMailbox;
    GameplayTimestampWatermark timestampWatermark;

    std::atomic<DecodeHandoffState> state{
        DecodeHandoffState::AwaitingSampleRate
    };
    std::atomic<RuntimePhase> currentPhase{ RuntimePhase::InstallingChart };
    std::atomic<std::uint32_t> requestedSampleRate = {};
    std::atomic<AudioTransport*> audioTransport = {};
    std::atomic<const BrowserAudioClock*> audioClock = {};
    std::atomic<std::uint32_t> decodedAssets = {};
    std::atomic<std::uint32_t> totalAssets = {};
    std::atomic<std::size_t> snapshotCapacity = {};
    std::array<char, errorCapacity> terminalError = {};
    std::atomic<std::size_t> terminalErrorLength = {};
    std::atomic_flag failureClaimed = {};
    std::atomic<const QByteArray*> publishedCompletedTrace = {};

    WorkerTelemetry telemetry;
    std::uint64_t snapshotPublicationSequence = {};
    std::atomic<std::uint64_t> currentSessionGeneration = {};
    bool sessionEverStarted = {};
};

static_assert(std::atomic<DecodeHandoffState>::is_always_lock_free);
static_assert(std::atomic<RuntimePhase>::is_always_lock_free);
static_assert(std::atomic<AudioTransport*>::is_always_lock_free);

} // namespace web_playtest

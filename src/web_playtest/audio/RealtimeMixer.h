#pragma once

#include "AudioCommand.h"
#include "PcmSoundBank.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace web_playtest {

class RealtimeMixer
{
  public:
    struct Config
    {
        std::uint32_t outputSampleRate{};
        std::size_t voiceCapacity{};
        std::size_t scheduledEventCapacity{};
        std::size_t authoredBgmEventCount{};
        std::size_t liveCommandHeadroom{};
    };

    RealtimeMixer(const PcmSoundBank& bank,
                  AudioTransport& transport,
                  Config config);

    void render(float* left, float* right, std::uint32_t frameCount) noexcept;
    [[nodiscard]] auto renderedFrames() const noexcept -> std::uint64_t;
    [[nodiscard]] auto scheduledEventCount() const noexcept -> std::size_t;
    [[nodiscard]] auto storageFingerprint() const noexcept -> std::uintptr_t;

  private:
    struct ActiveVoice
    {
        bool active{};
        bool emittedNonZero{};
        std::size_t clipFrame{};
        AudioCommand start{};
    };

    void drainCommands() noexcept;
    void handleBarrier(const AudioCommand& command) noexcept;
    void cancelPending(bool allGenerations,
                       std::uint64_t generation,
                       std::uint64_t observedFrame) noexcept;
    void cancelActive(bool allGenerations,
                      std::uint64_t generation,
                      std::uint64_t observedFrame) noexcept;
    void schedule(const AudioCommand& command) noexcept;
    void applyDue(std::uint64_t frame) noexcept;
    void apply(const AudioCommand& command, std::uint64_t frame) noexcept;
    void terminateActive(VoiceId voice,
                         AudioAckOutcome outcome,
                         std::uint64_t frame) noexcept;
    void emit(const AudioCommand& command,
              AudioAckPhase phase,
              AudioAckOutcome outcome,
              std::uint64_t frame) noexcept;

    const PcmSoundBank* soundBank;
    AudioTransport* channel;
    Config settings;
    std::vector<ActiveVoice> activeVoices;
    std::vector<float> appliedGains;
    std::vector<AudioCommand> scheduler;
    std::size_t schedulerSize{};
    std::uint64_t currentGeneration{};
    std::uint64_t outputFrame{};
    float masterGain{ 1.F };
};

} // namespace web_playtest

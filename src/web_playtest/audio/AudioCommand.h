#pragma once

#include "SpscRing.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>

#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
#error "The web audio core requires Emscripten pthread atomics"
#endif

namespace web_playtest {

static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
static_assert(std::atomic<std::int64_t>::is_always_lock_free);
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
static_assert(std::atomic<bool>::is_always_lock_free);
static_assert(std::atomic<float>::is_always_lock_free);

using VoiceId = std::uint32_t;
using ClipId = std::uint32_t;

inline constexpr auto invalidVoiceId = (std::numeric_limits<VoiceId>::max)();
inline constexpr auto neutralSourceInputId = std::uint64_t{};
inline constexpr auto neutralSourceEventMonotonicUs = std::int64_t{ -1 };

enum class AudioCommandType : std::uint8_t
{
    ResetSession,
    Start,
    Stop,
    StopAll,
    SetVoiceGain,
    SetMasterGain
};

struct AudioCommand
{
    AudioCommandType type = {};
    std::uint64_t sessionGeneration = {};
    std::uint64_t sequenceId = {};
    std::uint64_t sourceInputId = {};
    VoiceId voiceId = invalidVoiceId;
    std::uint64_t targetFrame = {};
    std::int64_t sourceEventMonotonicUs = neutralSourceEventMonotonicUs;
    std::int64_t publishedMonotonicUs = {};
    float value = 1.F;
};

enum class AudioAckPhase : std::uint8_t
{
    Dequeued,
    Applied,
    FirstNonZero,
    Terminal
};

enum class AudioAckOutcome : std::uint8_t
{
    Pending,
    Completed,
    Canceled,
    Silent,
    Rejected
};

struct AudioAcknowledgement
{
    std::uint64_t sessionGeneration = {};
    std::uint64_t sequenceId = {};
    std::uint64_t sourceInputId = {};
    AudioAckPhase phase = {};
    AudioAckOutcome outcome = {};
    std::uint64_t observedFrame = {};
    std::uint64_t targetFrame = {};
    std::uint64_t lateByFrames = {};
    std::int64_t sourceEventMonotonicUs = neutralSourceEventMonotonicUs;
    std::int64_t publishedMonotonicUs = {};
};

enum class AudioTerminalErrorCode : std::uint8_t
{
    None,
    CommandRingOverflow,
    SchedulerOverflow,
    AcknowledgementRingOverflow,
    InvalidConfiguration
};
static_assert(std::atomic<AudioTerminalErrorCode>::is_always_lock_free);

struct AudioTerminalError
{
    AudioTerminalErrorCode code = {};
    std::uint64_t sessionGeneration = {};
    std::uint64_t sequenceId = {};
};

struct AudioCommandProvenance
{
    std::uint64_t sourceInputId = neutralSourceInputId;
    std::int64_t sourceEventMonotonicUs = neutralSourceEventMonotonicUs;
    std::int64_t publishedMonotonicUs = {};
};

struct AudioSessionSnapshot
{
    std::uint64_t generation = {};
    std::uint64_t chartStartFrame = {};
    std::uint64_t currentOutputFrame = {};
    std::uint32_t outputSampleRate = {};
};

class AudioTransport
{
  public:
    static constexpr std::size_t commandCapacity = 4'096;
    static constexpr std::size_t acknowledgementCapacity = 32'768;
    static constexpr std::size_t voiceCapacity = 8'192;

    SpscRing<AudioCommand, commandCapacity> commands;
    SpscRing<AudioAcknowledgement, acknowledgementCapacity> acknowledgements;

    AudioTransport()
    {
        for (auto& gain : gains) {
            gain.store(1.F, std::memory_order_relaxed);
        }
    }
    AudioTransport(const AudioTransport&) = delete;
    auto operator=(const AudioTransport&) -> AudioTransport& = delete;
    AudioTransport(AudioTransport&&) = delete;
    auto operator=(AudioTransport&&) -> AudioTransport& = delete;

    [[nodiscard]] auto tryPublish(const AudioCommand& command) noexcept -> bool
    {
        if (commands.tryPush(command)) {
            return true;
        }
        fail(AudioTerminalErrorCode::CommandRingOverflow,
             command.sessionGeneration,
             command.sequenceId);
        return false;
    }

    void fail(AudioTerminalErrorCode code,
              std::uint64_t generation,
              std::uint64_t sequence) noexcept
    {
        if (!errorClaimed.test_and_set(std::memory_order_acq_rel)) {
            errorGeneration.store(generation, std::memory_order_relaxed);
            errorSequence.store(sequence, std::memory_order_relaxed);
            errorCode.store(code, std::memory_order_release);
        }
    }

    [[nodiscard]] auto terminalError() const noexcept -> AudioTerminalError
    {
        const auto code = errorCode.load(std::memory_order_acquire);
        return {
            .code = code,
            .sessionGeneration =
              errorGeneration.load(std::memory_order_acquire),
            .sequenceId = errorSequence.load(std::memory_order_acquire),
        };
    }

    void clearTerminalErrorBeforeReady() noexcept
    {
        errorSequence.store(0, std::memory_order_relaxed);
        errorGeneration.store(0, std::memory_order_relaxed);
        errorCode.store(AudioTerminalErrorCode::None,
                        std::memory_order_release);
        errorClaimed.clear(std::memory_order_release);
    }

    void beginSession(std::uint64_t generation,
                      std::uint64_t chartStartFrame,
                      std::uint32_t outputSampleRate) noexcept
    {
        sessionRevision.fetch_add(1, std::memory_order_acq_rel);
        publisherOutputSampleRate.store(outputSampleRate,
                                        std::memory_order_relaxed);
        publisherChartStartFrame.store(chartStartFrame,
                                       std::memory_order_relaxed);
        publisherGeneration.store(generation, std::memory_order_relaxed);
        sessionRevision.fetch_add(1, std::memory_order_release);
    }

    void setCommandProvenance(AudioCommandProvenance value) noexcept
    {
        provenanceInput.store(value.sourceInputId, std::memory_order_relaxed);
        provenanceEvent.store(value.sourceEventMonotonicUs,
                              std::memory_order_relaxed);
        provenancePublished.store(value.publishedMonotonicUs,
                                  std::memory_order_release);
    }

    void setNeutralCommandProvenance(
      std::int64_t publishedMonotonicUs = 0) noexcept
    {
        setCommandProvenance(
          { .sourceInputId = neutralSourceInputId,
            .sourceEventMonotonicUs = neutralSourceEventMonotonicUs,
            .publishedMonotonicUs = publishedMonotonicUs });
    }

    [[nodiscard]] auto commandProvenance() const noexcept
      -> AudioCommandProvenance
    {
        return {
            .sourceInputId = provenanceInput.load(std::memory_order_relaxed),
            .sourceEventMonotonicUs =
              provenanceEvent.load(std::memory_order_relaxed),
            .publishedMonotonicUs =
              provenancePublished.load(std::memory_order_acquire),
        };
    }

    [[nodiscard]] auto nextSequence() noexcept -> std::uint64_t
    {
        return sequence.fetch_add(1, std::memory_order_relaxed);
    }

    [[nodiscard]] auto generation() const noexcept -> std::uint64_t
    {
        return publisherGeneration.load(std::memory_order_acquire);
    }
    [[nodiscard]] auto chartStartFrame() const noexcept -> std::uint64_t
    {
        return publisherChartStartFrame.load(std::memory_order_acquire);
    }
    [[nodiscard]] auto outputSampleRate() const noexcept -> std::uint32_t
    {
        return publisherOutputSampleRate.load(std::memory_order_acquire);
    }
    [[nodiscard]] auto currentOutputFrame() const noexcept -> std::uint64_t
    {
        return outputFrame.load(std::memory_order_acquire);
    }
    void setCurrentOutputFrame(std::uint64_t frame) noexcept
    {
        sessionRevision.fetch_add(1, std::memory_order_acq_rel);
        outputFrame.store(frame, std::memory_order_relaxed);
        sessionRevision.fetch_add(1, std::memory_order_release);
    }

    [[nodiscard]] auto sessionSnapshot() const noexcept -> AudioSessionSnapshot
    {
        for (;;) {
            const auto before = sessionRevision.load(std::memory_order_acquire);
            if ((before & 1U) != 0U) {
                continue;
            }
            const auto snapshot = AudioSessionSnapshot{
                .generation =
                  publisherGeneration.load(std::memory_order_relaxed),
                .chartStartFrame =
                  publisherChartStartFrame.load(std::memory_order_relaxed),
                .currentOutputFrame =
                  outputFrame.load(std::memory_order_relaxed),
                .outputSampleRate =
                  publisherOutputSampleRate.load(std::memory_order_relaxed),
            };
            const auto after = sessionRevision.load(std::memory_order_acquire);
            if (before == after) {
                return snapshot;
            }
        }
    }

    [[nodiscard]] auto isVoicePlaying(VoiceId id) const noexcept -> bool
    {
        return id < voiceCapacity &&
               playing[id].load(std::memory_order_acquire);
    }
    void setVoicePlaying(VoiceId id, bool value) noexcept
    {
        if (id < voiceCapacity) {
            playing[id].store(value, std::memory_order_release);
        }
    }
    [[nodiscard]] auto voiceGain(VoiceId id) const noexcept -> float
    {
        return id < voiceCapacity ? gains[id].load(std::memory_order_acquire)
                                  : 0.F;
    }
    void setAppliedVoiceGain(VoiceId id, float value) noexcept
    {
        if (id < voiceCapacity) {
            gains[id].store(value, std::memory_order_release);
        }
    }

  private:
    std::atomic<AudioTerminalErrorCode> errorCode{
        AudioTerminalErrorCode::None
    };
    std::atomic_flag errorClaimed = {};
    std::atomic<std::uint64_t> errorGeneration = {};
    std::atomic<std::uint64_t> errorSequence = {};
    std::atomic<std::uint64_t> publisherGeneration = {};
    std::atomic<std::uint64_t> publisherChartStartFrame = {};
    std::atomic<std::uint32_t> publisherOutputSampleRate = {};
    std::atomic<std::uint64_t> sequence = 1;
    std::atomic<std::uint64_t> provenanceInput = {};
    std::atomic<std::int64_t> provenanceEvent = neutralSourceEventMonotonicUs;
    std::atomic<std::int64_t> provenancePublished = {};
    std::atomic<std::uint64_t> outputFrame = {};
    mutable std::atomic<std::uint64_t> sessionRevision = {};
    std::array<std::atomic_bool, voiceCapacity> playing = {};
    std::array<std::atomic<float>, voiceCapacity> gains = {};
};

} // namespace web_playtest

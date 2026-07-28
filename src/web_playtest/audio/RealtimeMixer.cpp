#include "RealtimeMixer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace web_playtest {
namespace {
auto
before(const AudioCommand& left, const AudioCommand& right) noexcept -> bool
{
    return left.targetFrame < right.targetFrame ||
           (left.targetFrame == right.targetFrame &&
            left.sequenceId < right.sequenceId);
}

auto
saturate(double value) noexcept -> float
{
    if (value >= 1.0) {
        return 1.F;
    }
    if (value <= -1.0) {
        return -1.F;
    }
    return static_cast<float>(value);
}
} // namespace

RealtimeMixer::RealtimeMixer(const PcmSoundBank& bank,
                             AudioTransport& transport,
                             Config config)
  : soundBank(&bank)
  , channel(&transport)
  , settings(config)
  , activeVoices(config.voiceCapacity)
  , appliedGains(config.voiceCapacity, 1.F)
  , scheduler(config.scheduledEventCapacity)
{
    const auto required =
      config.authoredBgmEventCount + config.liveCommandHeadroom;
    if (!bank.frozen() || config.outputSampleRate != bank.outputSampleRate() ||
        config.voiceCapacity < bank.voiceCount() ||
        config.voiceCapacity > AudioTransport::voiceCapacity ||
        config.scheduledEventCapacity < required ||
        required > AudioTransport::commandCapacity ||
        config.scheduledEventCapacity > AudioTransport::commandCapacity) {
        transport.fail(AudioTerminalErrorCode::InvalidConfiguration, 0, 0);
        throw std::invalid_argument("invalid realtime mixer capacity");
    }
    for (std::size_t voice = 0; voice < bank.voiceCount(); ++voice) {
        channel->setRequestedVoiceGain(static_cast<VoiceId>(voice), 1.F);
        channel->setVoicePlaying(static_cast<VoiceId>(voice), false);
    }
}

void
RealtimeMixer::render(float* left,
                      float* right,
                      std::uint32_t frameCount) noexcept
{
    std::fill_n(left, frameCount, 0.F);
    std::fill_n(right, frameCount, 0.F);
    drainCommands();

    for (std::uint32_t offset = 0; offset < frameCount; ++offset) {
        const auto frame = outputFrame + offset;
        applyDue(frame);
        auto mixedLeft = 0.0;
        auto mixedRight = 0.0;
        for (std::size_t voiceIndex = 0; voiceIndex < activeVoices.size();
             ++voiceIndex) {
            auto& voice = activeVoices[voiceIndex];
            if (!voice.active) {
                continue;
            }
            const auto clipId =
              soundBank->clipForVoice(static_cast<VoiceId>(voiceIndex));
            const auto& clip = soundBank->clip(clipId).interleavedStereo;
            if (voice.clipFrame * 2 >= clip.size()) {
                terminateActive(static_cast<VoiceId>(voiceIndex),
                                voice.emittedNonZero
                                  ? AudioAckOutcome::Completed
                                  : AudioAckOutcome::Silent,
                                frame);
                continue;
            }
            const auto gain = static_cast<double>(appliedGains[voiceIndex]) *
                              static_cast<double>(masterGain);
            const auto contributionLeft =
              static_cast<double>(clip[voice.clipFrame * 2]) * gain;
            const auto contributionRight =
              static_cast<double>(clip[voice.clipFrame * 2 + 1]) * gain;
            if (!voice.emittedNonZero &&
                (contributionLeft != 0.F || contributionRight != 0.F)) {
                voice.emittedNonZero = true;
                emit(voice.start,
                     AudioAckPhase::FirstNonZero,
                     AudioAckOutcome::Pending,
                     frame);
            }
            mixedLeft += contributionLeft;
            mixedRight += contributionRight;
            ++voice.clipFrame;
            if (voice.clipFrame * 2 >= clip.size()) {
                terminateActive(static_cast<VoiceId>(voiceIndex),
                                voice.emittedNonZero
                                  ? AudioAckOutcome::Completed
                                  : AudioAckOutcome::Silent,
                                frame);
            }
        }
        left[offset] = saturate(mixedLeft);
        right[offset] = saturate(mixedRight);
    }
    outputFrame += frameCount;
    channel->setCurrentOutputFrame(outputFrame);
}

void
RealtimeMixer::drainCommands() noexcept
{
    auto command = AudioCommand{};
    while (channel->commands.tryPop(command)) {
        emit(command,
             AudioAckPhase::Dequeued,
             AudioAckOutcome::Pending,
             outputFrame);
        if (command.type == AudioCommandType::ResetSession ||
            command.type == AudioCommandType::StopAll) {
            handleBarrier(command);
            continue;
        }
        if (command.sessionGeneration != currentGeneration) {
            emit(command,
                 AudioAckPhase::Terminal,
                 AudioAckOutcome::Rejected,
                 outputFrame);
            continue;
        }
        schedule(command);
    }
}

void
RealtimeMixer::handleBarrier(const AudioCommand& command) noexcept
{
    if (command.type == AudioCommandType::ResetSession) {
        if (command.sessionGeneration <= currentGeneration) {
            emit(command,
                 AudioAckPhase::Terminal,
                 AudioAckOutcome::Rejected,
                 outputFrame);
            return;
        }
        cancelPending(true, 0, outputFrame);
        cancelActive(true, 0, outputFrame);
        currentGeneration = command.sessionGeneration;
        outputFrame = command.targetFrame;
        channel->beginSession(
          currentGeneration, command.targetFrame, settings.outputSampleRate);
    } else {
        if (command.sessionGeneration != currentGeneration) {
            emit(command,
                 AudioAckPhase::Terminal,
                 AudioAckOutcome::Rejected,
                 outputFrame);
            return;
        }
        cancelPending(false, currentGeneration, outputFrame);
        cancelActive(false, currentGeneration, outputFrame);
    }
    emit(
      command, AudioAckPhase::Applied, AudioAckOutcome::Completed, outputFrame);
    emit(command,
         AudioAckPhase::Terminal,
         AudioAckOutcome::Completed,
         outputFrame);
}

void
RealtimeMixer::cancelPending(bool allGenerations,
                             std::uint64_t generation,
                             std::uint64_t observedFrame) noexcept
{
    std::sort(scheduler.begin(),
              scheduler.begin() + static_cast<std::ptrdiff_t>(schedulerSize),
              before);
    auto retained = std::size_t{};
    for (std::size_t i = 0; i < schedulerSize; ++i) {
        const auto& command = scheduler[i];
        if (allGenerations || command.sessionGeneration == generation) {
            emit(command,
                 AudioAckPhase::Terminal,
                 AudioAckOutcome::Canceled,
                 observedFrame);
        } else {
            scheduler[retained++] = command;
        }
    }
    schedulerSize = retained;
    for (std::size_t i = schedulerSize / 2; i > 0; --i) {
        auto parent = i - 1;
        while (true) {
            const auto left = parent * 2 + 1;
            if (left >= schedulerSize) {
                break;
            }
            auto smallest = left;
            const auto right = left + 1;
            if (right < schedulerSize &&
                before(scheduler[right], scheduler[left])) {
                smallest = right;
            }
            if (!before(scheduler[smallest], scheduler[parent])) {
                break;
            }
            std::swap(scheduler[parent], scheduler[smallest]);
            parent = smallest;
        }
    }
}

void
RealtimeMixer::cancelActive(bool allGenerations,
                            std::uint64_t generation,
                            std::uint64_t observedFrame) noexcept
{
    for (std::size_t i = 0; i < activeVoices.size(); ++i) {
        auto& voice = activeVoices[i];
        if (voice.active &&
            (allGenerations || voice.start.sessionGeneration == generation)) {
            terminateActive(static_cast<VoiceId>(i),
                            AudioAckOutcome::Canceled,
                            observedFrame);
        }
    }
}

void
RealtimeMixer::schedule(const AudioCommand& command) noexcept
{
    if (schedulerSize == scheduler.size()) {
        channel->fail(AudioTerminalErrorCode::SchedulerOverflow,
                      command.sessionGeneration,
                      command.sequenceId);
        emit(command,
             AudioAckPhase::Terminal,
             AudioAckOutcome::Rejected,
             outputFrame);
        return;
    }
    auto position = schedulerSize++;
    scheduler[position] = command;
    while (position > 0) {
        const auto parent = (position - 1) / 2;
        if (!before(scheduler[position], scheduler[parent])) {
            break;
        }
        std::swap(scheduler[position], scheduler[parent]);
        position = parent;
    }
}

void
RealtimeMixer::applyDue(std::uint64_t frame) noexcept
{
    while (schedulerSize != 0 && scheduler[0].targetFrame <= frame) {
        const auto command = scheduler[0];
        --schedulerSize;
        if (schedulerSize != 0) {
            scheduler[0] = scheduler[schedulerSize];
            auto parent = std::size_t{};
            while (true) {
                const auto left = parent * 2 + 1;
                if (left >= schedulerSize) {
                    break;
                }
                auto smallest = left;
                const auto right = left + 1;
                if (right < schedulerSize &&
                    before(scheduler[right], scheduler[left])) {
                    smallest = right;
                }
                if (!before(scheduler[smallest], scheduler[parent])) {
                    break;
                }
                std::swap(scheduler[parent], scheduler[smallest]);
                parent = smallest;
            }
        }
        apply(command, frame);
    }
}

void
RealtimeMixer::apply(const AudioCommand& command, std::uint64_t frame) noexcept
{
    switch (command.type) {
        case AudioCommandType::Start: {
            if (command.voiceId >= soundBank->voiceCount()) {
                emit(command,
                     AudioAckPhase::Terminal,
                     AudioAckOutcome::Rejected,
                     frame);
                return;
            }
            auto& voice = activeVoices[command.voiceId];
            if (voice.active) {
                terminateActive(
                  command.voiceId, AudioAckOutcome::Canceled, frame);
            }
            emit(
              command, AudioAckPhase::Applied, AudioAckOutcome::Pending, frame);
            const auto& clip =
              soundBank->clip(soundBank->clipForVoice(command.voiceId));
            if (clip.interleavedStereo.empty()) {
                emit(command,
                     AudioAckPhase::Terminal,
                     AudioAckOutcome::Silent,
                     frame);
                return;
            }
            voice = { .active = true,
                      .emittedNonZero = false,
                      .clipFrame = 0,
                      .start = command };
            channel->setVoicePlaying(command.voiceId, true);
            break;
        }
        case AudioCommandType::Stop: {
            emit(command,
                 AudioAckPhase::Applied,
                 AudioAckOutcome::Completed,
                 frame);
            if (command.voiceId < activeVoices.size() &&
                activeVoices[command.voiceId].active) {
                terminateActive(
                  command.voiceId, AudioAckOutcome::Canceled, frame);
            }
            emit(command,
                 AudioAckPhase::Terminal,
                 AudioAckOutcome::Completed,
                 frame);
            break;
        }
        case AudioCommandType::SetVoiceGain:
            if (command.voiceId < activeVoices.size() &&
                std::isfinite(command.value)) {
                appliedGains[command.voiceId] = command.value;
                emit(command,
                     AudioAckPhase::Applied,
                     AudioAckOutcome::Completed,
                     frame);
                emit(command,
                     AudioAckPhase::Terminal,
                     AudioAckOutcome::Completed,
                     frame);
            } else {
                emit(command,
                     AudioAckPhase::Terminal,
                     AudioAckOutcome::Rejected,
                     frame);
            }
            break;
        case AudioCommandType::SetMasterGain:
            if (!std::isfinite(command.value)) {
                emit(command,
                     AudioAckPhase::Terminal,
                     AudioAckOutcome::Rejected,
                     frame);
                break;
            }
            masterGain = command.value;
            emit(command,
                 AudioAckPhase::Applied,
                 AudioAckOutcome::Completed,
                 frame);
            emit(command,
                 AudioAckPhase::Terminal,
                 AudioAckOutcome::Completed,
                 frame);
            break;
        case AudioCommandType::ResetSession:
        case AudioCommandType::StopAll:
            break;
    }
}

void
RealtimeMixer::terminateActive(VoiceId voiceId,
                               AudioAckOutcome outcome,
                               std::uint64_t frame) noexcept
{
    auto& voice = activeVoices[voiceId];
    if (!voice.active) {
        return;
    }
    emit(voice.start, AudioAckPhase::Terminal, outcome, frame);
    voice.active = false;
    voice.emittedNonZero = false;
    voice.clipFrame = 0;
    channel->setVoicePlaying(voiceId, false);
}

void
RealtimeMixer::emit(const AudioCommand& command,
                    AudioAckPhase phase,
                    AudioAckOutcome outcome,
                    std::uint64_t frame) noexcept
{
    const auto lateBy =
      frame > command.targetFrame ? frame - command.targetFrame : 0;
    const auto ack = AudioAcknowledgement{
        .sessionGeneration = command.sessionGeneration,
        .sequenceId = command.sequenceId,
        .sourceInputId = command.sourceInputId,
        .phase = phase,
        .outcome = outcome,
        .observedFrame = frame,
        .targetFrame = command.targetFrame,
        .lateByFrames = lateBy,
        .sourceEventMonotonicUs = command.sourceEventMonotonicUs,
        .publishedMonotonicUs = command.publishedMonotonicUs,
    };
    if (!channel->acknowledgements.tryPush(ack)) {
        channel->fail(AudioTerminalErrorCode::AcknowledgementRingOverflow,
                      command.sessionGeneration,
                      command.sequenceId);
    }
}

auto
RealtimeMixer::renderedFrames() const noexcept -> std::uint64_t
{
    return outputFrame;
}
auto
RealtimeMixer::scheduledEventCount() const noexcept -> std::size_t
{
    return schedulerSize;
}
auto
RealtimeMixer::storageFingerprint() const noexcept -> std::uintptr_t
{
    return reinterpret_cast<std::uintptr_t>(activeVoices.data()) ^
           reinterpret_cast<std::uintptr_t>(appliedGains.data()) ^
           reinterpret_cast<std::uintptr_t>(scheduler.data());
}

} // namespace web_playtest

#include "web_playtest/audio/PcmSoundBank.h"
#include "web_playtest/audio/RealtimeMixer.h"
#include "web_playtest/audio/ScheduledPcmSound.h"
#include "web_playtest/audio/SpscRing.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <new>
#include <thread>
#include <type_traits>
#include <vector>

using Catch::Approx;
using namespace std::chrono_literals;
using namespace web_playtest;

static_assert(!std::is_nothrow_default_constructible_v<AudioTransport>);
static_assert(!std::is_copy_constructible_v<AudioTransport>);
static_assert(!std::is_move_constructible_v<AudioTransport>);
static_assert(!std::is_copy_constructible_v<RealtimeMixer>);
static_assert(!std::is_move_constructible_v<RealtimeMixer>);

namespace allocation_probe {
std::atomic<std::uint64_t> count{};
}

void*
operator new(std::size_t size)
{
    allocation_probe::count.fetch_add(1, std::memory_order_relaxed);
    if (auto* memory = std::malloc(size)) {
        return memory;
    }
    throw std::bad_alloc{};
}
void*
operator new[](std::size_t size)
{
    allocation_probe::count.fetch_add(1, std::memory_order_relaxed);
    if (auto* memory = std::malloc(size)) {
        return memory;
    }
    throw std::bad_alloc{};
}
void
operator delete(void* memory) noexcept
{
    std::free(memory);
}
void
operator delete[](void* memory) noexcept
{
    std::free(memory);
}
void
operator delete(void* memory, std::size_t) noexcept
{
    std::free(memory);
}
void
operator delete[](void* memory, std::size_t) noexcept
{
    std::free(memory);
}

namespace {
auto
mono(std::initializer_list<float> samples, std::uint32_t rate = 48'000)
  -> DecodedPcm
{
    return { .interleaved = samples, .sampleRate = rate, .channelCount = 1 };
}

auto
stereo(std::initializer_list<float> samples, std::uint32_t rate = 48'000)
  -> DecodedPcm
{
    return { .interleaved = samples, .sampleRate = rate, .channelCount = 2 };
}

auto
downmixFirstTwo(std::span<const float> samples, std::uint8_t channels)
  -> std::vector<float>
{
    auto output = std::vector<float>{};
    output.reserve(samples.size() / channels * 2);
    for (std::size_t frame = 0; frame < samples.size() / channels; ++frame) {
        output.push_back(samples[frame * channels]);
        output.push_back(samples[frame * channels + 1]);
    }
    return output;
}

struct Fixture
{
    PcmSoundBank bank = PcmSoundBank{ 48'000 };
    AudioTransport transport;
    VoiceId voice0 = {};
    VoiceId voice1 = {};

    Fixture()
    {
        const auto shared =
          bank.addClip("shared.wav", mono({ 0.5F, 0.25F, 0.125F, 0.0F }));
        voice0 = bank.addVoice(1, shared);
        voice1 = bank.addVoice(2, shared);
        bank.freeze();
    }

    auto mixer(std::size_t scheduled = 32) -> RealtimeMixer
    {
        return RealtimeMixer(bank,
                             transport,
                             { .outputSampleRate = 48'000,
                               .voiceCapacity = bank.voiceCount(),
                               .scheduledEventCapacity = scheduled,
                               .authoredBgmEventCount = 0,
                               .liveCommandHeadroom = scheduled });
    }
};

void
push(AudioTransport& transport, AudioCommand command)
{
    REQUIRE(transport.tryPublish(command));
}

auto
drain(AudioTransport& transport) -> std::vector<AudioAcknowledgement>
{
    auto result = std::vector<AudioAcknowledgement>{};
    auto ack = AudioAcknowledgement{};
    while (transport.acknowledgements.tryPop(ack)) {
        result.push_back(ack);
    }
    return result;
}

void
advanceSilently(RealtimeMixer& mixer, std::uint32_t frameCount)
{
    auto left = std::vector<float>(frameCount);
    auto right = std::vector<float>(frameCount);
    mixer.render(left.data(), right.data(), frameCount);
    REQUIRE(
      std::ranges::all_of(left, [](float sample) { return sample == 0.F; }));
    REQUIRE(
      std::ranges::all_of(right, [](float sample) { return sample == 0.F; }));
}
} // namespace

TEST_CASE("SPSC ring is bounded and wraps without reordering",
          "[RealtimeMixer][SpscRing]")
{
    auto ring = SpscRing<std::uint32_t, 3>{};
    REQUIRE(ring.tryPush(1));
    REQUIRE(ring.tryPush(2));
    REQUIRE(ring.tryPush(3));
    REQUIRE_FALSE(ring.tryPush(4));
    auto value = std::uint32_t{};
    REQUIRE(ring.tryPop(value));
    REQUIRE(value == 1);
    REQUIRE(ring.tryPush(4));
    for (const auto expected : { 2U, 3U, 4U }) {
        REQUIRE(ring.tryPop(value));
        REQUIRE(value == expected);
    }
    REQUIRE_FALSE(ring.tryPop(value));
}

TEST_CASE("SPSC acquire-release ordering survives cross-thread wraparound",
          "[RealtimeMixer][SpscRing]")
{
    constexpr auto count = std::uint64_t{ 100'000 };
    auto ring = SpscRing<std::uint64_t, 31>{};
    auto consumerOk = std::atomic_bool{ true };
    auto consumer = std::thread([&] {
        for (std::uint64_t expected = 0; expected < count; ++expected) {
            auto value = std::uint64_t{};
            while (!ring.tryPop(value)) {
                std::this_thread::yield();
            }
            if (value != expected) {
                consumerOk.store(false, std::memory_order_relaxed);
                return;
            }
        }
    });
    for (std::uint64_t value = 0; value < count; ++value) {
        while (!ring.tryPush(value)) {
            std::this_thread::yield();
        }
    }
    consumer.join();
    REQUIRE(consumerOk.load(std::memory_order_relaxed));
    REQUIRE(ring.empty());
}

TEST_CASE("Render drains a bounded command snapshot under replenishment",
          "[RealtimeMixer][SpscRing][realtime]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer(AudioTransport::commandCapacity);
    for (std::size_t i = 0; i < AudioTransport::commandCapacity; ++i) {
        push(fixture.transport,
             { .type = AudioCommandType::SetMasterGain,
               .sessionGeneration = 0,
               .sequenceId = i + 1,
               .targetFrame = 1,
               .value = 1.F });
    }
    auto renderComplete = std::atomic_bool{ false };
    auto producerReady = std::atomic_bool{ false };
    auto successfulRefill = std::atomic_bool{ false };
    auto refilledDuringRender = std::atomic_bool{ false };
    auto producer = std::thread([&] {
        const auto command = AudioCommand{
            .type = AudioCommandType::SetMasterGain,
            .sessionGeneration = 0,
            .sequenceId = 10'000,
            .targetFrame = 1,
            .value = 1.F,
        };
        producerReady.store(true, std::memory_order_release);
        while (!fixture.transport.commands.tryPush(command)) {
            if (renderComplete.load(std::memory_order_acquire)) {
                return;
            }
            std::this_thread::yield();
        }
        successfulRefill.store(true, std::memory_order_release);
        refilledDuringRender.store(
          !renderComplete.load(std::memory_order_acquire),
          std::memory_order_release);
    });
    while (!producerReady.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    auto left = std::vector<float>(1'000'000);
    auto right = std::vector<float>(left.size());
    mixer.render(
      left.data(), right.data(), static_cast<std::uint32_t>(left.size()));
    renderComplete.store(true, std::memory_order_release);
    producer.join();
    REQUIRE(successfulRefill.load(std::memory_order_acquire));
    REQUIRE(refilledDuringRender.load(std::memory_order_acquire));
    REQUIRE(mixer.lastDrainedCommandCountForTesting() ==
            AudioTransport::commandCapacity);
}

TEST_CASE("PCM bank converts to immutable output-rate stereo",
          "[RealtimeMixer][PcmSoundBank]")
{
    auto bank = PcmSoundBank{ 48'000 };
    const auto monoId = bank.addClip("mono", mono({ 0.F, 1.F }, 44'100));
    const auto stereoId =
      bank.addClip("stereo", stereo({ 0.25F, -0.25F, 0.5F, -0.5F }));
    REQUIRE(bank.clip(monoId).sampleRate == 48'000);
    REQUIRE(bank.clip(monoId).interleavedStereo.size() == 4);
    REQUIRE(bank.clip(monoId).interleavedStereo[0] ==
            bank.clip(monoId).interleavedStereo[1]);
    REQUIRE(bank.clip(monoId).interleavedStereo[2] ==
            Approx(0.91875F).margin(0.000001F));
    REQUIRE(bank.clip(stereoId).interleavedStereo ==
            std::vector<float>{ 0.25F, -0.25F, 0.5F, -0.5F });
    REQUIRE_THROWS(bank.addClip(
      "multichannel",
      { .interleaved = { 1, 2, 3 }, .sampleRate = 48'000, .channelCount = 3 }));
    const auto downmixed = bank.addClip("downmixed",
                                        { .interleaved = { 0.25F, -0.5F, 1.F },
                                          .sampleRate = 48'000,
                                          .channelCount = 3,
                                          .downmixToStereo = downmixFirstTwo });
    REQUIRE(bank.clip(downmixed).interleavedStereo ==
            std::vector<float>{ 0.25F, -0.5F });
    const auto unnormalized = bank.addClip("unnormalized", mono({ 2.F }));
    REQUIRE(bank.clip(unnormalized).interleavedStereo[0] == 2.F);
    REQUIRE_THROWS(bank.addClip(
      "malformed",
      { .interleaved = { 1, 2, 3 }, .sampleRate = 48'000, .channelCount = 2 }));
    REQUIRE_THROWS(
      bank.addClip("nan",
                   { .interleaved = { std::numeric_limits<float>::quiet_NaN() },
                     .sampleRate = 48'000,
                     .channelCount = 1 }));
    REQUIRE_THROWS(bank.addClip("empty", mono({})));
    bank.addVoice((std::numeric_limits<std::uint64_t>::max)(), stereoId);
    bank.freeze();
    REQUIRE(bank.frozen());
    REQUIRE_THROWS(bank.addClip("late", mono({ 1 })));

    auto timingBank = PcmSoundBank{ 48'000 };
    auto source = std::vector<float>(441, 0.25F);
    const auto timingClip = timingBank.addClip(
      "441-frames",
      { .interleaved = source, .sampleRate = 44'100, .channelCount = 1 });
    REQUIRE(timingBank.clip(timingClip).interleavedStereo.size() == 480 * 2);
    const auto floorClip =
      timingBank.addClip("floor-100",
                         { .interleaved = std::vector<float>(100, 0.25F),
                           .sampleRate = 44'100,
                           .channelCount = 1 });
    REQUIRE(timingBank.clip(floorClip).interleavedStereo.size() == 108 * 2);
    REQUIRE(detail::resampledFrameCount(
              (std::numeric_limits<std::uint32_t>::max)(), 8'000, 192'000) ==
            std::size_t{ 103'079'215'080 });
    REQUIRE_THROWS(detail::resampledFrameCount(
      (std::numeric_limits<std::size_t>::max)(), 8'000, 192'000));
    REQUIRE_THROWS(
      detail::resampledFrameCount((std::numeric_limits<std::uint32_t>::max)(),
                                  8'000,
                                  192'000,
                                  (std::numeric_limits<std::uint32_t>::max)()));
    REQUIRE_THROWS(timingBank.addClip("source-low", mono({ 1.F }, 7'999)));
    REQUIRE_THROWS(timingBank.addClip("source-high", mono({ 1.F }, 192'001)));
    REQUIRE_THROWS(PcmSoundBank{ 7'999 });
    REQUIRE_THROWS(PcmSoundBank{ 192'001 });
}

TEST_CASE("Mixer drains around far-future commands and is sample accurate",
          "[RealtimeMixer]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    advanceSilently(mixer, 100);
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 100 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 2,
           .voiceId = fixture.voice0,
           .targetFrame = 10'000 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 3,
           .sourceInputId = 77,
           .voiceId = fixture.voice1,
           .targetFrame = 102,
           .sourceEventMonotonicUs = 5,
           .publishedMonotonicUs = 6 });

    auto left = std::array<float, 8>{};
    auto right = std::array<float, 8>{};
    mixer.render(left.data(), right.data(), left.size());
    REQUIRE(left[0] == 0.F);
    REQUIRE(left[1] == 0.F);
    REQUIRE(left[2] == Approx(0.5F));
    REQUIRE(left[3] == Approx(0.25F));
    REQUIRE(mixer.renderedFrames() == 108);

    const auto acks = drain(fixture.transport);
    REQUIRE(std::ranges::any_of(acks, [](const auto& ack) {
        return ack.sequenceId == 2 && ack.phase == AudioAckPhase::Dequeued &&
               ack.observedFrame == 100;
    }));
    REQUIRE(std::ranges::any_of(acks, [](const auto& ack) {
        return ack.sequenceId == 3 &&
               ack.phase == AudioAckPhase::FirstNonZero &&
               ack.sourceInputId == 77 && ack.observedFrame == 102;
    }));
}

TEST_CASE("Future session anchors preserve the physical output clock",
          "[RealtimeMixer][session][clock]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    advanceSilently(mixer, 8);

    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 12 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 2,
           .voiceId = fixture.voice0,
           .targetFrame = 12 });
    auto left = std::array<float, 5>{};
    auto right = left;
    mixer.render(left.data(), right.data(), left.size());
    REQUIRE(left == std::array<float, 5>{ 0.F, 0.F, 0.F, 0.F, 0.5F });
    REQUIRE(mixer.renderedFrames() == 13);
    REQUIRE(fixture.transport.sessionSnapshot().chartStartFrame == 12);
    REQUIRE(fixture.transport.sessionSnapshot().currentOutputFrame == 13);

    push(fixture.transport,
         { .type = AudioCommandType::StopAll,
           .sessionGeneration = 1,
           .sequenceId = 3,
           .targetFrame = 13 });
    mixer.render(left.data(), right.data(), 1);
    REQUIRE(mixer.renderedFrames() == 14);

    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 2,
           .sequenceId = 4,
           .targetFrame = 16 });
    mixer.render(left.data(), right.data(), 1);
    REQUIRE(mixer.renderedFrames() == 15);
    REQUIRE(fixture.transport.sessionSnapshot().generation == 2);
    REQUIRE(fixture.transport.sessionSnapshot().chartStartFrame == 16);

    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 3,
           .sequenceId = 5,
           .targetFrame = 14 });
    mixer.render(left.data(), right.data(), 1);
    auto acks = drain(fixture.transport);
    REQUIRE(std::ranges::any_of(acks, [](const auto& ack) {
        return ack.sequenceId == 5 && ack.phase == AudioAckPhase::Terminal &&
               ack.outcome == AudioAckOutcome::Rejected &&
               ack.observedFrame == 15;
    }));
    REQUIRE(mixer.renderedFrames() == 16);
    REQUIRE(fixture.transport.sessionSnapshot().generation == 2);
    REQUIRE(fixture.transport.sessionSnapshot().chartStartFrame == 16);

    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 2,
           .sequenceId = 6,
           .targetFrame = 100 });
    mixer.render(left.data(), right.data(), 1);
    REQUIRE(std::ranges::any_of(drain(fixture.transport), [](const auto& ack) {
        return ack.sequenceId == 6 && ack.phase == AudioAckPhase::Terminal &&
               ack.outcome == AudioAckOutcome::Rejected;
    }));
    REQUIRE(mixer.renderedFrames() == 17);
}

TEST_CASE("Rendered frame polling is monotonic across concurrent resets",
          "[RealtimeMixer][session][clock][concurrency]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    auto started = std::atomic_bool{ false };
    auto go = std::atomic_bool{ false };
    auto finished = std::atomic_bool{ false };
    auto renderThread = std::thread([&] {
        auto left = std::array<float, 1>{};
        auto right = left;
        started.store(true, std::memory_order_release);
        while (!go.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (std::size_t i = 0; i < 20'000; ++i) {
            mixer.render(left.data(), right.data(), 1);
        }
        finished.store(true, std::memory_order_release);
    });
    while (!started.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 100'000 });
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 2,
           .sequenceId = 2,
           .targetFrame = 200'000 });
    go.store(true, std::memory_order_release);

    auto previous = std::uint64_t{};
    while (!finished.load(std::memory_order_acquire)) {
        const auto current = mixer.renderedFrames();
        REQUIRE(current >= previous);
        previous = current;
    }
    renderThread.join();
    REQUIRE(mixer.renderedFrames() >= previous);
    REQUIRE(mixer.renderedFrames() == 20'000);
}

TEST_CASE("Mixer preserves voice identity, overlap, gain, clamp, and stop",
          "[RealtimeMixer]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 0 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 2,
           .voiceId = fixture.voice0,
           .targetFrame = 0 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 3,
           .voiceId = fixture.voice1,
           .targetFrame = 0 });
    auto left = std::array<float, 2>{};
    auto right = std::array<float, 2>{};
    mixer.render(left.data(), right.data(), 1);
    REQUIRE(left[0] == Approx(1.F));

    push(fixture.transport,
         { .type = AudioCommandType::SetMasterGain,
           .sessionGeneration = 1,
           .sequenceId = 4,
           .targetFrame = 1,
           .value = 4.F });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 5,
           .voiceId = fixture.voice0,
           .targetFrame = 1 });
    mixer.render(left.data(), right.data(), 1);
    REQUIRE(left[0] == 1.F);
    push(fixture.transport,
         { .type = AudioCommandType::Stop,
           .sessionGeneration = 1,
           .sequenceId = 6,
           .voiceId = fixture.voice0,
           .targetFrame = 2 });
    mixer.render(left.data(), right.data(), 1);
    REQUIRE_FALSE(fixture.transport.isVoicePlaying(fixture.voice0));
    REQUIRE(fixture.transport.isVoicePlaying(fixture.voice1));
}

TEST_CASE(
  "Mid-buffer start, same-voice retrigger, gain, and clip end are exact",
  "[RealtimeMixer]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    advanceSilently(mixer, 10);
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 10 });
    push(fixture.transport,
         { .type = AudioCommandType::SetVoiceGain,
           .sessionGeneration = 1,
           .sequenceId = 2,
           .voiceId = fixture.voice0,
           .targetFrame = 11,
           .value = 0.5F });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 3,
           .voiceId = fixture.voice0,
           .targetFrame = 11 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 4,
           .voiceId = fixture.voice0,
           .targetFrame = 13 });
    auto left = std::array<float, 8>{};
    auto right = left;
    mixer.render(left.data(), right.data(), left.size());
    REQUIRE(left == std::array<float, 8>{
                      0.F, 0.25F, 0.125F, 0.25F, 0.125F, 0.0625F, 0.F, 0.F });
    REQUIRE_FALSE(fixture.transport.isVoicePlaying(fixture.voice0));
    const auto acks = drain(fixture.transport);
    REQUIRE(std::ranges::any_of(acks, [](const auto& ack) {
        return ack.sequenceId == 3 && ack.phase == AudioAckPhase::Terminal &&
               ack.outcome == AudioAckOutcome::Canceled &&
               ack.observedFrame == 13;
    }));
    REQUIRE(std::ranges::any_of(acks, [](const auto& ack) {
        return ack.sequenceId == 4 && ack.phase == AudioAckPhase::Terminal &&
               ack.outcome == AudioAckOutcome::Completed &&
               ack.observedFrame == 16;
    }));
}

TEST_CASE("Timed gain is render-applied and non-finite gain is rejected",
          "[RealtimeMixer][gain]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 0 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 2,
           .voiceId = fixture.voice0,
           .targetFrame = 0 });
    REQUIRE(fixture.transport.voiceGain(fixture.voice0) == 1.F);
    push(fixture.transport,
         { .type = AudioCommandType::SetVoiceGain,
           .sessionGeneration = 1,
           .sequenceId = 3,
           .voiceId = fixture.voice0,
           .targetFrame = 2,
           .value = 0.25F });
    push(fixture.transport,
         { .type = AudioCommandType::SetMasterGain,
           .sessionGeneration = 1,
           .sequenceId = 4,
           .targetFrame = 3,
           .value = std::numeric_limits<float>::quiet_NaN() });
    auto left = std::array<float, 4>{};
    auto right = left;
    mixer.render(left.data(), right.data(), left.size());
    REQUIRE(fixture.transport.voiceGain(fixture.voice0) == 0.25F);
    REQUIRE(left[0] == Approx(0.5F));
    REQUIRE(left[1] == Approx(0.25F));
    REQUIRE(left[2] == Approx(0.03125F));
    REQUIRE(std::isfinite(left[3]));
    const auto acks = drain(fixture.transport);
    REQUIRE(std::ranges::any_of(acks, [](const auto& ack) {
        return ack.sequenceId == 4 && ack.phase == AudioAckPhase::Terminal &&
               ack.outcome == AudioAckOutcome::Rejected;
    }));
}

TEST_CASE("Canceled and overflowed gain never changes observable volume",
          "[RealtimeMixer][gain][overflow]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    auto sound = ScheduledPcmSound{ fixture.transport, fixture.voice0 };
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 50,
           .targetFrame = 0 });
    auto left = std::array<float, 1>{};
    auto right = left;
    mixer.render(left.data(), right.data(), 1);
    (void)drain(fixture.transport);

    sound.setVolume(0.25F);
    REQUIRE(sound.getVolume() == 1.F);
    push(fixture.transport,
         { .type = AudioCommandType::StopAll,
           .sessionGeneration = 1,
           .sequenceId = 51 });
    mixer.render(left.data(), right.data(), 1);
    REQUIRE(sound.getVolume() == 1.F);
    const auto canceledAcks = drain(fixture.transport);
    const auto barrierApplied =
      std::ranges::find_if(canceledAcks, [](const auto& ack) {
          return ack.sequenceId == 51 && ack.phase == AudioAckPhase::Applied;
      });
    REQUIRE(barrierApplied != canceledAcks.end());
    REQUIRE(std::ranges::count_if(
              canceledAcks.begin(), barrierApplied, [](const auto& ack) {
                  return ack.sequenceId == 1 &&
                         ack.phase == AudioAckPhase::Terminal &&
                         ack.outcome == AudioAckOutcome::Canceled;
              }) == 1);
    REQUIRE_FALSE(std::ranges::any_of(canceledAcks, [](const auto& ack) {
        return ack.sequenceId == 1 && ack.phase == AudioAckPhase::Applied;
    }));

    for (std::size_t i = 0; i < AudioTransport::commandCapacity; ++i) {
        auto command = AudioCommand{};
        command.sequenceId = i + 100;
        REQUIRE(fixture.transport.commands.tryPush(command));
    }
    sound.setVolume(0.5F);
    REQUIRE(sound.getVolume() == 1.F);
    REQUIRE(fixture.transport.terminalError().code ==
            AudioTerminalErrorCode::CommandRingOverflow);
}

TEST_CASE("Finite extreme gain and silence can never produce NaN",
          "[RealtimeMixer][gain]")
{
    auto bank = PcmSoundBank{ 48'000 };
    const auto clip = bank.addClip(
      "extreme", mono({ 0.F, (std::numeric_limits<float>::max)() }));
    const auto voice = bank.addVoice(1, clip);
    bank.freeze();
    auto transport = AudioTransport{};
    auto mixer = RealtimeMixer(bank,
                               transport,
                               { .outputSampleRate = 48'000,
                                 .voiceCapacity = 1,
                                 .scheduledEventCapacity = 8,
                                 .liveCommandHeadroom = 8 });
    push(transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 0 });
    push(transport,
         { .type = AudioCommandType::SetVoiceGain,
           .sessionGeneration = 1,
           .sequenceId = 2,
           .voiceId = voice,
           .targetFrame = 0,
           .value = (std::numeric_limits<float>::max)() });
    push(transport,
         { .type = AudioCommandType::SetMasterGain,
           .sessionGeneration = 1,
           .sequenceId = 3,
           .targetFrame = 0,
           .value = (std::numeric_limits<float>::max)() });
    push(transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 4,
           .voiceId = voice,
           .targetFrame = 0 });
    auto left = std::array<float, 2>{};
    auto right = left;
    mixer.render(left.data(), right.data(), left.size());
    REQUIRE(left[0] == 0.F);
    REQUIRE(left[1] == 1.F);
    REQUIRE(std::ranges::all_of(
      left, [](float sample) { return std::isfinite(sample); }));
}

TEST_CASE("Acknowledgements distinguish leading silence and cancellation",
          "[RealtimeMixer][ack]")
{
    auto bank = PcmSoundBank{ 48'000 };
    const auto leading = bank.addClip("leading", mono({ 0.F, 0.5F }));
    const auto silent = bank.addClip("silent", mono({ 0.F, 0.F }));
    const auto leadingVoice = bank.addVoice(1, leading);
    const auto silentVoice = bank.addVoice(2, silent);
    bank.freeze();
    auto transport = AudioTransport{};
    auto mixer = RealtimeMixer(bank,
                               transport,
                               { .outputSampleRate = 48'000,
                                 .voiceCapacity = 2,
                                 .scheduledEventCapacity = 16,
                                 .liveCommandHeadroom = 16 });
    advanceSilently(mixer, 100);
    push(transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 100 });
    push(transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 2,
           .sourceInputId = 99,
           .voiceId = leadingVoice,
           .targetFrame = 100 });
    push(transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 3,
           .voiceId = silentVoice,
           .targetFrame = 100 });
    auto left = std::array<float, 2>{};
    auto right = left;
    mixer.render(left.data(), right.data(), 1);
    push(transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 4,
           .voiceId = leadingVoice,
           .targetFrame = 101 });
    mixer.render(left.data(), right.data(), 2);
    auto acks = drain(transport);
    REQUIRE_FALSE(std::ranges::any_of(acks, [](const auto& ack) {
        return ack.sequenceId == 2 && ack.phase == AudioAckPhase::FirstNonZero;
    }));
    REQUIRE(std::ranges::any_of(acks, [](const auto& ack) {
        return ack.sequenceId == 2 && ack.phase == AudioAckPhase::Terminal &&
               ack.outcome == AudioAckOutcome::Canceled;
    }));
    REQUIRE(std::ranges::any_of(acks, [](const auto& ack) {
        return ack.sequenceId == 3 && ack.phase == AudioAckPhase::Terminal &&
               ack.outcome == AudioAckOutcome::Silent;
    }));
    REQUIRE(std::ranges::any_of(acks, [](const auto& ack) {
        return ack.sequenceId == 4 &&
               ack.phase == AudioAckPhase::FirstNonZero &&
               ack.observedFrame == 102;
    }));

    push(transport,
         { .type = AudioCommandType::SetVoiceGain,
           .sessionGeneration = 1,
           .sequenceId = 5,
           .voiceId = leadingVoice,
           .targetFrame = 103,
           .value = 0.F });
    push(transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 6,
           .voiceId = leadingVoice,
           .targetFrame = 103 });
    mixer.render(left.data(), right.data(), 2);
    acks = drain(transport);
    REQUIRE(std::ranges::any_of(acks, [](const auto& ack) {
        return ack.sequenceId == 6 && ack.phase == AudioAckPhase::Terminal &&
               ack.outcome == AudioAckOutcome::Silent;
    }));
    REQUIRE_FALSE(std::ranges::any_of(acks, [](const auto& ack) {
        return ack.sequenceId == 6 && ack.phase == AudioAckPhase::FirstNonZero;
    }));
}

TEST_CASE("Repeated starts coexist and barriers cancel before applying",
          "[RealtimeMixer][session]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    const auto initialStorage = mixer.storageFingerprint();
    advanceSilently(mixer, 100);
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 100 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 2,
           .voiceId = fixture.voice0,
           .targetFrame = 110 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 3,
           .voiceId = fixture.voice0,
           .targetFrame = 120 });
    push(fixture.transport,
         { .type = AudioCommandType::StopAll,
           .sessionGeneration = 1,
           .sequenceId = 4 });
    auto left = std::array<float, 8>{};
    auto right = left;
    mixer.render(left.data(), right.data(), left.size());
    REQUIRE(
      std::ranges::all_of(left, [](float value) { return value == 0.F; }));
    const auto acks = drain(fixture.transport);
    const auto appliedBarrier = std::ranges::find_if(acks, [](const auto& ack) {
        return ack.sequenceId == 4 && ack.phase == AudioAckPhase::Applied;
    });
    REQUIRE(appliedBarrier != acks.end());
    REQUIRE(
      std::ranges::count_if(acks.begin(), appliedBarrier, [](const auto& ack) {
          return (ack.sequenceId == 2 || ack.sequenceId == 3) &&
                 ack.phase == AudioAckPhase::Terminal &&
                 ack.outcome == AudioAckOutcome::Canceled;
      }) == 2);

    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 2,
           .sequenceId = 5,
           .targetFrame = 108 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 2,
           .sequenceId = 6,
           .voiceId = fixture.voice0,
           .targetFrame = 109 });
    mixer.render(left.data(), right.data(), 2);
    REQUIRE(left[0] == 0.F);
    REQUIRE(left[1] == Approx(0.5F));
    REQUIRE(mixer.storageFingerprint() != 0);
    REQUIRE(mixer.storageFingerprint() == initialStorage);

    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 2,
           .sequenceId = 7,
           .voiceId = fixture.voice0,
           .targetFrame = 130 });
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 3,
           .sequenceId = 8,
           .targetFrame = 110 });
    mixer.render(left.data(), right.data(), 1);
    const auto resetAcks = drain(fixture.transport);
    const auto appliedReset =
      std::ranges::find_if(resetAcks, [](const auto& ack) {
          return ack.sequenceId == 8 && ack.phase == AudioAckPhase::Applied;
      });
    REQUIRE(appliedReset != resetAcks.end());
    REQUIRE(
      std::ranges::any_of(resetAcks.begin(), appliedReset, [](const auto& ack) {
          return ack.sequenceId == 7 && ack.phase == AudioAckPhase::Terminal &&
                 ack.outcome == AudioAckOutcome::Canceled;
      }));
    REQUIRE(mixer.storageFingerprint() == initialStorage);

    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 3,
           .sequenceId = 9,
           .targetFrame = 111 });
    mixer.render(left.data(), right.data(), 1);
    REQUIRE(std::ranges::any_of(drain(fixture.transport), [](const auto& ack) {
        return ack.sequenceId == 9 && ack.phase == AudioAckPhase::Terminal &&
               ack.outcome == AudioAckOutcome::Rejected;
    }));
    REQUIRE(mixer.renderedFrames() == 112);
}

TEST_CASE("Mid-chart StopAll resolves active and pending correlations once",
          "[RealtimeMixer][session][ack]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    advanceSilently(mixer, 10);
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 10 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 2,
           .sourceInputId = 22,
           .voiceId = fixture.voice0,
           .targetFrame = 10,
           .sourceEventMonotonicUs = 123,
           .publishedMonotonicUs = 456 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 3,
           .voiceId = fixture.voice1,
           .targetFrame = 100 });
    auto left = std::array<float, 1>{};
    auto right = left;
    mixer.render(left.data(), right.data(), 1);
    (void)drain(fixture.transport);
    push(fixture.transport,
         { .type = AudioCommandType::StopAll,
           .sessionGeneration = 1,
           .sequenceId = 4 });
    mixer.render(left.data(), right.data(), 1);
    const auto acks = drain(fixture.transport);
    auto barrierPhases = std::vector<AudioAckPhase>{};
    for (const auto& ack : acks) {
        if (ack.sequenceId == 4) {
            barrierPhases.push_back(ack.phase);
        }
    }
    REQUIRE(barrierPhases == std::vector{ AudioAckPhase::Dequeued,
                                          AudioAckPhase::Applied,
                                          AudioAckPhase::Terminal });
    const auto applied = std::ranges::find_if(acks, [](const auto& ack) {
        return ack.sequenceId == 4 && ack.phase == AudioAckPhase::Applied;
    });
    REQUIRE(applied != acks.end());
    for (const auto sequence : { 2ULL, 3ULL }) {
        REQUIRE(std::ranges::count_if(
                  acks.begin(), applied, [sequence](const auto& ack) {
                      return ack.sequenceId == sequence &&
                             ack.phase == AudioAckPhase::Terminal &&
                             ack.outcome == AudioAckOutcome::Canceled;
                  }) == 1);
    }
    mixer.render(left.data(), right.data(), 1);
    REQUIRE(left[0] == 0.F);
}

TEST_CASE("Acknowledgement phases preserve exact provenance and lateness",
          "[RealtimeMixer][ack]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    advanceSilently(mixer, 100);
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 100 });
    auto scratchLeft = std::array<float, 3>{};
    auto scratchRight = scratchLeft;
    mixer.render(scratchLeft.data(), scratchRight.data(), scratchLeft.size());
    (void)drain(fixture.transport);
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 2,
           .sourceInputId = 77,
           .voiceId = fixture.voice0,
           .targetFrame = 101,
           .sourceEventMonotonicUs = 111,
           .publishedMonotonicUs = 222 });
    push(fixture.transport,
         { .type = AudioCommandType::Stop,
           .sessionGeneration = 1,
           .sequenceId = 3,
           .voiceId = fixture.voice1,
           .targetFrame = 103 });
    push(fixture.transport,
         { .type = AudioCommandType::SetVoiceGain,
           .sessionGeneration = 1,
           .sequenceId = 4,
           .voiceId = fixture.voice1,
           .targetFrame = 103,
           .value = 0.5F });
    auto left = std::array<float, 1>{};
    auto right = left;
    mixer.render(left.data(), right.data(), 1);
    const auto acks = drain(fixture.transport);
    const auto phasesFor = [&](std::uint64_t sequence) {
        auto phases = std::vector<AudioAckPhase>{};
        for (const auto& ack : acks) {
            if (ack.sequenceId == sequence) {
                phases.push_back(ack.phase);
            }
        }
        return phases;
    };
    REQUIRE(phasesFor(2) == std::vector{ AudioAckPhase::Dequeued,
                                         AudioAckPhase::Applied,
                                         AudioAckPhase::FirstNonZero });
    const auto applied = std::ranges::find_if(acks, [](const auto& ack) {
        return ack.sequenceId == 2 && ack.phase == AudioAckPhase::Applied;
    });
    REQUIRE(applied->observedFrame == 103);
    REQUIRE(applied->lateByFrames == 2);
    for (const auto& ack : acks) {
        if (ack.sequenceId != 2) {
            continue;
        }
        REQUIRE(ack.sourceInputId == 77);
        REQUIRE(ack.sourceEventMonotonicUs == 111);
        REQUIRE(ack.publishedMonotonicUs == 222);
        REQUIRE(ack.targetFrame == 101);
    }
    REQUIRE(phasesFor(3) == std::vector{ AudioAckPhase::Dequeued,
                                         AudioAckPhase::Applied,
                                         AudioAckPhase::Terminal });
    REQUIRE(phasesFor(4) == std::vector{ AudioAckPhase::Dequeued,
                                         AudioAckPhase::Applied,
                                         AudioAckPhase::Terminal });
}

TEST_CASE("Concurrent acknowledgement consumers observe published voice state",
          "[RealtimeMixer][ack][concurrency]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    auto stage = std::atomic_int{ 0 };
    auto consumerOk = std::atomic_bool{ true };
    auto stopConsumer = std::atomic_bool{ false };
    auto consumer = std::thread([&] {
        auto ack = AudioAcknowledgement{};
        while (!stopConsumer.load(std::memory_order_acquire)) {
            if (!fixture.transport.acknowledgements.tryPop(ack)) {
                std::this_thread::yield();
                continue;
            }
            if (ack.sequenceId == 2 && ack.phase == AudioAckPhase::Applied) {
                if (!fixture.transport.isVoicePlaying(fixture.voice0)) {
                    consumerOk.store(false, std::memory_order_release);
                }
                stage.store(1, std::memory_order_release);
            }
            if ((ack.sequenceId == 2 && ack.phase == AudioAckPhase::Terminal) ||
                (ack.sequenceId == 3 &&
                 (ack.phase == AudioAckPhase::Applied ||
                  ack.phase == AudioAckPhase::Terminal))) {
                if (fixture.transport.isVoicePlaying(fixture.voice0)) {
                    consumerOk.store(false, std::memory_order_release);
                }
            }
            if (ack.sequenceId == 3 && ack.phase == AudioAckPhase::Terminal) {
                stage.store(2, std::memory_order_release);
            }
            if (ack.sequenceId == 4 && ack.phase == AudioAckPhase::Applied) {
                if (!fixture.transport.isVoicePlaying(fixture.voice0)) {
                    consumerOk.store(false, std::memory_order_release);
                }
                stage.store(3, std::memory_order_release);
            }
            if (ack.sequenceId == 4 && ack.phase == AudioAckPhase::Terminal) {
                if (fixture.transport.isVoicePlaying(fixture.voice0)) {
                    consumerOk.store(false, std::memory_order_release);
                }
                stage.store(4, std::memory_order_release);
            }
        }
    });
    const auto waitForStage = [&](int expected) {
        const auto deadline = std::chrono::steady_clock::now() + 5s;
        while (stage.load(std::memory_order_acquire) < expected &&
               std::chrono::steady_clock::now() < deadline) {
            std::this_thread::yield();
        }
        return stage.load(std::memory_order_acquire) >= expected;
    };

    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 0 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 2,
           .voiceId = fixture.voice0,
           .targetFrame = 0 });
    auto left = std::array<float, 3>{};
    auto right = left;
    mixer.render(left.data(), right.data(), 1);
    const auto sawStart = waitForStage(1);

    push(fixture.transport,
         { .type = AudioCommandType::Stop,
           .sessionGeneration = 1,
           .sequenceId = 3,
           .voiceId = fixture.voice0,
           .targetFrame = 1 });
    mixer.render(left.data(), right.data(), 1);
    const auto sawStop = waitForStage(2);

    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 4,
           .voiceId = fixture.voice0,
           .targetFrame = 2 });
    mixer.render(left.data(), right.data(), 1);
    const auto sawNaturalStart = waitForStage(3);
    mixer.render(left.data(), right.data(), 3);
    const auto sawNaturalTerminal = waitForStage(4);

    stopConsumer.store(true, std::memory_order_release);
    consumer.join();
    REQUIRE(sawStart);
    REQUIRE(sawStop);
    REQUIRE(sawNaturalStart);
    REQUIRE(sawNaturalTerminal);
    REQUIRE(consumerOk.load(std::memory_order_acquire));
}

TEST_CASE("Command and scheduler overflow are terminal and correlated",
          "[RealtimeMixer][overflow]")
{
    auto transport = AudioTransport{};
    auto rejected = AudioCommand{};
    rejected.sessionGeneration = 9;
    for (std::size_t i = 0; i < AudioTransport::commandCapacity; ++i) {
        auto command = AudioCommand{};
        command.sequenceId = i + 1;
        REQUIRE(transport.tryPublish(command));
    }
    rejected.sequenceId = AudioTransport::commandCapacity + 1;
    REQUIRE_FALSE(transport.tryPublish(rejected));
    REQUIRE(transport.terminalError().code ==
            AudioTerminalErrorCode::CommandRingOverflow);
    REQUIRE(transport.terminalError().sequenceId == rejected.sequenceId);

    auto fixture = Fixture{};
    auto mixer = fixture.mixer(1);
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 0 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 2,
           .voiceId = fixture.voice0,
           .targetFrame = 100 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 3,
           .voiceId = fixture.voice1,
           .targetFrame = 101 });
    auto left = std::array<float, 1>{};
    auto right = left;
    mixer.render(left.data(), right.data(), 1);
    REQUIRE(fixture.transport.terminalError().code ==
            AudioTerminalErrorCode::SchedulerOverflow);
}

TEST_CASE("Acknowledgement overflow is a terminal realtime failure",
          "[RealtimeMixer][overflow]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    auto filler = AudioAcknowledgement{};
    for (std::size_t i = 0; i < AudioTransport::acknowledgementCapacity; ++i) {
        REQUIRE(fixture.transport.acknowledgements.tryPush(filler));
    }
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 991,
           .targetFrame = 0 });
    auto left = std::array<float, 1>{};
    auto right = left;
    mixer.render(left.data(), right.data(), 1);
    REQUIRE(fixture.transport.terminalError().code ==
            AudioTerminalErrorCode::AcknowledgementRingOverflow);
    REQUIRE(fixture.transport.terminalError().sequenceId == 991);
}

TEST_CASE("Render performs no allocation and retains preallocated storage",
          "[RealtimeMixer][realtime]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 0 });
    push(fixture.transport,
         { .type = AudioCommandType::Start,
           .sessionGeneration = 1,
           .sequenceId = 2,
           .voiceId = fixture.voice0,
           .targetFrame = 0 });
    auto left = std::array<float, 128>{};
    auto right = left;
    const auto storage = mixer.storageFingerprint();
    const auto before = allocation_probe::count.load(std::memory_order_relaxed);
    mixer.render(left.data(), right.data(), left.size());
    const auto after = allocation_probe::count.load(std::memory_order_relaxed);
    REQUIRE(after == before);
    REQUIRE(mixer.storageFingerprint() == storage);
}

TEST_CASE("Dstorv-sized pre-schedule burst fits transport and scheduler",
          "[RealtimeMixer][Dstorv]")
{
    auto bank = PcmSoundBank{ 48'000 };
    const auto clip = bank.addClip("click", mono({ 0.1F }));
    for (std::uint64_t key = 1; key <= 630; ++key) {
        REQUIRE(bank.addVoice(key, clip) == key - 1);
    }
    bank.freeze();
    REQUIRE(bank.voiceCount() == 630);
    auto transport = AudioTransport{};
    auto mixer = RealtimeMixer(bank,
                               transport,
                               { .outputSampleRate = 48'000,
                                 .voiceCapacity = 630,
                                 .scheduledEventCapacity = 1'700,
                                 .authoredBgmEventCount = 1'629,
                                 .liveCommandHeadroom = 64 });
    push(transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 0 });
    auto usedVoices = std::array<bool, 411>{};
    for (std::uint64_t i = 0; i < 1'629; ++i) {
        const auto voice = static_cast<VoiceId>(i % usedVoices.size());
        usedVoices[voice] = true;
        push(transport,
             { .type = AudioCommandType::Start,
               .sessionGeneration = 1,
               .sequenceId = i + 2,
               .voiceId = voice,
               .targetFrame = 10'000 + i });
    }
    auto left = std::array<float, 1>{};
    auto right = left;
    mixer.render(left.data(), right.data(), 1);
    REQUIRE(mixer.scheduledEventCount() == 1'629);
    REQUIRE(std::ranges::count(usedVoices, true) == 411);
    REQUIRE(transport.terminalError().code == AudioTerminalErrorCode::None);
}

TEST_CASE("Scheduled sound publishes chart frames with explicit provenance",
          "[RealtimeMixer][ScheduledPcmSound]")
{
    auto fixture = Fixture{};
    fixture.transport.setCurrentOutputFrame(777);
    fixture.transport.beginSession(7, 1'000, 48'000);
    REQUIRE(fixture.transport.currentOutputFrame() == 777);
    fixture.transport.setCommandProvenance({ .sourceInputId = 123,
                                             .sourceEventMonotonicUs = 45,
                                             .publishedMonotonicUs = 46 });
    auto sound = ScheduledPcmSound{ fixture.transport, fixture.voice0 };
    sound.playAt(1ms);
    auto command = AudioCommand{};
    REQUIRE(fixture.transport.commands.tryPop(command));
    REQUIRE(command.targetFrame == 1'048);
    REQUIRE(command.sessionGeneration == 7);
    REQUIRE(command.sourceInputId == 123);
    sound.setVolume(0.25F);
    REQUIRE(sound.getVolume() == Approx(1.F));
}

TEST_CASE("Scheduled frame conversion rounds with integer saturation",
          "[RealtimeMixer][ScheduledPcmSound]")
{
    REQUIRE(detail::chartTimeToFrame(
              (std::numeric_limits<std::uint64_t>::max)() - 2,
              (std::numeric_limits<std::int64_t>::max)(),
              192'000) == (std::numeric_limits<std::uint64_t>::max)());
    REQUIRE(detail::chartTimeToFrame(100, 10'416, 48'000) == 100);
    REQUIRE(detail::chartTimeToFrame(100, 10'417, 48'000) == 101);
}

TEST_CASE("Invalid start, stop, and gain voice IDs are rejected before apply",
          "[RealtimeMixer][ack]")
{
    auto fixture = Fixture{};
    auto mixer = fixture.mixer();
    push(fixture.transport,
         { .type = AudioCommandType::ResetSession,
           .sessionGeneration = 1,
           .sequenceId = 1,
           .targetFrame = 0 });
    for (const auto type : { AudioCommandType::Start,
                             AudioCommandType::Stop,
                             AudioCommandType::SetVoiceGain }) {
        const auto sequence =
          type == AudioCommandType::Start
            ? 2ULL
            : (type == AudioCommandType::Stop ? 3ULL : 4ULL);
        push(fixture.transport,
             { .type = type,
               .sessionGeneration = 1,
               .sequenceId = sequence,
               .voiceId = 99,
               .targetFrame = 0,
               .value = 0.5F });
    }
    auto left = std::array<float, 1>{};
    auto right = left;
    mixer.render(left.data(), right.data(), 1);
    const auto acks = drain(fixture.transport);
    for (const auto sequence : { 2ULL, 3ULL, 4ULL }) {
        REQUIRE_FALSE(std::ranges::any_of(acks, [sequence](const auto& ack) {
            return ack.sequenceId == sequence &&
                   ack.phase == AudioAckPhase::Applied;
        }));
        REQUIRE(std::ranges::any_of(acks, [sequence](const auto& ack) {
            return ack.sequenceId == sequence &&
                   ack.phase == AudioAckPhase::Terminal &&
                   ack.outcome == AudioAckOutcome::Rejected;
        }));
    }
}

TEST_CASE("Invalid mixer config fails before allocating storage",
          "[RealtimeMixer][config]")
{
    auto fixture = Fixture{};
    REQUIRE_THROWS_AS(
      RealtimeMixer(
        fixture.bank,
        fixture.transport,
        { .outputSampleRate = 48'000,
          .voiceCapacity = (std::numeric_limits<std::size_t>::max)(),
          .scheduledEventCapacity = (std::numeric_limits<std::size_t>::max)(),
          .authoredBgmEventCount = (std::numeric_limits<std::size_t>::max)(),
          .liveCommandHeadroom = 1 }),
      std::invalid_argument);
    REQUIRE(fixture.transport.terminalError().code ==
            AudioTerminalErrorCode::InvalidConfiguration);
}

TEST_CASE("Scheduled sound takes a coherent generation-anchor snapshot",
          "[RealtimeMixer][ScheduledPcmSound][concurrency]")
{
    auto transport = AudioTransport{};
    transport.beginSession(1, 1'000, 48'000);
    auto sound = ScheduledPcmSound{ transport, 0 };
    auto resetter = std::thread([&] {
        for (std::uint64_t generation = 2; generation <= 1'000; ++generation) {
            transport.beginSession(generation, generation * 1'000, 48'000);
        }
    });
    for (std::size_t i = 0; i < 1'000; ++i) {
        sound.playAt(1ms);
    }
    resetter.join();
    auto command = AudioCommand{};
    auto count = std::size_t{};
    while (transport.commands.tryPop(command)) {
        REQUIRE(command.targetFrame == command.sessionGeneration * 1'000 + 48);
        ++count;
    }
    REQUIRE(count == 1'000);
    REQUIRE(transport.terminalError().code == AudioTerminalErrorCode::None);
}

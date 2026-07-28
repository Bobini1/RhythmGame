#pragma once

#include "gameplay_logic/SinglePlayerGameplayCore.h"
#include "input/BmsKeys.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

namespace web_playtest {

enum class InputPreset : std::uint8_t
{
    Native,
    Lr2
};

enum class RuntimePhase : std::uint8_t
{
    InstallingChart,
    Decoding,
    Ready,
    Countdown,
    Playing,
    Finished,
    Aborted,
    Error
};

enum class BrowserControl : std::uint8_t
{
    Gameplay,
    Start,
    Abort
};

struct MappedBrowserCode
{
    BrowserControl control = BrowserControl::Gameplay;
    input::BmsKey key = input::BmsKey::Col11;
    std::uint8_t pressedSlot = {};
};

struct BrowserKeyTransition
{
    input::BmsKey key = input::BmsKey::Col11;
    gameplay_logic::GameplayKeyAction action =
      gameplay_logic::GameplayKeyAction::Press;
};

[[nodiscard]] auto
mapBrowserCode(std::string_view code, InputPreset preset) noexcept
  -> std::optional<MappedBrowserCode>;

class BrowserInputDeduplicator final
{
  public:
    static constexpr auto gameplayCodeCount = std::size_t{ 9 };

    [[nodiscard]] auto apply(const MappedBrowserCode& mapping,
                             bool pressed,
                             bool repeat) noexcept
      -> std::optional<BrowserKeyTransition>;
    [[nodiscard]] auto synthesizeReleases(
      InputPreset preset,
      std::array<BrowserKeyTransition, gameplayCodeCount>& releases) noexcept
      -> std::size_t;
    void clear() noexcept;
    [[nodiscard]] auto anyPressed() const noexcept -> bool;

  private:
    std::array<bool, gameplayCodeCount> pressedCodes = {};
};

struct InputEvent
{
    std::uint64_t sequenceId = {};
    input::BmsKey key = input::BmsKey::Col11;
    gameplay_logic::GameplayKeyAction action =
      gameplay_logic::GameplayKeyAction::Press;
    std::int64_t browserMonotonicUs = {};
};

enum class RuntimeCommandType : std::uint8_t
{
    Input,
    Tick,
    StartSession,
    Abort
};

struct RuntimeCommand
{
    RuntimeCommandType type = RuntimeCommandType::Tick;
    InputEvent input;
    std::uint64_t sequenceId = {};
    std::uint64_t sessionGeneration = {};
    std::uint64_t chartStartFrame = {};
    std::int64_t browserMonotonicUs = {};
    std::uint32_t outputSampleRate = {};
};

class GameplayTimestampWatermark final
{
  public:
    [[nodiscard]] auto clamp(std::int64_t mappedChartTimeNs) noexcept
      -> std::int64_t;
    [[nodiscard]] auto lateInputClampNs() const noexcept -> std::uint64_t;
    void reset() noexcept;

  private:
    std::int64_t watermarkNs = {};
    std::uint64_t accumulatedClampNs = {};
    bool initialized = {};
};

template<std::size_t Capacity>
class RuntimeCommandQueue final
{
    static_assert(Capacity > 1);
    static_assert(std::is_nothrow_copy_assignable_v<RuntimeCommand>);

  public:
    [[nodiscard]] auto tryPush(const RuntimeCommand& command) noexcept -> bool
    {
        const auto write = writePosition.load(std::memory_order_relaxed);
        const auto read = readPosition.load(std::memory_order_acquire);
        if (write - read >= Capacity) {
            dropped.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        storage[write % Capacity] = command;
        writePosition.store(write + 1, std::memory_order_release);
        writePosition.notify_one();
        return true;
    }

    [[nodiscard]] auto tryPop(RuntimeCommand& command) noexcept -> bool
    {
        const auto read = readPosition.load(std::memory_order_relaxed);
        const auto write = writePosition.load(std::memory_order_acquire);
        if (read == write) {
            return false;
        }
        command = storage[read % Capacity];
        readPosition.store(read + 1, std::memory_order_release);
        return true;
    }

    void waitForData() const noexcept
    {
        const auto observed = writePosition.load(std::memory_order_acquire);
        if (readPosition.load(std::memory_order_acquire) == observed) {
            writePosition.wait(observed, std::memory_order_relaxed);
        }
    }

    [[nodiscard]] auto droppedCount() const noexcept -> std::uint64_t
    {
        return dropped.load(std::memory_order_acquire);
    }

    [[nodiscard]] auto empty() const noexcept -> bool
    {
        return readPosition.load(std::memory_order_acquire) ==
               writePosition.load(std::memory_order_acquire);
    }

  private:
    alignas(64) std::array<RuntimeCommand, Capacity> storage = {};
    alignas(64) mutable std::atomic<std::uint64_t> writePosition = {};
    alignas(64) std::atomic<std::uint64_t> readPosition = {};
    std::atomic<std::uint64_t> dropped = {};
};

static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
static_assert(std::atomic<std::int64_t>::is_always_lock_free);

} // namespace web_playtest

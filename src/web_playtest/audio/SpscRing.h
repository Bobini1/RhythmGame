#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace web_playtest {

template<typename T, std::size_t Capacity>
class SpscRing
{
    static_assert(Capacity > 0);
    static_assert(std::is_nothrow_copy_assignable_v<T>);

  public:
    SpscRing()
      : storage(std::make_unique<T[]>(Capacity))
    {
    }
    SpscRing(const SpscRing&) = delete;
    auto operator=(const SpscRing&) -> SpscRing& = delete;
    SpscRing(SpscRing&&) = delete;
    auto operator=(SpscRing&&) -> SpscRing& = delete;

    [[nodiscard]] auto tryPush(const T& value) noexcept -> bool
    {
        const auto write = writePosition.load(std::memory_order_relaxed);
        const auto read = readPosition.load(std::memory_order_acquire);
        if (write - read >= Capacity) {
            return false;
        }
        storage[write % Capacity] = value;
        writePosition.store(write + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] auto tryPop(T& value) noexcept -> bool
    {
        const auto read = readPosition.load(std::memory_order_relaxed);
        const auto write = writePosition.load(std::memory_order_acquire);
        if (read == write) {
            return false;
        }
        value = storage[read % Capacity];
        readPosition.store(read + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] auto size() const noexcept -> std::size_t
    {
        const auto write = writePosition.load(std::memory_order_acquire);
        const auto read = readPosition.load(std::memory_order_acquire);
        return static_cast<std::size_t>(write - read);
    }
    [[nodiscard]] auto empty() const noexcept -> bool { return size() == 0; }
    [[nodiscard]] static consteval auto capacity() noexcept -> std::size_t
    {
        return Capacity;
    }

  private:
    alignas(64) std::unique_ptr<T[]> storage;
    alignas(64) std::atomic<std::uint64_t> writePosition{};
    alignas(64) std::atomic<std::uint64_t> readPosition{};
};

} // namespace web_playtest

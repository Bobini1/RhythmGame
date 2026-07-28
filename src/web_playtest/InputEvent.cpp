#include "InputEvent.h"

#include <algorithm>
#include <functional>
#include <limits>

namespace web_playtest {
namespace {

struct CodeEntry
{
    std::string_view code;
    input::BmsKey key;
    std::uint8_t slot;
};

constexpr auto nativeMappings = std::array{
    CodeEntry{ "ShiftLeft", input::BmsKey::Col1sUp, 0 },
    CodeEntry{ "ControlLeft", input::BmsKey::Col1sDown, 1 },
    CodeEntry{ "KeyA", input::BmsKey::Col11, 2 },
    CodeEntry{ "KeyS", input::BmsKey::Col12, 3 },
    CodeEntry{ "KeyD", input::BmsKey::Col13, 4 },
    CodeEntry{ "Space", input::BmsKey::Col14, 5 },
    CodeEntry{ "KeyJ", input::BmsKey::Col15, 6 },
    CodeEntry{ "KeyK", input::BmsKey::Col16, 7 },
    CodeEntry{ "KeyL", input::BmsKey::Col17, 8 },
};

constexpr auto lr2Mappings = std::array{
    CodeEntry{ "ShiftLeft", input::BmsKey::Col1sUp, 0 },
    CodeEntry{ "KeyZ", input::BmsKey::Col11, 2 },
    CodeEntry{ "KeyS", input::BmsKey::Col12, 3 },
    CodeEntry{ "KeyX", input::BmsKey::Col13, 4 },
    CodeEntry{ "KeyD", input::BmsKey::Col14, 5 },
    CodeEntry{ "KeyC", input::BmsKey::Col15, 6 },
    CodeEntry{ "KeyF", input::BmsKey::Col16, 7 },
    CodeEntry{ "KeyV", input::BmsKey::Col17, 8 },
};

template<std::size_t Size>
auto
findMapping(std::string_view code,
            const std::array<CodeEntry, Size>& entries) noexcept
  -> std::optional<MappedBrowserCode>
{
    const auto found = std::ranges::find(entries, code, &CodeEntry::code);
    if (found == entries.end()) {
        return std::nullopt;
    }
    return MappedBrowserCode{ .control = BrowserControl::Gameplay,
                              .key = found->key,
                              .pressedSlot = found->slot };
}

auto
keyForSlot(InputPreset preset, std::size_t slot) noexcept
  -> std::optional<input::BmsKey>
{
    if (preset == InputPreset::Native) {
        const auto found =
          std::ranges::find(nativeMappings, slot, &CodeEntry::slot);
        return found == nativeMappings.end()
                 ? std::nullopt
                 : std::optional<input::BmsKey>{ found->key };
    }
    const auto found = std::ranges::find(lr2Mappings, slot, &CodeEntry::slot);
    return found == lr2Mappings.end()
             ? std::nullopt
             : std::optional<input::BmsKey>{ found->key };
}

void
saturatingAdd(std::uint64_t& destination, std::uint64_t value) noexcept
{
    const auto maximum = (std::numeric_limits<std::uint64_t>::max)();
    destination = value > maximum - destination ? maximum : destination + value;
}

} // namespace

auto
mapBrowserCode(std::string_view code, InputPreset preset) noexcept
  -> std::optional<MappedBrowserCode>
{
    if (code == "Enter") {
        return MappedBrowserCode{ .control = BrowserControl::Start };
    }
    if (code == "Escape") {
        return MappedBrowserCode{ .control = BrowserControl::Abort };
    }
    return preset == InputPreset::Native ? findMapping(code, nativeMappings)
                                         : findMapping(code, lr2Mappings);
}

auto
BrowserInputDeduplicator::apply(const MappedBrowserCode& mapping,
                                bool pressed,
                                bool repeat) noexcept
  -> std::optional<BrowserKeyTransition>
{
    if (mapping.control != BrowserControl::Gameplay ||
        mapping.pressedSlot >= pressedCodes.size() || (pressed && repeat) ||
        pressedCodes[mapping.pressedSlot] == pressed) {
        return std::nullopt;
    }
    pressedCodes[mapping.pressedSlot] = pressed;
    return BrowserKeyTransition{
        .key = mapping.key,
        .action = pressed ? gameplay_logic::GameplayKeyAction::Press
                          : gameplay_logic::GameplayKeyAction::Release,
    };
}

auto
BrowserInputDeduplicator::synthesizeReleases(
  InputPreset preset,
  std::array<BrowserKeyTransition, gameplayCodeCount>& releases) noexcept
  -> std::size_t
{
    auto count = std::size_t{};
    for (auto slot = std::size_t{}; slot < pressedCodes.size(); ++slot) {
        if (!pressedCodes[slot]) {
            continue;
        }
        pressedCodes[slot] = false;
        if (const auto key = keyForSlot(preset, slot)) {
            releases[count++] = {
                .key = *key,
                .action = gameplay_logic::GameplayKeyAction::Release,
            };
        }
    }
    return count;
}

void
BrowserInputDeduplicator::clear() noexcept
{
    pressedCodes.fill(false);
}

auto
BrowserInputDeduplicator::anyPressed() const noexcept -> bool
{
    return std::ranges::any_of(pressedCodes, std::identity{});
}

auto
GameplayTimestampWatermark::clamp(std::int64_t mappedChartTimeNs) noexcept
  -> std::int64_t
{
    if (!initialized || mappedChartTimeNs >= watermarkNs) {
        initialized = true;
        watermarkNs = mappedChartTimeNs;
        return mappedChartTimeNs;
    }
    const auto delta = static_cast<std::uint64_t>(watermarkNs) -
                       static_cast<std::uint64_t>(mappedChartTimeNs);
    saturatingAdd(accumulatedClampNs, delta);
    return watermarkNs;
}

auto
GameplayTimestampWatermark::lateInputClampNs() const noexcept -> std::uint64_t
{
    return accumulatedClampNs;
}

void
GameplayTimestampWatermark::reset() noexcept
{
    watermarkNs = 0;
    accumulatedClampNs = 0;
    initialized = false;
}

} // namespace web_playtest

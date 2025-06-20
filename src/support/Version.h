//
// Created by PC on 20/06/2025.
//

#ifndef VERSION_H
#define VERSION_H
#include <cstdint>
#include <tuple>

namespace support {
constexpr auto
packVersion(const uint32_t major, const uint32_t minor, const uint32_t patch)
  -> int64_t
{
    return (static_cast<uint64_t>(major) << 40) |
           (static_cast<uint64_t>(minor) << 20) | static_cast<uint64_t>(patch);
}

constexpr auto
unpackVersion(const uint64_t version)
  -> std::tuple<uint32_t, uint32_t, uint32_t>
{
    const auto major = static_cast<int32_t>((version >> 40) & 0xFFFFF);
    const auto minor = static_cast<int32_t>((version >> 20) & 0xFFFFF);
    const auto patch = static_cast<int32_t>(version & 0xFFFFF);
    return { static_cast<uint32_t>(major),
             static_cast<uint32_t>(minor),
             static_cast<uint32_t>(patch) };
}

constexpr uint64_t currentVersion = packVersion(RHYTHMGAME_VERSION_MAJOR,
                                                RHYTHMGAME_VERSION_MINOR,
                                                RHYTHMGAME_VERSION_PATCH);
} // namespace support

#endif // VERSION_H

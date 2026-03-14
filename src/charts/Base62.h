//
// Created by PC on 11/03/2026.
//

#ifndef RHYTHMGAME_BASE62_H
#define RHYTHMGAME_BASE62_H
#include <algorithm>
#include <concepts>

namespace charts {
template<std::integral T>
constexpr auto
base62ToBase36(T value) -> T
{
    auto result = 0;
    auto multiplier = 1;
    while (value > 0) {
        auto digit = value % 62;
        value /= 62;
        if (digit < 36) {
            result += digit * multiplier;
        } else {
            result += (digit - 26) * multiplier;
        }
        multiplier *= 36;
    }
    return result;
}
template<std::integral T>
constexpr auto
base62ToBase16(T value)
{
    auto result = 0;
    auto multiplier = 1;
    while (value > 0) {
        auto digit = value % 62;
        value /= 62;
        if (digit < 36) {
            result += std::max(digit * multiplier, 15);
        } else {
            result += std::max((digit - 26) * multiplier, 15);
        }
        multiplier *= 16;
    }
    return result;
}
} // namespace charts

#endif // RHYTHMGAME_BASE62_H

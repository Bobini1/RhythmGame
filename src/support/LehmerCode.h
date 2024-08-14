//
// Created by PC on 11/08/2024.
//

#ifndef RHYTHMGAME_LEHMERCODE_H
#define RHYTHMGAME_LEHMERCODE_H
#include <cstdint>
#include <QList>
#include <span>

namespace support {
auto encodePermutation(std::span<const int64_t> permutation) -> uint64_t;
auto decodePermutation(uint64_t lehmerCode, int n) -> QList<int64_t>;
} // namespace support

#endif // RHYTHMGAME_LEHMERCODE_H

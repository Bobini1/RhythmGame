//
// Created by PC on 11/08/2024.
//

#ifndef RHYTHMGAME_LEHMERCODE_H
#define RHYTHMGAME_LEHMERCODE_H
#include <cstdint>
#include <QList>
#include <span>

namespace support {
uint64_t encodePermutation(std::span<const int> permutation);
QList<int> decodePermutation(uint64_t lehmerCode, int n);
} // namespace support

#endif // RHYTHMGAME_LEHMERCODE_H

//
// Created by PC on 01/10/2024.
//

#ifndef GENERATEPERMUTATION_H
#define GENERATEPERMUTATION_H
#include "gameplay_logic/BmsNotes.h"

#include <QList>
#include <random>
#include <ranges>
#include "resource_managers/Vars.h"
#include "charts/BmsNotesData.h"

namespace support {
struct ShuffleResult
{
    uint64_t seed;
    QList<int> columns;
};

/**
 * @brief Generate a permutation of notes.
 * @details The permutation is generated using the given algorithm and seed.
 * @warning For S-Random, this function relies on total stability of note
 * generation. If we ever change how we load notes, replays WILL break.
 * @param notes The notes to shuffle.
 * @param algorithm The algorithm to use.
 * @param seed The seed to use.
 * @param k5 Whether to treat the notes 5k as 7k (legacy behavior)
 * @param usePre130 Whether to use the broken pre-1.3.0 version of the s-random
 * shuffle algorithm (legacy behavior)
 * @return The result of the shuffle, including the new seed and the new column
 * order.
 */
auto
generatePermutation(std::span<std::vector<charts::BmsNotesData::Note>>& notes,
                    resource_managers::NoteOrderAlgorithm algorithm,
                    uint64_t seed,
                    bool k5,
                    bool usePre130) -> ShuffleResult;
} // namespace support

#endif // GENERATEPERMUTATION_H

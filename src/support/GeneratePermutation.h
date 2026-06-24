//
// Created by PC on 01/10/2024.
//

#ifndef GENERATEPERMUTATION_H
#define GENERATEPERMUTATION_H
#include "gameplay_logic/BmsNotes.h"

#include <QList>
#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <ranges>
#include <span>
#include <vector>
#include "resource_managers/Vars.h"
#include "charts/BmsNotesData.h"

namespace support {
class Lr2Random final
{
    static constexpr auto stateSize = std::size_t{ 624 };

    std::array<uint32_t, stateSize> state{};
    std::size_t index{ stateSize };
    uint32_t initialSeed;

    auto getNextLong() -> uint32_t;
    auto generateNextSet() -> void;

  public:
    explicit Lr2Random(uint32_t seed);

    auto getRand(int maxInclusive) -> int;
    auto discard(std::size_t draws) -> void;
    [[nodiscard]] auto getInitialSeed() const -> uint32_t;
};

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

auto
generateBeatorajaLanePermutation(
  std::span<std::vector<charts::BmsNotesData::Note>> notes,
  resource_managers::NoteOrderAlgorithm algorithm,
  int64_t seed) -> ShuffleResult;

auto
generateLr2LanePermutation(
  std::span<std::vector<charts::BmsNotesData::Note>> notes,
  resource_managers::NoteOrderAlgorithm algorithm,
  Lr2Random& rng) -> ShuffleResult;

auto
isBeatorajaNoteOrderAlgorithm(resource_managers::NoteOrderAlgorithm algorithm)
  -> bool;

auto
isLr2NoteOrderAlgorithm(resource_managers::NoteOrderAlgorithm algorithm)
  -> bool;

auto
flipBeatorajaDpPlayfields(
  std::array<std::vector<charts::BmsNotesData::Note>,
             charts::BmsNotesData::columnNumber>& notes) -> void;

auto
flipLr2DpPlayfields(std::array<std::vector<charts::BmsNotesData::Note>,
                               charts::BmsNotesData::columnNumber>& notes)
  -> void;

} // namespace support

#endif // GENERATEPERMUTATION_H

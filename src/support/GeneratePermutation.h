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
#include "charts/gameplay_models/BmsNotesData.h"

#include <set>
#include <boost/icl/interval_set.hpp>

namespace support {

template<typename Arr, typename Random>
void
fisherYatesShuffle(Arr arr, Random& randomGenerator)
{
    for (auto i = arr.size() - 1; i > 0; --i) {
        // we shouldn't need to support more than 256 columns
        auto distribution = std::uniform_int_distribution<uint8_t>(0, i);
        const auto j = distribution(randomGenerator);
        std::swap(arr[i], arr[j]);
    }
}

template<typename Random>
void
fisherYatesShuffle(
  std::span<std::vector<charts::gameplay_models::BmsNotesData::Note>> arr,
  Random& randomGenerator)
{
    // using an interval set to handle long notes more easily
    auto takenSpots =
      std::vector<boost::icl::interval_set<std::chrono::nanoseconds>>{};
    for (auto i = arr.size() - 1; i > 0; --i) {
        auto distribution = std::uniform_int_distribution<uint8_t>(0, i);
        const auto j = distribution(randomGenerator);
        std::swap(arr[i], arr[j]);
    }
}

auto
getColumsIota(const int size) -> QList<int>
{
    auto columns = QList<int>{};
    for (auto i = 0; i < size; ++i) {
        columns.append(i);
    }
    return columns;
}

struct ShuffleResult
{
    QList<int> permutation;
    uint64_t seed{};
};

ShuffleResult
shuffleColumns(
  std::span<std::vector<charts::gameplay_models::BmsNotesData::Note>>&
    visibleNotes,
  std::span<std::vector<charts::gameplay_models::BmsNotesData::Note>>&
    invisibleNotes,
  const resource_managers::NoteOrderAlgorithm algorithm,
  const std::optional<uint64_t> seed = std::nullopt)
{
    using RandomGenerator = std::mersenne_twister_engine<std::uint64_t,
                                                         32,
                                                         624,
                                                         397,
                                                         31,
                                                         0x9908b0df,
                                                         11,
                                                         0xffffffff,
                                                         7,
                                                         0x9d2c5680,
                                                         15,
                                                         0xefc60000,
                                                         18,
                                                         1812433253>;
    thread_local std::random_device rd;

    if (algorithm == resource_managers::NoteOrderAlgorithm::Normal) {
        auto columns = getColumsIota(visibleNotes.size());
        return { .permutation = columns };
    }
    if (algorithm == resource_managers::NoteOrderAlgorithm::Mirror) {
        auto columns = getColumsIota(visibleNotes.size());
        std::reverse(columns.begin(), columns.end() - 1);
        std::reverse(visibleNotes.begin(), visibleNotes.end() - 1);
        std::reverse(invisibleNotes.begin(), invisibleNotes.end() - 1);
        return { .permutation = columns };
    }
    if (algorithm == resource_managers::NoteOrderAlgorithm::Random) {
        const auto randomSeed = seed.has_value() ? seed.value() : rd();
        auto columns = getColumsIota(visibleNotes.size());
        auto randomGenerator = RandomGenerator{ randomSeed };
        fisherYatesShuffle(std::span(columns).subspan(0, columns.size() - 1),
                           randomGenerator);
        randomGenerator.seed(randomSeed);
        fisherYatesShuffle(
          std::span(visibleNotes).subspan(0, visibleNotes.size() - 1),
          randomGenerator);
        randomGenerator.seed(randomSeed);
        fisherYatesShuffle(
          std::span(invisibleNotes).subspan(0, invisibleNotes.size() - 1),
          randomGenerator);
        return { .permutation = columns, .seed = randomSeed };
    }
    if (algorithm == resource_managers::NoteOrderAlgorithm::RandomPlus) {
        const auto randomSeed = seed.has_value() ? seed.value() : rd();
        const auto columns = getColumsIota(visibleNotes.size());
        auto randomGenerator = RandomGenerator{ randomSeed };
        fisherYatesShuffle(columns, randomGenerator);
        randomGenerator.seed(randomSeed);
        fisherYatesShuffle(visibleNotes, randomGenerator);
        randomGenerator.seed(randomSeed);
        fisherYatesShuffle(invisibleNotes, randomGenerator);
        return { columns, randomSeed };
    }
    if (algorithm == resource_managers::NoteOrderAlgorithm::SRandom) {
        const auto randomSeed = seed.has_value() ? seed.value() : rd();
        auto columns = QList<int>{};
    }
}
} // namespace support

#endif // GENERATEPERMUTATION_H

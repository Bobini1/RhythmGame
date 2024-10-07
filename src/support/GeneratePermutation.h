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

#include <spdlog/spdlog.h>

using namespace std::chrono_literals;
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

inline auto
getColumsIota(const int size) -> QList<int>
{
    auto columns = QList<int>{};
    columns.reserve(size);
    for (auto i = 0; i < size; ++i) {
        columns.append(i);
    }
    return columns;
}

inline auto
getNextNote(
  const std::span<std::vector<charts::gameplay_models::BmsNotesData::Note>> arr,
  std::vector<
    std::vector<charts::gameplay_models::BmsNotesData::Note>::const_iterator>&
    noteIters)
  -> std::optional<
    std::vector<charts::gameplay_models::BmsNotesData::Note>::const_iterator>
{
    auto nextNote = std::optional<std::vector<
      charts::gameplay_models::BmsNotesData::Note>::const_iterator>{};
    auto selectedColumn = 0;
    for (auto i = 0; i < noteIters.size(); ++i) {
        if (noteIters[i] == arr[i].cend()) {
            continue;
        }
        if (!nextNote ||
            noteIters[i]->time.timestamp < (*nextNote)->time.timestamp) {
            nextNote = noteIters[i];
            selectedColumn = i;
        }
    }
    if (nextNote) {
        ++noteIters[selectedColumn];
        if ((*nextNote)->noteType ==
            charts::gameplay_models::BmsNotesData::NoteType::LongNoteBegin) {
            ++noteIters[selectedColumn];
        }
    }
    return nextNote;
}

inline void
pushProposedNote(
  std::vector<charts::gameplay_models::BmsNotesData::Note>& lane,
  const std::vector<
    charts::gameplay_models::BmsNotesData::Note>::const_iterator& note)
{
    lane.push_back(*note);
    if (note->noteType ==
        charts::gameplay_models::BmsNotesData::NoteType::LongNoteBegin) {
        auto endNote = note;
        ++endNote;
        lane.push_back(*endNote);
    }
}

inline auto
tryPushingNoteWithDistance(
  const std::vector<
    charts::gameplay_models::BmsNotesData::Note>::const_iterator& noteIt,
  const QList<int>& columnsIota,
  std::vector<std::vector<charts::gameplay_models::BmsNotesData::Note>>&
    newColumns,
  const std::chrono::nanoseconds preferredNoteDistance) -> bool
{
    for (const auto& proposedColumn : columnsIota) {
        auto& proposedColumnNotes = newColumns[proposedColumn];
        if (proposedColumnNotes.empty()) {
            pushProposedNote(proposedColumnNotes, noteIt);
            return true;
        }
        const auto& lastNote = proposedColumnNotes.back();
        const auto proposedTime =
          lastNote.time.timestamp + preferredNoteDistance;
        if (proposedTime < noteIt->time.timestamp) {
            pushProposedNote(proposedColumnNotes, noteIt);
            return true;
        }
    }
    return false;
}

inline void
pushNoteWithMaxDistance(
  const std::vector<
    charts::gameplay_models::BmsNotesData::Note>::const_iterator& noteIt,
  const QList<int>& columnsIota,
  std::vector<std::vector<charts::gameplay_models::BmsNotesData::Note>>&
    newColumns)
{
    auto bestPick = -1;
    auto bestDistance = 0ns;
    for (const auto& proposedColumn : columnsIota) {
        auto& proposedColumnNotes = newColumns[proposedColumn];
        const auto& lastNote = proposedColumnNotes.back();
        const auto proposedTime = lastNote.time.timestamp;
        if (noteIt->time.timestamp - proposedTime > bestDistance) {
            bestPick = proposedColumn;
            bestDistance = noteIt->time.timestamp - proposedTime;
        }
    }
    if (bestDistance == 0ns) {
        spdlog::critical(
          "Failed to find a column for a note in randomization "
          "algorithm. Please report this issue to the developers.");
    }
    pushProposedNote(newColumns[bestPick], noteIt);
}

template<typename Random>
void
shuffleAllNotes(
  std::span<std::vector<charts::gameplay_models::BmsNotesData::Note>> arr,
  const std::chrono::nanoseconds preferredNoteDistance,
  Random& randomGenerator)
{
    auto newColumns =
      std::vector<std::vector<charts::gameplay_models::BmsNotesData::Note>>{};
    newColumns.resize(arr.size());
    auto noteIters = std::vector<std::vector<
      charts::gameplay_models::BmsNotesData::Note>::const_iterator>{};
    noteIters.resize(arr.size());
    for (auto i = 0; i < arr.size(); ++i) {
        noteIters[i] = arr[i].cbegin();
    }

    while (auto noteIt = getNextNote(arr, noteIters)) {
        const auto& note = **noteIt;
        // first, we generate a list of proposed positions, ordered by
        // preference
        auto columnsIota = getColumsIota(arr.size());
        fisherYatesShuffle(columnsIota, randomGenerator);
        // check where there is enough space for the note
        const auto spotFound = tryPushingNoteWithDistance(
          *noteIt, columnsIota, newColumns, preferredNoteDistance);
        if (spotFound) {
            continue;
        }
        pushNoteWithMaxDistance(*noteIt, columnsIota, newColumns);
    }
    for (auto i = 0; i < arr.size(); ++i) {
        arr[i] = newColumns[i];
    }
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

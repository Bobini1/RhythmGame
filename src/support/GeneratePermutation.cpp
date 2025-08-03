//
// Created by PC on 01/10/2024.
//

#include "GeneratePermutation.h"

#include <unordered_set>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

namespace support {

template<typename T, typename Random>
void
fisherYatesShuffle(std::span<T> arr, Random& randomGenerator)
{
    for (auto i = arr.size() - 1; i > 0; --i) {
        // we shouldn't need to support more than 256 columns
        auto distribution =
          std::uniform_int_distribution(0, static_cast<int>(i - 1));
        const auto j = distribution(randomGenerator);
        std::swap(arr[i], arr[j]);
    }
}

auto
getColumsIota(const int size) -> QList<int>
{
    auto columns = QList<int>{};
    columns.reserve(size);
    for (auto i = 0; i < size; ++i) {
        columns.append(i);
    }
    return columns;
}

auto
getNextNote(
  const std::span<std::vector<charts::BmsNotesData::Note>> arr,
  std::vector<
    std::vector<charts::BmsNotesData::Note>::const_iterator>&
    noteIters)
  -> std::optional<
    std::vector<charts::BmsNotesData::Note>::const_iterator>
{
    auto nextNote = std::optional<std::vector<
      charts::BmsNotesData::Note>::const_iterator>{};
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
            charts::BmsNotesData::NoteType::LongNoteBegin) {
            ++noteIters[selectedColumn];
        }
    }
    return nextNote;
}

auto
pushProposedNote(
  std::vector<charts::BmsNotesData::Note>& lane,
  const std::vector<
    charts::BmsNotesData::Note>::const_iterator& note) -> void
{
    lane.push_back(*note);
    if (note->noteType ==
        charts::BmsNotesData::NoteType::LongNoteBegin) {
        auto endNote = note;
        ++endNote;
        lane.push_back(*endNote);
    }
}

auto
tryPushingNoteWithDistance(
  const std::vector<
    charts::BmsNotesData::Note>::const_iterator& noteIt,
  const QList<int>& columnsIota,
  std::vector<std::vector<charts::BmsNotesData::Note>>&
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

void
pushNoteWithMaxDistance(
  const std::vector<
    charts::BmsNotesData::Note>::const_iterator& noteIt,
  const QList<int>& columnsIota,
  std::vector<std::vector<charts::BmsNotesData::Note>>&
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
    if (bestPick != -1) {
        pushProposedNote(newColumns[bestPick], noteIt);
        return;
    }
    pushProposedNote(newColumns[columnsIota[0]], noteIt);
}

template<typename Random>
void
shuffleAllNotes(
  std::span<std::vector<charts::BmsNotesData::Note>> arr,
  const std::chrono::nanoseconds preferredNoteDistance,
  Random& randomGenerator)
{
    auto newColumns =
      std::vector<std::vector<charts::BmsNotesData::Note>>{};
    newColumns.resize(arr.size());
    auto noteIters = std::vector<std::vector<
      charts::BmsNotesData::Note>::const_iterator>{};
    noteIters.resize(arr.size());
    for (auto i = 0; i < arr.size(); ++i) {
        noteIters[i] = arr[i].cbegin();
    }

    while (auto noteIt = getNextNote(arr, noteIters)) {
        // first, we generate a list of proposed positions, ordered by
        // preference
        auto columnsIota = getColumsIota(arr.size());
        fisherYatesShuffle(std::span(columnsIota), randomGenerator);
        //  check where there is enough space for the note
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

auto
generatePermutation(
  std::span<std::vector<charts::BmsNotesData::Note>>&
    visibleNotes,
  const resource_managers::NoteOrderAlgorithm algorithm,
  const std::optional<uint64_t> seed) -> ShuffleResult
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
    switch (algorithm) {
        case resource_managers::NoteOrderAlgorithm::Normal: {
            const auto columns = getColumsIota(visibleNotes.size());
            return { 0, columns };
        }
        case resource_managers::NoteOrderAlgorithm::Mirror: {
            auto columns = getColumsIota(visibleNotes.size());
            std::reverse(columns.begin(), columns.end() - 1);
            std::reverse(visibleNotes.begin(), visibleNotes.end() - 1);
            return { 0, columns };
        }
        case resource_managers::NoteOrderAlgorithm::RRandom: {
            const auto randomSeed = seed.has_value() ? seed.value() : rd();
            auto randomGenerator = RandomGenerator{ randomSeed };
            const auto shift = std::uniform_int_distribution<>(
              0, visibleNotes.size() - 2)(randomGenerator);
            auto columns = getColumsIota(visibleNotes.size());
            // rotate all but last column
            std::rotate(
              columns.begin(), columns.begin() + shift, columns.end() - 1);
            std::rotate(visibleNotes.begin(),
                        visibleNotes.begin() + shift,
                        visibleNotes.end() - 1);
            // also mirror sometimes
            if (std::uniform_int_distribution(0, 1)(randomGenerator) != 0) {
                std::reverse(columns.begin(), columns.end() - 1);
                std::reverse(visibleNotes.begin(), visibleNotes.end() - 1);
            }
            return { randomSeed, columns };
        }
        case resource_managers::NoteOrderAlgorithm::Random: {
            const auto randomSeed = seed.has_value() ? seed.value() : rd();
            auto columns = getColumsIota(visibleNotes.size());
            auto randomGenerator = RandomGenerator{ randomSeed };
            fisherYatesShuffle(
              std::span(columns).subspan(0, columns.size() - 1),
              randomGenerator);
            randomGenerator.seed(randomSeed);
            fisherYatesShuffle(
              std::span(visibleNotes).subspan(0, visibleNotes.size() - 1),
              randomGenerator);
            return { randomSeed, columns };
        }
        case resource_managers::NoteOrderAlgorithm::RandomPlus: {
            const auto randomSeed = seed.has_value() ? seed.value() : rd();
            auto columns = getColumsIota(visibleNotes.size());
            auto randomGenerator = RandomGenerator{ randomSeed };
            fisherYatesShuffle(std::span(columns), randomGenerator);
            randomGenerator.seed(randomSeed);
            fisherYatesShuffle(visibleNotes, randomGenerator);
            return { randomSeed, columns };
        }
        case resource_managers::NoteOrderAlgorithm::SRandom: {
            const auto randomSeed = seed.has_value() ? seed.value() : rd();
            auto randomGenerator = RandomGenerator{ randomSeed };
            static constexpr auto preferredNoteDistance = 40ms;
            shuffleAllNotes(
              std::span(visibleNotes).subspan(0, visibleNotes.size() - 1),
              preferredNoteDistance,
              randomGenerator);
            return { randomSeed, {} };
        }
        case resource_managers::NoteOrderAlgorithm::SRandomPlus: {
            const auto randomSeed = seed.has_value() ? seed.value() : rd();
            auto randomGenerator = RandomGenerator{ randomSeed };
            static constexpr auto preferredNoteDistance = 40ms;
            shuffleAllNotes(
              visibleNotes, preferredNoteDistance, randomGenerator);
            return { randomSeed, {} };
        }
    }
    spdlog::error("Unknown note order algorithm");
    return {};
}
} // namespace support
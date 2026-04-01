//
// Created by PC on 01/10/2024.
//

#include "GeneratePermutation.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

namespace support {

template<typename T, typename Random>
void
fisherYatesShuffle(std::span<T> arr, Random& randomGenerator, bool usePre130)
{
    for (auto i = arr.size() - 1; i > 0; --i) {
        // The algorithm was broken before 1.3.0
        auto max = usePre130 ? static_cast<int>(i - 1) : static_cast<int>(i);
        // we shouldn't need to support more than 256 columns
        auto distribution = std::uniform_int_distribution(0, max);
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
  std::vector<std::vector<charts::BmsNotesData::Note>::const_iterator>&
    noteIters)
  -> std::optional<std::vector<charts::BmsNotesData::Note>::const_iterator>
{
    auto nextNote =
      std::optional<std::vector<charts::BmsNotesData::Note>::const_iterator>{};
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
  const std::vector<charts::BmsNotesData::Note>::const_iterator& note) -> void
{
    lane.push_back(*note);
    if (note->noteType == charts::BmsNotesData::NoteType::LongNoteBegin) {
        auto endNote = note;
        ++endNote;
        lane.push_back(*endNote);
    }
}

auto
tryPushingNoteWithDistance(
  const std::vector<charts::BmsNotesData::Note>::const_iterator& noteIt,
  const QList<int>& columnsIota,
  std::vector<std::vector<charts::BmsNotesData::Note>>& newColumns,
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
  const std::vector<charts::BmsNotesData::Note>::const_iterator& noteIt,
  const QList<int>& columnsIota,
  std::vector<std::vector<charts::BmsNotesData::Note>>& newColumns)
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
shuffleAllNotes(std::span<std::vector<charts::BmsNotesData::Note>> arr,
                const std::chrono::nanoseconds preferredNoteDistance,
                Random& randomGenerator,
                bool usePre130)
{
    auto newColumns = std::vector<std::vector<charts::BmsNotesData::Note>>{};
    newColumns.resize(arr.size());
    auto noteIters =
      std::vector<std::vector<charts::BmsNotesData::Note>::const_iterator>{};
    noteIters.resize(arr.size());
    for (auto i = 0; i < arr.size(); ++i) {
        noteIters[i] = arr[i].cbegin();
    }

    while (auto noteIt = getNextNote(arr, noteIters)) {
        // first, we generate a list of proposed positions, ordered by
        // preference
        auto columnsIota = getColumsIota(arr.size());
        fisherYatesShuffle(std::span(columnsIota), randomGenerator, usePre130);
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
convertColumnsToK7(QList<int>& columns) -> void
{
    if (columns.size() == 6) {
        // add 5 at index 5
        columns.insert(5, 5);
        columns.insert(6, 6);
    }
}

auto
convertNotesToK7(std::span<std::vector<charts::BmsNotesData::Note>>& notes)
  -> void
{
    using std::swap;
    swap(notes[5], notes[7]);
}

auto
generatePermutation(std::span<std::vector<charts::BmsNotesData::Note>>& notes,
                    const resource_managers::NoteOrderAlgorithm algorithm,
                    const uint64_t randomSeed,
                    bool k5,
                    bool usePre130) -> ShuffleResult
{
    using RandomGenerator = std::mt19937_64;
    auto originalSpan = notes;
    if (k5) {
        // temporarily move column 7 to column 5
        notes[5].swap(notes[7]);
        notes = notes.subspan(0, 6);
    }
    auto columns = getColumsIota(notes.size());
    switch (algorithm) {
        case resource_managers::NoteOrderAlgorithm::Normal: {
            convertColumnsToK7(columns);
            if (k5) {
                convertNotesToK7(originalSpan);
            }
            return { 0, columns };
        }
        case resource_managers::NoteOrderAlgorithm::Mirror: {
            std::reverse(columns.begin(), columns.end() - 1);
            std::reverse(notes.begin(), notes.end() - 1);
            convertColumnsToK7(columns);
            if (k5) {
                convertNotesToK7(originalSpan);
            }
            return { 0, columns };
        }
        case resource_managers::NoteOrderAlgorithm::RRandom: {
            auto randomGenerator = RandomGenerator{ randomSeed };
            const auto shift = std::uniform_int_distribution<>(
              0, notes.size() - 2)(randomGenerator);
            // rotate all but last column
            std::rotate(
              columns.begin(), columns.begin() + shift, columns.end() - 1);
            std::rotate(notes.begin(), notes.begin() + shift, notes.end() - 1);
            // also mirror sometimes
            if (std::uniform_int_distribution(0, 1)(randomGenerator) != 0) {
                std::reverse(columns.begin(), columns.end() - 1);
                std::reverse(notes.begin(), notes.end() - 1);
            }
            convertColumnsToK7(columns);
            if (k5) {
                convertNotesToK7(originalSpan);
            }
            return { randomSeed, columns };
        }
        case resource_managers::NoteOrderAlgorithm::Random: {
            auto randomGenerator = RandomGenerator{ randomSeed };
            fisherYatesShuffle(
              std::span(columns).subspan(0, columns.size() - 1),
              randomGenerator,
              usePre130);
            randomGenerator.seed(randomSeed);
            fisherYatesShuffle(std::span(notes).subspan(0, notes.size() - 1),
                               randomGenerator,
                               usePre130);
            convertColumnsToK7(columns);
            if (k5) {
                convertNotesToK7(originalSpan);
            }
            return { randomSeed, columns };
        }
        case resource_managers::NoteOrderAlgorithm::RandomPlus: {
            auto randomGenerator = RandomGenerator{ randomSeed };
            fisherYatesShuffle(std::span(columns), randomGenerator, usePre130);
            randomGenerator.seed(randomSeed);
            fisherYatesShuffle(notes, randomGenerator, usePre130);
            convertColumnsToK7(columns);
            if (k5) {
                convertNotesToK7(originalSpan);
            }
            return { randomSeed, columns };
        }
        case resource_managers::NoteOrderAlgorithm::SRandom: {
            auto randomGenerator = RandomGenerator{ randomSeed };
            static constexpr auto preferredNoteDistance = 40ms;
            shuffleAllNotes(std::span(notes).subspan(0, notes.size() - 1),
                            preferredNoteDistance,
                            randomGenerator,
                            usePre130);
            convertColumnsToK7(columns);
            if (k5) {
                convertNotesToK7(originalSpan);
            }
            return { randomSeed, columns };
        }
        case resource_managers::NoteOrderAlgorithm::SRandomPlus: {
            auto randomGenerator = RandomGenerator{ randomSeed };
            static constexpr auto preferredNoteDistance = 40ms;
            shuffleAllNotes(
              notes, preferredNoteDistance, randomGenerator, usePre130);
            convertColumnsToK7(columns);
            if (k5) {
                convertNotesToK7(originalSpan);
            }
            return { randomSeed, columns };
        }
    }
    spdlog::error("Unknown note order algorithm");
    return {};
}

// =============================================================================
// Beatoraja-compatible per-timeline permutation
// =============================================================================
//
// Java's java.util.Random (48-bit LCG):
//   seed_n = (seed_{n-1} * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1)
//   nextInt(n): calls next(31) → (int)(seed >>> 17), then rejection-samples.
//
// Each beatoraja randomizer operates on one TimeLine at a time, maintaining
// state across timelines (lastNoteTime, LN locks, spiral head, etc.).
// We replicate that state machine here.

// ---------------------------------------------------------------------------
// Java RNG
// ---------------------------------------------------------------------------
struct JavaRandom
{
    int64_t seed;

    explicit JavaRandom(int64_t s)
      : seed((s ^ 0x5DEECE66DLL) & 0x0000'FFFF'FFFF'FFFFLL)
    {
    }

    // Replicates java.util.Random.nextInt(int bound)
    auto nextInt(int bound) -> int
    {
        if (bound <= 0) {
            throw std::invalid_argument("bound must be positive");
        }
        if (bound == 1) {
            return 0;
        }
        if ((bound & (bound - 1)) == 0) {
            // power-of-two fast path
            seed = (seed * 0x5DEECE66DLL + 0xBLL) & 0x0000'FFFF'FFFF'FFFFLL;
            auto bits = static_cast<int32_t>(seed >> 17); // next(31)
            return static_cast<int>(
              static_cast<int64_t>(bound) * static_cast<int64_t>(bits) >> 31);
        }
        int32_t bits;
        int val;
        do {
            seed = (seed * 0x5DEECE66DLL + 0xBLL) & 0x0000'FFFF'FFFF'FFFFLL;
            bits = static_cast<int32_t>(seed >> 17); // next(31), always >= 0
            val = bits % bound;
        } while (bits - val + (bound - 1) < 0);
        return val;
    }
};

// ---------------------------------------------------------------------------
// Common timeline types
// ---------------------------------------------------------------------------
using Note = charts::BmsNotesData::Note;
using NoteType = charts::BmsNotesData::NoteType;

struct TimelineLane
{
    std::optional<std::size_t> mainNoteIndex;
    std::vector<std::size_t> noteIndices;
};

struct Timeline
{
    int64_t time;
    std::vector<TimelineLane> lanes;
};

struct TimelinePermutation
{
    std::vector<int> sourceForDest;
};

auto
eraseFirst(std::vector<int>& lanes, const int value) -> void
{
    if (const auto it = std::find(lanes.begin(), lanes.end(), value);
        it != lanes.end()) {
        lanes.erase(it);
    }
}

auto
isPlayableTimelineNote(const Note& note) -> bool
{
    return note.noteType != NoteType::Landmine &&
           note.noteType != NoteType::Invisible;
}

auto
makeSourceForDest(const int numCols,
                  const std::unordered_map<int, int>& permutationMap)
  -> TimelinePermutation
{
    auto permutation = TimelinePermutation{};
    permutation.sourceForDest.resize(numCols);
    std::iota(
      permutation.sourceForDest.begin(), permutation.sourceForDest.end(), 0);
    for (const auto& [src, dest] : permutationMap) {
        permutation.sourceForDest[dest] = src;
    }
    return permutation;
}

// ---------------------------------------------------------------------------
// Shared per-timeline state (lane lists, LN tracking, lastNoteTime)
// ---------------------------------------------------------------------------
struct BeatorajaState
{
    int numCols;
    std::vector<int> modifyLanes;
    std::vector<int> changeableLane;
    std::vector<int> assignableLane;
    std::vector<int64_t> lastNoteTime;
    std::unordered_map<int, int> lnActive; // src -> dst

    explicit BeatorajaState(const int n)
      : numCols(n)
      , lastNoteTime(static_cast<std::size_t>(n), -10'000'000'000LL)
    {
    }

    void setModifyLanes(std::vector<int> lanes)
    {
        modifyLanes = std::move(lanes);
        changeableLane = modifyLanes;
        assignableLane = modifyLanes;
        for (const auto lane : modifyLanes) {
            lastNoteTime[static_cast<std::size_t>(lane)] = -10'000'000'000LL;
        }
    }

    static auto getMainNote(const std::span<std::vector<Note>>& notes,
                            const Timeline& timeline,
                            const int lane) -> const Note*
    {
        const auto mainNoteIndex = timeline.lanes[lane].mainNoteIndex;
        if (!mainNoteIndex.has_value()) {
            return nullptr;
        }
        return &notes[lane][mainNoteIndex.value()];
    }

    static void classifyLanes(const std::span<std::vector<Note>>& notes,
                              const Timeline& timeline,
                              const std::vector<int>& changeableLane,
                              std::vector<int>& noteLane,
                              std::vector<int>& emptyLane)
    {
        noteLane.clear();
        emptyLane.clear();
        for (const auto lane : changeableLane) {
            const auto* note = getMainNote(notes, timeline, lane);
            if (note != nullptr && isPlayableTimelineNote(*note)) {
                noteLane.push_back(lane);
            } else {
                emptyLane.push_back(lane);
            }
        }
    }

    void classifyAssignable(const int64_t threshold,
                            const int64_t timelineTime,
                            const std::vector<int>& assignableLaneCopy,
                            std::vector<int>& primaryLane,
                            std::vector<int>& inferiorLane) const
    {
        primaryLane.clear();
        inferiorLane.clear();
        for (const auto lane : assignableLaneCopy) {
            if (timelineTime - lastNoteTime[static_cast<std::size_t>(lane)] >
                threshold) {
                primaryLane.push_back(lane);
            } else {
                inferiorLane.push_back(lane);
            }
        }
    }

    void timeBasedShuffle(
      JavaRandom& rng,
      const std::span<std::vector<Note>>& notes,
      const Timeline& timeline,
      const std::vector<int>& changeableLaneCopy,
      const std::vector<int>& assignableLaneCopy,
      const int64_t threshold,
      std::unordered_map<int, int>& randomMap,
      const std::function<int(std::vector<int>&)>& selectLane)
    {
        auto noteLane = std::vector<int>{};
        auto emptyLane = std::vector<int>{};
        auto primaryLane = std::vector<int>{};
        auto inferiorLane = std::vector<int>{};

        classifyLanes(notes, timeline, changeableLaneCopy, noteLane, emptyLane);
        classifyAssignable(threshold,
                           timeline.time,
                           assignableLaneCopy,
                           primaryLane,
                           inferiorLane);

        while (!(noteLane.empty() || primaryLane.empty())) {
            const auto selected = selectLane(primaryLane);
            randomMap[noteLane.front()] = primaryLane[selected];
            noteLane.erase(noteLane.begin());
            primaryLane.erase(primaryLane.begin() + selected);
        }

        while (!noteLane.empty()) {
            auto minimumNoteTime = std::numeric_limits<int64_t>::max();
            for (const auto lane : inferiorLane) {
                minimumNoteTime =
                  std::min(minimumNoteTime,
                           lastNoteTime[static_cast<std::size_t>(lane)]);
            }
            auto minimumLanes = std::vector<int>{};
            for (const auto lane : inferiorLane) {
                if (lastNoteTime[static_cast<std::size_t>(lane)] ==
                    minimumNoteTime) {
                    minimumLanes.push_back(lane);
                }
            }
            const auto chosen =
              minimumLanes[rng.nextInt(static_cast<int>(minimumLanes.size()))];
            randomMap[noteLane.front()] = chosen;
            noteLane.erase(noteLane.begin());
            eraseFirst(inferiorLane, chosen);
        }

        primaryLane.insert(
          primaryLane.end(), inferiorLane.begin(), inferiorLane.end());
        while (!emptyLane.empty()) {
            const auto selected =
              rng.nextInt(static_cast<int>(primaryLane.size()));
            randomMap[emptyLane.front()] = primaryLane[selected];
            emptyLane.erase(emptyLane.begin());
            primaryLane.erase(primaryLane.begin() + selected);
        }
    }

    void updateNoteTime(const std::span<std::vector<Note>>& notes,
                        const Timeline& timeline,
                        const std::unordered_map<int, int>& randomMap)
    {
        for (const auto& [src, dst] : randomMap) {
            const auto* note = getMainNote(notes, timeline, src);
            if (note != nullptr && note->noteType != NoteType::Landmine) {
                lastNoteTime[static_cast<std::size_t>(dst)] = timeline.time;
            }
        }
    }

    void updateLongNoteState(const std::span<std::vector<Note>>& notes,
                             const Timeline& timeline,
                             const std::unordered_map<int, int>& permutationMap)
    {
        auto mappings = std::vector<std::pair<int, int>>{};
        mappings.reserve(permutationMap.size());
        for (const auto& entry : permutationMap) {
            mappings.push_back(entry);
        }
        std::ranges::sort(mappings, [](const auto& left, const auto& right) {
            return left.first < right.first;
        });

        for (const auto& [src, dst] : mappings) {
            const auto* note = getMainNote(notes, timeline, src);
            if (note == nullptr) {
                continue;
            }
            if (note->noteType == NoteType::LongNoteEnd &&
                lnActive.contains(src) &&
                timeline.time == note->time.timestamp.count()) {
                lnActive.erase(src);
                changeableLane.push_back(src);
                assignableLane.push_back(dst);
            } else if (note->noteType == NoteType::LongNoteBegin) {
                lnActive[src] = dst;
                eraseFirst(changeableLane, src);
                eraseFirst(assignableLane, dst);
            }
        }
    }
};

void
applyTimelinePermutations(std::span<std::vector<Note>> notes,
                          const std::vector<Timeline>& timelines,
                          const std::vector<TimelinePermutation>& permutations)
{
    auto out = std::vector<std::vector<Note>>(notes.size());
    for (std::size_t timelineIndex = 0; timelineIndex < timelines.size();
         ++timelineIndex) {
        const auto& timeline = timelines[timelineIndex];
        const auto& permutation = permutations[timelineIndex];
        for (int dest = 0; dest < static_cast<int>(notes.size()); ++dest) {
            const auto src = permutation.sourceForDest[dest];
            for (const auto noteIndex : timeline.lanes[src].noteIndices) {
                out[dest].push_back(notes[src][noteIndex]);
            }
        }
    }
    for (int i = 0; i < static_cast<int>(notes.size()); ++i) {
        notes[i] = std::move(out[i]);
    }
}

ShuffleResult
makeIdentityResult(int64_t seed, int numCols)
{
    auto columns = QList<int>{};
    columns.reserve(numCols);
    for (int i = 0; i < numCols; ++i) {
        columns.append(i);
    }
    return { static_cast<uint64_t>(seed), columns };
}

auto
makeModifyLanes(int numCols, int scratchCol, bool includeScratch)
  -> std::vector<int>
{
    auto lanes = std::vector<int>{};
    lanes.reserve(numCols);
    for (int i = 0; i < numCols; ++i) {
        if (!includeScratch && i == scratchCol) {
            continue;
        }
        lanes.push_back(i);
    }
    return lanes;
}

auto
makeLaneIdentity(int numCols) -> QList<int>
{
    auto columns = QList<int>{};
    columns.reserve(numCols);
    for (int i = 0; i < numCols; ++i) {
        columns.append(i);
    }
    return columns;
}

auto
applyLanePermutation(std::span<std::vector<Note>> notes,
                     const QList<int>& columns) -> void
{
    auto out = std::vector<std::vector<Note>>(notes.size());
    for (int dest = 0; dest < static_cast<int>(notes.size()); ++dest) {
        out[dest] = notes[columns[dest]];
    }
    for (int i = 0; i < static_cast<int>(notes.size()); ++i) {
        notes[i] = std::move(out[i]);
    }
}

auto
generateBeatorajaLanePermutation(
  std::span<std::vector<Note>> notes,
  resource_managers::NoteOrderAlgorithm algorithm,
  int64_t seed) -> ShuffleResult
{
    auto columns = makeLaneIdentity(static_cast<int>(notes.size()));
    auto includesScratch =
      algorithm == resource_managers::NoteOrderAlgorithm::BeatorajaRandomEx;
    auto scratchCol = notes.size() - 1;
    const auto lanes = makeModifyLanes(
      static_cast<int>(notes.size()), scratchCol, includesScratch);

    switch (algorithm) {
        case resource_managers::NoteOrderAlgorithm::Mirror: {
            for (int i = 0; i < static_cast<int>(lanes.size()); ++i) {
                columns[lanes[i]] =
                  lanes[static_cast<int>(lanes.size()) - 1 - i];
            }
            break;
        }
        case resource_managers::NoteOrderAlgorithm::BeatorajaRandom:
        case resource_managers::NoteOrderAlgorithm::BeatorajaRandomEx: {
            auto rng = JavaRandom(seed);
            auto remaining = lanes;
            for (const auto lane : lanes) {
                const auto picked =
                  rng.nextInt(static_cast<int>(remaining.size()));
                columns[lane] = remaining[picked];
                remaining.erase(remaining.begin() + picked);
            }
            break;
        }
        default:
            return { static_cast<uint64_t>(seed), columns };
    }

    applyLanePermutation(notes, columns);
    return { static_cast<uint64_t>(seed), columns };
}

// ---------------------------------------------------------------------------
// Algorithm implementations
// ---------------------------------------------------------------------------

auto
mergePermutationMap(const std::unordered_map<int, int>& randomMap,
                    const std::unordered_map<int, int>& lnActive)
  -> std::unordered_map<int, int>
{
    auto permutationMap = randomMap;
    for (const auto& [src, dst] : lnActive) {
        permutationMap[src] = dst;
    }
    return permutationMap;
}

auto
isBeatorajaNoteOrderAlgorithm(resource_managers::NoteOrderAlgorithm algorithm)
  -> bool
{
    switch (algorithm) {
        case resource_managers::NoteOrderAlgorithm::BeatorajaRandom:
        case resource_managers::NoteOrderAlgorithm::BeatorajaRandomEx:
            return true;
        default:
            return false;
    }
}

} // namespace support

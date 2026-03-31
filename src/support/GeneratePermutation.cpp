//
// Created by PC on 01/10/2024.
//

#include "GeneratePermutation.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <numeric>
#include <unordered_set>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

namespace support {

template<typename T, typename Random>
void
fisherYatesShuffle(std::span<T> arr, Random& randomGenerator, bool usePre130)
{
    for (auto i = arr.size() - 1; i > 0; --i) {
        // The algorithm was broken before 1.3.0
        auto max = usePre130 ? static_cast<int>(i + 1) : static_cast<int>(i);
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
        if (bound <= 1) {
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
            val  = bits % bound;
        } while (bits - val + (bound - 1) < 0);
        return val;
    }
};

// ---------------------------------------------------------------------------
// Common timeline types
// ---------------------------------------------------------------------------
using Note     = charts::BmsNotesData::Note;
using NoteType = charts::BmsNotesData::NoteType;

struct NoteEvent
{
    std::chrono::nanoseconds time;
    int          column;
    std::size_t  noteIndex;
};

struct LnEntry
{
    int         srcCol;
    std::size_t beginIdx; // index of the LongNoteBegin note in original column
};

// Attack-note timelines (LongNoteEnd excluded — tied to its begin).
auto
buildTimelines(const std::span<std::vector<Note>>& notes)
  -> std::vector<std::vector<NoteEvent>>
{
    std::vector<NoteEvent> all;
    for (int col = 0; col < static_cast<int>(notes.size()); ++col) {
        for (std::size_t i = 0; i < notes[col].size(); ++i) {
            if (notes[col][i].noteType != NoteType::LongNoteEnd) {
                all.push_back({ notes[col][i].time.timestamp, col, i });
            }
        }
    }
    std::stable_sort(all.begin(), all.end(), [](const NoteEvent& a,
                                                const NoteEvent& b) {
        return a.time < b.time;
    });
    std::vector<std::vector<NoteEvent>> tls;
    for (auto& ev : all) {
        if (tls.empty() || tls.back().front().time != ev.time) {
            tls.push_back({ ev });
        } else {
            tls.back().push_back(ev);
        }
    }
    return tls;
}

// ---------------------------------------------------------------------------
// Shared per-timeline state (lane lists, LN tracking, lastNoteTime)
// ---------------------------------------------------------------------------
struct BeatorajaState
{
    int                              numCols;
    std::vector<int>                 modifyLanes;  // all column indices
    std::vector<int64_t>             lastNoteTime; // ns, init -10 s
    std::unordered_map<int, LnEntry> lnActive;     // dst -> {src, beginIdx}

    explicit BeatorajaState(int n)
      : numCols(n)
      , modifyLanes(n)
      , lastNoteTime(n, -10'000'000'000LL)
    {
        std::iota(modifyLanes.begin(), modifyLanes.end(), 0);
    }

    // Build the changeable/assignable lane lists for this timeline,
    // excluding lanes locked by active LNs.
    void buildLaneLists(std::vector<int>& changeableLane,
                        std::vector<int>& assignableLane) const
    {
        changeableLane.assign(modifyLanes.begin(), modifyLanes.end());
        assignableLane.assign(modifyLanes.begin(), modifyLanes.end());
        for (auto& [dst, ln] : lnActive) {
            changeableLane.erase(
              std::remove(changeableLane.begin(), changeableLane.end(), ln.srcCol),
              changeableLane.end());
            assignableLane.erase(
              std::remove(assignableLane.begin(), assignableLane.end(), dst),
              assignableLane.end());
        }
    }

    // Determine which changeableLane entries have a real note in this timeline.
    static void classifyLanes(const std::span<std::vector<Note>>& notes,
                               const std::vector<NoteEvent>&       tl,
                               const std::unordered_set<int>&      tlCols,
                               const std::vector<int>&             changeableLane,
                               std::vector<int>&                   noteLane,
                               std::vector<int>&                   emptyLane)
    {
        noteLane.clear();
        emptyLane.clear();
        for (int cl : changeableLane) {
            bool hasNote = false;
            if (tlCols.count(cl)) {
                auto it = std::find_if(tl.begin(), tl.end(),
                                       [cl](const NoteEvent& e) {
                                           return e.column == cl;
                                       });
                hasNote = (it != tl.end()) &&
                          notes[cl][it->noteIndex].noteType != NoteType::Landmine;
            }
            if (hasNote) {
                noteLane.push_back(cl);
            } else {
                emptyLane.push_back(cl);
            }
        }
    }

    // Classify assignable lanes into primary (quiet) and inferior (recent).
    void classifyAssignable(const std::vector<int>& assignableLane,
                            int64_t                  tlTime,
                            int64_t                  threshold,
                            std::vector<int>&        primaryLane,
                            std::vector<int>&        inferiorLane) const
    {
        primaryLane.clear();
        inferiorLane.clear();
        for (int al : assignableLane) {
            if (tlTime - lastNoteTime[al] > threshold) {
                primaryLane.push_back(al);
            } else {
                inferiorLane.push_back(al);
            }
        }
    }

    // Core time-based shuffle (SRandomizer.timeBasedShuffle).
    // randomMap is populated: src -> dst for all non-LN-locked lanes.
    void timeBasedShuffle(JavaRandom&                          rng,
                          const std::span<std::vector<Note>>&  notes,
                          const std::vector<NoteEvent>&        tl,
                          const std::unordered_set<int>&       tlCols,
                          std::vector<int>                     changeableLane,
                          std::vector<int>                     assignableLane,
                          int64_t                              tlTime,
                          int64_t                              threshold,
                          std::unordered_map<int, int>&        randomMap,
                          std::function<int(std::vector<int>&)> selectPrimary)
    {
        std::vector<int> noteLane, emptyLane, primaryLane, inferiorLane;
        classifyLanes(notes, tl, tlCols, changeableLane, noteLane, emptyLane);
        classifyAssignable(assignableLane, tlTime, threshold,
                           primaryLane, inferiorLane);

        // 1. Note lanes → primary lanes (selectPrimary picks which primary slot)
        while (!noteLane.empty() && !primaryLane.empty()) {
            int r = selectPrimary(primaryLane);
            randomMap[noteLane.front()] = primaryLane[r];
            noteLane.erase(noteLane.begin());
            primaryLane.erase(primaryLane.begin() + r);
        }
        // 2. Remaining note lanes → least-recently-used inferior lane
        while (!noteLane.empty()) {
            int64_t minT = std::numeric_limits<int64_t>::max();
            for (int l : inferiorLane) {
                minT = std::min(minT, lastNoteTime[l]);
            }
            std::vector<int> minCols;
            for (int l : inferiorLane) {
                if (lastNoteTime[l] == minT) {
                    minCols.push_back(l);
                }
            }
            int m = minCols[rng.nextInt(static_cast<int>(minCols.size()))];
            randomMap[noteLane.front()] = m;
            noteLane.erase(noteLane.begin());
            inferiorLane.erase(
              std::remove(inferiorLane.begin(), inferiorLane.end(), m),
              inferiorLane.end());
        }
        // 3. Empty lanes → any remaining lane
        primaryLane.insert(primaryLane.end(), inferiorLane.begin(),
                           inferiorLane.end());
        while (!emptyLane.empty()) {
            int r = rng.nextInt(static_cast<int>(primaryLane.size()));
            randomMap[emptyLane.front()] = primaryLane[r];
            emptyLane.erase(emptyLane.begin());
            primaryLane.erase(primaryLane.begin() + r);
        }
    }

    // Merge LN-active forced assignments into randomMap, then update
    // lastNoteTime + lnActive based on the chosen mapping + note types.
    void applyAndUpdateLnState(const std::span<std::vector<Note>>& notes,
                                const std::vector<NoteEvent>&       tl,
                                const std::unordered_set<int>&      tlCols,
                                int64_t                             tlTime,
                                std::unordered_map<int, int>&       randomMap)
    {
        // Merge locked LN lanes
        for (auto& [dst, ln] : lnActive) {
            randomMap[ln.srcCol] = dst;
        }

        // Update lastNoteTime and begin tracking new LNs
        for (auto& [src, dst] : randomMap) {
            if (!tlCols.count(src)) {
                continue;
            }
            auto it = std::find_if(tl.begin(), tl.end(),
                                   [src](const NoteEvent& e) {
                                       return e.column == src;
                                   });
            if (it == tl.end()) {
                continue;
            }
            const auto& note = notes[src][it->noteIndex];
            if (note.noteType == NoteType::Landmine) {
                continue;
            }
            lastNoteTime[dst] = tlTime;
            if (note.noteType == NoteType::LongNoteBegin) {
                lnActive[dst] = LnEntry{ src, it->noteIndex };
            }
        }

        // Unlock LNs whose end note coincides with this timeline's time
        std::vector<int> toRemove;
        for (auto& [dst, ln] : lnActive) {
            std::size_t endIdx = ln.beginIdx + 1;
            if (endIdx < notes[ln.srcCol].size() &&
                notes[ln.srcCol][endIdx].noteType == NoteType::LongNoteEnd &&
                notes[ln.srcCol][endIdx].time.timestamp ==
                  tl.front().time) {
                toRemove.push_back(dst);
            }
        }
        for (int d : toRemove) {
            lnActive.erase(d);
        }
    }
};

// ---------------------------------------------------------------------------
// Write output: apply the collected note mappings to rebuild the columns.
// ---------------------------------------------------------------------------
struct NoteMapping
{
    int         srcCol;
    std::size_t attackIdx;
    int         destCol;
};

void
applyMappings(std::span<std::vector<Note>>         notes,
              const std::vector<NoteMapping>&      mappings,
              int                                  numCols)
{
    std::vector<std::vector<Note>> out(numCols);
    for (auto& m : mappings) {
        auto& srcVec = notes[m.srcCol];
        auto& n      = srcVec[m.attackIdx];
        out[m.destCol].push_back(n);
        if (n.noteType == NoteType::LongNoteBegin &&
            m.attackIdx + 1 < srcVec.size() &&
            srcVec[m.attackIdx + 1].noteType == NoteType::LongNoteEnd) {
            out[m.destCol].push_back(srcVec[m.attackIdx + 1]);
        }
    }
    for (int i = 0; i < numCols; ++i) {
        std::stable_sort(out[i].begin(), out[i].end(),
                         [](const Note& a, const Note& b) {
                             return a.time.timestamp < b.time.timestamp;
                         });
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
applyLanePermutation(std::span<std::vector<Note>> notes, const QList<int>& columns)
  -> void
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
generateBeatorajaLanePermutation(std::span<std::vector<Note>> notes,
                                 resource_managers::NoteOrderAlgorithm algorithm,
                                 int64_t seed,
                                 int scratchCol,
                                 bool includeScratch) -> ShuffleResult
{
    auto columns = makeLaneIdentity(static_cast<int>(notes.size()));
    const auto lanes =
      makeModifyLanes(static_cast<int>(notes.size()), scratchCol, includeScratch);
    if (lanes.size() < 2) {
        return { static_cast<uint64_t>(seed), columns };
    }

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
        case resource_managers::NoteOrderAlgorithm::BeatorajaRotate: {
            auto rng = JavaRandom(seed);
            const auto clockwise = rng.nextInt(2) == 1;
            const auto start =
              rng.nextInt(static_cast<int>(lanes.size()) - 1) +
              (clockwise ? 1 : 0);
            auto source = start;
            for (const auto lane : lanes) {
                columns[lane] = lanes[source];
                source = clockwise
                           ? (source + 1) % static_cast<int>(lanes.size())
                           : (source + static_cast<int>(lanes.size()) - 1) %
                               static_cast<int>(lanes.size());
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

// SRandom / SRandomNoThreshold / SRandomEx / HRandom
// (all the same shape; differ only in threshold)
void
runSRandom(std::span<std::vector<Note>>         notes,
           const std::vector<std::vector<NoteEvent>>& timelines,
           JavaRandom&                          rng,
           int64_t                              threshold,
           int                                  numCols,
           const std::vector<int>&              modifyLanes,
           std::vector<NoteMapping>&            mappings)
{
    BeatorajaState state(numCols);
    state.modifyLanes = modifyLanes;
    for (auto& tl : timelines) {
        std::unordered_set<int> tlCols;
        for (auto& ev : tl) {
            tlCols.insert(ev.column);
        }
        const int64_t tlTime = tl.front().time.count();

        std::vector<int> changeableLane, assignableLane;
        state.buildLaneLists(changeableLane, assignableLane);

        std::unordered_map<int, int> randomMap;
        state.timeBasedShuffle(
          rng, notes, tl, tlCols, changeableLane, assignableLane,
          tlTime, threshold, randomMap,
          // SRandomizer.selectLane: pure random pick
          [&](std::vector<int>& lane) {
              return rng.nextInt(static_cast<int>(lane.size()));
          });

        state.applyAndUpdateLnState(notes, tl, tlCols, tlTime, randomMap);

        for (auto& ev : tl) {
            if (auto it = randomMap.find(ev.column); it != randomMap.end()) {
                mappings.push_back({ ev.column, ev.noteIndex, it->second });
            }
        }
    }
}

// Spiral: rotate all lanes by an ever-increasing increment each chord.
// When LNs are active the head does NOT advance (beatoraja source comment).
void
runSpiral(std::span<std::vector<Note>>              notes,
          const std::vector<std::vector<NoteEvent>>& timelines,
          JavaRandom&                               rng,
          int                                       numCols,
          const std::vector<int>&                   modifyLanes,
          std::vector<NoteMapping>&                 mappings)
{
    BeatorajaState state(numCols);
    state.modifyLanes = modifyLanes;

    // SpiralRandomizer constructor draws increment and initial head
    const auto cycle = static_cast<int>(modifyLanes.size());
    const int increment = rng.nextInt(cycle - 1) + 1;
    int       head      = 0;

    for (auto& tl : timelines) {
        std::unordered_set<int> tlCols;
        for (auto& ev : tl) {
            tlCols.insert(ev.column);
        }
        const int64_t tlTime = tl.front().time.count();

        std::vector<int> changeableLane, assignableLane;
        state.buildLaneLists(changeableLane, assignableLane);

        std::unordered_map<int, int> rotateMap;

        if (static_cast<int>(changeableLane.size()) == cycle) {
            // All lanes free → advance head and rotate everything
            head = (head + increment) % cycle;
            for (int i = 0; i < cycle; ++i) {
                rotateMap[state.modifyLanes[i]] =
                  state.modifyLanes[(i + head) % cycle];
            }
        } else {
            // Some lanes LN-locked → keep only free lanes in rotation
            for (int i = 0; i < cycle; ++i) {
                if (changeableLane.end() !=
                    std::find(changeableLane.begin(), changeableLane.end(),
                              state.modifyLanes[i])) {
                    rotateMap[state.modifyLanes[i]] =
                      state.modifyLanes[(i + head) % cycle];
                }
            }
        }

        state.applyAndUpdateLnState(notes, tl, tlCols, tlTime, rotateMap);

        for (auto& ev : tl) {
            if (auto it = rotateMap.find(ev.column); it != rotateMap.end()) {
                mappings.push_back({ ev.column, ev.noteIndex, it->second });
            }
        }
    }
}

// AllScr: route one note per chord to the next scratch lane (round-robin),
// provided the scratch lane is quiet enough, then timeBasedShuffle the rest.
void
runAllScr(std::span<std::vector<Note>>              notes,
          const std::vector<std::vector<NoteEvent>>& timelines,
          JavaRandom&                               rng,
          int                                       numCols,
          const std::vector<int>&                   modifyLanes,
          int                                       scratchCol,
          int64_t                                   scratchThreshold, // 40 ms
          int64_t                                   keyThreshold,     // caller
          std::vector<NoteMapping>&                 mappings)
{
    BeatorajaState state(numCols);
    state.modifyLanes = modifyLanes;

    // In beatoraja AllScratchRandomizer the scratchLane is just {scratchCol}
    // for SP.  For DP it would be both scratch columns, but we receive a single
    // half of the note span so scratchCol is always one index.
    const std::vector<int> scratchLanes = { scratchCol };
    int scratchIndex = 0;

    for (auto& tl : timelines) {
        std::unordered_set<int> tlCols;
        for (auto& ev : tl) {
            tlCols.insert(ev.column);
        }
        const int64_t tlTime = tl.front().time.count();

        std::vector<int> changeableLane, assignableLane;
        state.buildLaneLists(changeableLane, assignableLane);

        std::unordered_map<int, int> randomMap;

        // Try to assign one note to the scratch lane first
        int curScr = scratchLanes[scratchIndex];
        if (std::find(assignableLane.begin(), assignableLane.end(), curScr) !=
              assignableLane.end() &&
            tlTime - state.lastNoteTime[curScr] > scratchThreshold) {
            // Pick the first note-bearing changeable lane
            int chosen = -1;
            for (int cl : changeableLane) {
                if (tlCols.count(cl) &&
                    notes[cl][std::find_if(tl.begin(), tl.end(),
                                           [cl](const NoteEvent& e) {
                                               return e.column == cl;
                                           })->noteIndex].noteType !=
                      NoteType::Landmine) {
                    chosen = cl;
                    break;
                }
            }
            if (chosen != -1) {
                randomMap[chosen] = curScr;
                changeableLane.erase(
                  std::remove(changeableLane.begin(), changeableLane.end(),
                               chosen),
                  changeableLane.end());
                assignableLane.erase(
                  std::remove(assignableLane.begin(), assignableLane.end(),
                               curScr),
                  assignableLane.end());
                scratchIndex =
                  (scratchIndex + 1) % static_cast<int>(scratchLanes.size());
            }
        }

        // Shuffle the rest with timeBasedShuffle
        state.timeBasedShuffle(
          rng, notes, tl, tlCols, changeableLane, assignableLane, tlTime,
          keyThreshold, randomMap,
          // AllScratchRandomizer.selectLane for SP: pure random (same as SRandom)
          [&](std::vector<int>& lane) {
              return rng.nextInt(static_cast<int>(lane.size()));
          });

        state.applyAndUpdateLnState(notes, tl, tlCols, tlTime, randomMap);

        for (auto& ev : tl) {
            if (auto it = randomMap.find(ev.column); it != randomMap.end()) {
                mappings.push_back({ ev.column, ev.noteIndex, it->second });
            }
        }
    }
}

// Converge: prefer lanes with the longest current consecutive-note streak,
// aiming to maximise trills within [threshold1, threshold2] ms.
void
runConverge(std::span<std::vector<Note>>              notes,
            const std::vector<std::vector<NoteEvent>>& timelines,
            JavaRandom&                               rng,
            int                                       numCols,
            const std::vector<int>&                   modifyLanes,
            int64_t                                   threshold1, // lower bound
            int64_t                                   threshold2, // upper bound
            std::vector<NoteMapping>&                 mappings)
{
    BeatorajaState         state(numCols);
    state.modifyLanes = modifyLanes;
    std::vector<int>       rendaCount(numCols, 0);

    for (auto& tl : timelines) {
        std::unordered_set<int> tlCols;
        for (auto& ev : tl) {
            tlCols.insert(ev.column);
        }
        const int64_t tlTime = tl.front().time.count();

        // Reset streak counter if the lane went quiet beyond threshold2
        for (int i = 0; i < numCols; ++i) {
            if (tlTime - state.lastNoteTime[i] > threshold2) {
                rendaCount[i] = 0;
            }
        }

        std::vector<int> changeableLane, assignableLane;
        state.buildLaneLists(changeableLane, assignableLane);

        std::unordered_map<int, int> randomMap;
        state.timeBasedShuffle(
          rng, notes, tl, tlCols, changeableLane, assignableLane,
          tlTime, threshold1, randomMap,
          // ConvergeRandomizer.selectLane: pick the lane with max renda count
          [&](std::vector<int>& lane) -> int {
              // Find the maximum streak count among candidates
              int maxCount = 0;
              for (int l : lane) {
                  if (rendaCount[l] > maxCount) {
                      maxCount = rendaCount[l];
                  }
              }
              // Collect all lanes tied at max, pick one at random
              std::vector<int> best;
              for (int l : lane) {
                  if (rendaCount[l] == maxCount) {
                      best.push_back(l);
                  }
              }
              int chosen = best[rng.nextInt(static_cast<int>(best.size()))];
              rendaCount[chosen]++;
              auto pos = std::find(lane.begin(), lane.end(), chosen);
              return static_cast<int>(std::distance(lane.begin(), pos));
          });

        state.applyAndUpdateLnState(notes, tl, tlCols, tlTime, randomMap);

        for (auto& ev : tl) {
            if (auto it = randomMap.find(ev.column); it != randomMap.end()) {
                mappings.push_back({ ev.column, ev.noteIndex, it->second });
            }
        }
    }
}

// SRandomPlayable (NoMurioshiRandomizer): PMS-specific.
// The 9-button PMS avoids simultaneous impossible chord combinations by
// constraining notes to one of the 10 "safe" 6-button combinations.
// For non-PMS charts (< 9 lanes) it degrades gracefully to plain SRandom.
void
runSRandomPlayable(std::span<std::vector<Note>>              notes,
                   const std::vector<std::vector<NoteEvent>>& timelines,
                   JavaRandom&                               rng,
                   int                                       numCols,
                   const std::vector<int>&                   modifyLanes,
                   int64_t                                   threshold,
                   std::vector<NoteMapping>&                 mappings)
{
    // The 10 safe 6-button combinations for 9-button PMS
    static const std::vector<std::vector<int>> buttonCombinationTable = {
        {0,1,2,3,4,5}, {0,1,2,4,5,6}, {0,1,2,5,6,7}, {0,1,2,6,7,8},
        {1,2,3,4,5,6}, {1,2,3,5,6,7}, {1,2,3,6,7,8},
        {2,3,4,5,6,7}, {2,3,4,6,7,8}, {3,4,5,6,7,8}
    };

    BeatorajaState state(numCols);
    state.modifyLanes = modifyLanes;
    const bool usePmsLogic = (numCols == 9);

    for (auto& tl : timelines) {
        std::unordered_set<int> tlCols;
        for (auto& ev : tl) {
            tlCols.insert(ev.column);
        }
        const int64_t tlTime = tl.front().time.count();

        std::vector<int> changeableLane, assignableLane;
        state.buildLaneLists(changeableLane, assignableLane);

        std::unordered_map<int, int> randomMap;
        bool usedPmsPath = false;

        if (usePmsLogic) {
            // Count real notes in this timeline
            int noteCount = 0;
            for (int cl : changeableLane) {
                if (tlCols.count(cl)) {
                    auto it = std::find_if(tl.begin(), tl.end(),
                                           [cl](const NoteEvent& e) {
                                               return e.column == cl;
                                           });
                    if (it != tl.end() &&
                        notes[cl][it->noteIndex].noteType != NoteType::Landmine) {
                        noteCount++;
                    }
                }
            }
            // Count active LNs as notes too (beatoraja: getLNLane().size())
            noteCount += static_cast<int>(state.lnActive.size());

            if (noteCount > 2 && noteCount < 7) {
                // Collect active LN destination columns
                std::vector<int> lnDstCols;
                for (auto& [dst, ln] : state.lnActive) {
                    lnDstCols.push_back(dst);
                }

                // Filter button combinations to those that contain all LN lanes
                std::vector<std::vector<int>> candidate;
                for (auto& combo : buttonCombinationTable) {
                    bool ok = true;
                    for (int d : lnDstCols) {
                        if (std::find(combo.begin(), combo.end(), d) ==
                            combo.end()) {
                            ok = false;
                            break;
                        }
                    }
                    if (ok) {
                        candidate.push_back(combo);
                    }
                }

                if (!candidate.empty()) {
                    // Exclude recently hit lanes from candidates
                    std::vector<int> rendaLane;
                    for (int i = 0; i < numCols; ++i) {
                        if (tlTime - state.lastNoteTime[i] < threshold) {
                            rendaLane.push_back(i);
                        }
                    }
                    // Filter combinations: keep those with enough non-renda slots
                    std::vector<std::vector<int>> candidate2;
                    for (auto& combo : candidate) {
                        std::vector<int> avail;
                        for (int l : combo) {
                            if (std::find(rendaLane.begin(), rendaLane.end(), l) ==
                                rendaLane.end()) {
                                avail.push_back(l);
                            }
                        }
                        if (static_cast<int>(avail.size()) >= noteCount) {
                            candidate2.push_back(avail);
                        }
                    }

                    std::vector<int> buttonCombination;
                    if (!candidate2.empty()) {
                        buttonCombination =
                          candidate2[rng.nextInt(
                            static_cast<int>(candidate2.size()))];
                        usedPmsPath = true;

                        // Assign note lanes using the safe combination
                        state.timeBasedShuffle(
                          rng, notes, tl, tlCols, changeableLane, assignableLane,
                          tlTime, threshold, randomMap,
                          // selectLane: prefer slots inside buttonCombination
                          [&](std::vector<int>& lane) -> int {
                              std::vector<int> preferred;
                              for (int l : lane) {
                                  if (std::find(buttonCombination.begin(),
                                                buttonCombination.end(), l) !=
                                      buttonCombination.end()) {
                                      preferred.push_back(l);
                                  }
                              }
                              if (!preferred.empty()) {
                                  int chosen = preferred[rng.nextInt(
                                    static_cast<int>(preferred.size()))];
                                  auto pos =
                                    std::find(lane.begin(), lane.end(), chosen);
                                  return static_cast<int>(
                                    std::distance(lane.begin(), pos));
                              }
                              return rng.nextInt(static_cast<int>(lane.size()));
                          });
                    } else {
                        // Prioritise safe combo over trill avoidance
                        // (beatoraja: pick a valid combo ignoring renda)
                        auto& src = candidate[rng.nextInt(
                          static_cast<int>(candidate.size()))];
                        buttonCombination.clear();
                        for (int l : src) {
                            if (std::find(assignableLane.begin(),
                                          assignableLane.end(), l) !=
                                assignableLane.end()) {
                                buttonCombination.push_back(l);
                            }
                        }

                        // Assign note-bearing changeable lanes first
                        std::vector<int> noteLane, emptyLane;
                        BeatorajaState::classifyLanes(notes, tl, tlCols,
                                                      changeableLane, noteLane,
                                                      emptyLane);
                        for (int lane : noteLane) {
                            if (!buttonCombination.empty()) {
                                int r = rng.nextInt(
                                  static_cast<int>(buttonCombination.size()));
                                randomMap[lane] = buttonCombination[r];
                                buttonCombination.erase(
                                  buttonCombination.begin() + r);
                                changeableLane.erase(
                                  std::remove(changeableLane.begin(),
                                               changeableLane.end(), lane),
                                  changeableLane.end());
                                assignableLane.erase(
                                  std::remove(assignableLane.begin(),
                                               assignableLane.end(),
                                               randomMap[lane]),
                                  assignableLane.end());
                            }
                        }
                        // Shuffle the remainder normally
                        state.timeBasedShuffle(
                          rng, notes, tl, tlCols, changeableLane, assignableLane,
                          tlTime, threshold, randomMap,
                          [&](std::vector<int>& lane) {
                              return rng.nextInt(
                                static_cast<int>(lane.size()));
                          });
                        usedPmsPath = true;
                    }
                }
                // else: no safe combination exists → fall through to SRandom
            }
        }

        if (!usedPmsPath) {
            // Fallback: plain S-RANDOM (H-RANDOM style, same threshold)
            state.timeBasedShuffle(
              rng, notes, tl, tlCols, changeableLane, assignableLane,
              tlTime, threshold, randomMap,
              [&](std::vector<int>& lane) {
                  return rng.nextInt(static_cast<int>(lane.size()));
              });
        }

        state.applyAndUpdateLnState(notes, tl, tlCols, tlTime, randomMap);

        for (auto& ev : tl) {
            if (auto it = randomMap.find(ev.column); it != randomMap.end()) {
                mappings.push_back({ ev.column, ev.noteIndex, it->second });
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------
auto
generateBeatorajaPermutation(
  std::span<std::vector<Note>>& notes,
  BeatorajaRandom               algorithm,
  int64_t                       seed,
  int                           scratchCol,
  bool                          includeScratch,
  int64_t                       threshold_ns) -> ShuffleResult
{
    if (notes.empty()) {
        return {};
    }

    static constexpr int64_t SRAN_THRESHOLD = 40'000'000LL; // 40 ms
    const int numCols = static_cast<int>(notes.size());

    auto rng = JavaRandom(seed);
    const auto modifyLanes =
      makeModifyLanes(numCols, scratchCol, includeScratch);
    if (modifyLanes.size() < 2) {
        return makeIdentityResult(seed, numCols);
    }
    auto timelines = buildTimelines(notes);
    std::vector<NoteMapping> mappings;
    mappings.reserve(timelines.size() * 4);

    switch (algorithm) {
        case BeatorajaRandom::SRandom:
            runSRandom(notes,
                       timelines,
                       rng,
                       SRAN_THRESHOLD,
                       numCols,
                       modifyLanes,
                       mappings);
            break;

        case BeatorajaRandom::SRandomNoThreshold:
            runSRandom(notes, timelines, rng, 0, numCols, modifyLanes, mappings);
            break;

        case BeatorajaRandom::SRandomEx:
            runSRandom(notes,
                       timelines,
                       rng,
                       SRAN_THRESHOLD,
                       numCols,
                       modifyLanes,
                       mappings);
            break;

        case BeatorajaRandom::HRandom:
            runSRandom(notes,
                       timelines,
                       rng,
                       threshold_ns,
                       numCols,
                       modifyLanes,
                       mappings);
            break;

        case BeatorajaRandom::Spiral:
            runSpiral(notes, timelines, rng, numCols, modifyLanes, mappings);
            break;

        case BeatorajaRandom::AllScr:
            runAllScr(notes,
                      timelines,
                      rng,
                      numCols,
                      modifyLanes,
                      scratchCol,
                      SRAN_THRESHOLD,
                      threshold_ns,
                      mappings);
            break;

        case BeatorajaRandom::SRandomPlayable:
            runSRandomPlayable(notes,
                               timelines,
                               rng,
                               numCols,
                               modifyLanes,
                               threshold_ns,
                               mappings);
            break;

        case BeatorajaRandom::Converge:
            runConverge(notes,
                        timelines,
                        rng,
                        numCols,
                        modifyLanes,
                        threshold_ns,
                        threshold_ns * 2,
                        mappings);
            break;
    }

    applyMappings(notes, mappings, numCols);
    return makeIdentityResult(seed, numCols);
}

auto
isBeatorajaNoteOrderAlgorithm(resource_managers::NoteOrderAlgorithm algorithm)
  -> bool
{
    switch (algorithm) {
        case resource_managers::NoteOrderAlgorithm::BeatorajaRandom:
        case resource_managers::NoteOrderAlgorithm::BeatorajaRotate:
        case resource_managers::NoteOrderAlgorithm::BeatorajaSRandom:
        case resource_managers::NoteOrderAlgorithm::BeatorajaSpiral:
        case resource_managers::NoteOrderAlgorithm::BeatorajaHRandom:
        case resource_managers::NoteOrderAlgorithm::BeatorajaAllScr:
        case resource_managers::NoteOrderAlgorithm::BeatorajaRandomEx:
        case resource_managers::NoteOrderAlgorithm::BeatorajaSRandomEx:
            return true;
        default:
            return false;
    }
}

auto
beatorajaRandomFromNoteOrderAlgorithm(
  resource_managers::NoteOrderAlgorithm algorithm) -> BeatorajaRandom
{
    switch (algorithm) {
        case resource_managers::NoteOrderAlgorithm::BeatorajaSRandom:
            return BeatorajaRandom::SRandom;
        case resource_managers::NoteOrderAlgorithm::BeatorajaSpiral:
            return BeatorajaRandom::Spiral;
        case resource_managers::NoteOrderAlgorithm::BeatorajaHRandom:
            return BeatorajaRandom::HRandom;
        case resource_managers::NoteOrderAlgorithm::BeatorajaAllScr:
            return BeatorajaRandom::AllScr;
        case resource_managers::NoteOrderAlgorithm::BeatorajaSRandomEx:
            return BeatorajaRandom::SRandomEx;
        default:
            throw std::logic_error("Unsupported beatoraja note algorithm");
    }
}

auto
beatorajaNoteOrderIncludesScratch(
  resource_managers::NoteOrderAlgorithm algorithm) -> bool
{
    switch (algorithm) {
        case resource_managers::NoteOrderAlgorithm::BeatorajaRandomEx:
        case resource_managers::NoteOrderAlgorithm::BeatorajaAllScr:
        case resource_managers::NoteOrderAlgorithm::BeatorajaSRandomEx:
            return true;
        default:
            return false;
    }
}

} // namespace support

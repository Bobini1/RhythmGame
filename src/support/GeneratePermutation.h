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

/**
 * @brief Randomizer algorithms supported by beatoraja.
 * @details Maps to the bms.player.beatoraja.pattern.PatternModifier.Random
 * enum in the beatoraja source.
 */
enum class BeatorajaRandom
{
    SRandom,           ///< S-RANDOM: per-timeline shuffle, 40 ms trill threshold
    SRandomNoThreshold,///< S-RANDOM (no threshold): per-timeline, threshold = 0
    SRandomEx,         ///< S-RANDOM-EX: same as H-RANDOM (light assist)
    HRandom,           ///< H-RANDOM: per-timeline, configurable trill threshold
    Spiral,            ///< SPIRAL: cyclic lane rotation, one step per chord
    AllScr,            ///< ALL-SCR: route notes to scratch lane(s) preferentially
    SRandomPlayable,   ///< S-RANDOM-PLAYABLE: avoid physically impossible chords
    Converge,          ///< CONVERGE: maximise consecutive-note streaks per lane
};

/**
 * @brief Generate a per-timeline (beatoraja-style) permutation.
 * @details Replicates beatoraja's Randomizer subclasses exactly, using Java's
 * java.util.Random LCG so that seeds extracted from beatoraja replay files
 * produce identical note orderings.
 * @param notes        Notes to shuffle in-place (scratch column must be last).
 * @param algorithm    Which beatoraja randomizer to use.
 * @param seed         48-bit seed from the beatoraja replay (int64_t >= 0).
 * @param scratchCol   Index of the scratch/turntable column (used by AllScr).
 * @param threshold_ns Trill-avoidance threshold in nanoseconds (used by
 *                     HRandom and the AllScr key-note threshold). Ignored by
 *                     algorithms that have a fixed threshold.
 * @return ShuffleResult whose seed echoes the input seed and whose columns
 *         field is an identity mapping (notes are physically rearranged).
 */
auto
generateBeatorajaPermutation(
  std::span<std::vector<charts::BmsNotesData::Note>>& notes,
  BeatorajaRandom algorithm,
  int64_t seed,
  int scratchCol,
  bool includeScratch,
  int64_t threshold_ns = 100'000'000LL) -> ShuffleResult;

auto
generateBeatorajaLanePermutation(
  std::span<std::vector<charts::BmsNotesData::Note>> notes,
  resource_managers::NoteOrderAlgorithm algorithm,
  int64_t seed,
  int scratchCol,
  bool includeScratch) -> ShuffleResult;

auto
isBeatorajaNoteOrderAlgorithm(resource_managers::NoteOrderAlgorithm algorithm)
  -> bool;

auto
beatorajaRandomFromNoteOrderAlgorithm(
  resource_managers::NoteOrderAlgorithm algorithm) -> BeatorajaRandom;

auto
beatorajaNoteOrderIncludesScratch(
  resource_managers::NoteOrderAlgorithm algorithm) -> bool;
} // namespace support

#endif // GENERATEPERMUTATION_H

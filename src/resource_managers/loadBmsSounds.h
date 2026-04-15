//
// Created by bobini on 16.06.23.
//

#ifndef RHYTHMGAME_LOADBMSSOUNDS_H
#define RHYTHMGAME_LOADBMSSOUNDS_H

#include "charts/BmsNotesData.h"
#include "sounds/AudioEngine.h"

#include <unordered_map>
#include <filesystem>
#include "sounds/Sound.h"

namespace charts {
auto
loadBmsSounds(sounds::AudioEngine* engine,
              const std::unordered_map<uint64_t, std::filesystem::path>& wavs,
              const std::filesystem::path& path)
  -> std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>;

/**
 * @brief Loads and slices sounds for a bmson chart.
 * @param engine The audio engine.
 * @param channelPaths Map from channel index to sound file path.
 * @param slices Slice descriptors from BmsNotesData::bmsonSlices.
 * @param fusions Fusion map from BmsNotesData::bmsonFusions.
 * @param basePath The directory containing the bmson file.
 * @return Map from sound ID to Sound (including MultiSounds for fusions).
 */
auto
loadBmsonSounds(
  sounds::AudioEngine* engine,
  const std::unordered_map<uint64_t, std::filesystem::path>& channelPaths,
  const std::vector<BmsNotesData::BmsonSliceInfo>& slices,
  const std::unordered_map<uint64_t, std::vector<uint64_t>>& fusions,
  const std::filesystem::path& basePath)
  -> std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>;

} // namespace charts

#endif // RHYTHMGAME_LOADBMSSOUNDS_H

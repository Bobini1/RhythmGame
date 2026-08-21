//
// Created by bobini on 16.06.23.
//

#ifndef RHYTHMGAME_LOADBMSSOUNDS_H
#define RHYTHMGAME_LOADBMSSOUNDS_H

#include "charts/BmsNotesData.h"
#include "sounds/AudioEngine.h"

#include <QByteArray>

#include <atomic>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include "sounds/Sound.h"

namespace resource_managers {
class SongAssetStore;
}

namespace charts {
using EncodedSound = std::shared_ptr<const QByteArray>;
using EncodedSounds = std::unordered_map<uint64_t, EncodedSound>;

auto
loadArchivedSoundData(
  resource_managers::SongAssetStore* assetStore,
  const std::filesystem::path& chartDirectory,
  const std::unordered_map<uint64_t, std::filesystem::path>& paths,
  const std::atomic_bool* cancellation = nullptr) -> EncodedSounds;

auto
loadBmsSounds(sounds::AudioEngine* engine,
              const std::unordered_map<uint64_t, std::filesystem::path>& wavs,
              const std::filesystem::path& path,
              const std::atomic_bool* cancellation = nullptr)
  -> std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>;

auto
loadBmsSounds(sounds::AudioEngine* engine,
              const EncodedSounds& wavs,
              const std::atomic_bool* cancellation = nullptr)
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
  const std::filesystem::path& basePath,
  const std::atomic_bool* cancellation = nullptr)
  -> std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>;

auto
loadBmsonSounds(
  sounds::AudioEngine* engine,
  const EncodedSounds& channels,
  const std::vector<BmsNotesData::BmsonSliceInfo>& slices,
  const std::unordered_map<uint64_t, std::vector<uint64_t>>& fusions,
  const std::atomic_bool* cancellation = nullptr)
  -> std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>;

} // namespace charts

#endif // RHYTHMGAME_LOADBMSSOUNDS_H

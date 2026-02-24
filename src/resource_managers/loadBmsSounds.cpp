//
// Created by bobini on 16.06.23.
//

#include "loadBmsSounds.h"

#include "sounds/MultiSound.h"
#include "sounds/NormalSound.h"
#include "sounds/NormalSoundBuffer.h"
#include "sounds/SlicedSoundBuffer.h"
#include "sounds/SoundBuffer.h"
#include "support/PathToQString.h"

#include <optional>
#include <unordered_set>
#include <QtConcurrent>

namespace charts {

#if _WIN32
auto
getActualPathWindows(std::filesystem::path filePath)
  -> std::optional<std::filesystem::path>
{
    if (exists(filePath)) {
        return filePath;
    }
    filePath.replace_extension(".wav");
    if (exists(filePath)) {
        return filePath;
    }
    filePath.replace_extension(".flac");
    if (exists(filePath)) {
        return filePath;
    }
    filePath.replace_extension(".ogg");
    if (exists(filePath)) {
        return filePath;
    }
    filePath.replace_extension(".mp3");
    if (exists(filePath)) {
        return filePath;
    }
    return std::nullopt;
}
#endif

auto
getActualPath(
  std::unordered_map<std::string, std::filesystem::path>& lowerCaseFilesMap,
  std::string filePath) -> std::optional<std::filesystem::path>
{
    if (auto it = lowerCaseFilesMap.find(filePath);
        it != lowerCaseFilesMap.end()) {
        return std::filesystem::path{ it->second };
    }
    filePath.replace(filePath.end() - 3, filePath.end(), "wav");
    if (auto it = lowerCaseFilesMap.find(filePath);
        it != lowerCaseFilesMap.end()) {
        return std::filesystem::path{ it->second };
    }
    filePath.replace(filePath.end() - 3, filePath.end(), "flac");
    if (auto it = lowerCaseFilesMap.find(filePath);
        it != lowerCaseFilesMap.end()) {
        return std::filesystem::path{ it->second };
    }
    filePath.replace(filePath.end() - 4, filePath.end(), "ogg");
    if (auto it = lowerCaseFilesMap.find(filePath);
        it != lowerCaseFilesMap.end()) {
        return std::filesystem::path{ it->second };
    }
    filePath.replace(filePath.end() - 3, filePath.end(), "mp3");
    if (auto it = lowerCaseFilesMap.find(filePath);
        it != lowerCaseFilesMap.end()) {
        return std::filesystem::path{ it->second };
    }
    return std::nullopt;
}

auto
createLowerCaseFilesMap(std::filesystem::path dirToSearch)
  -> std::unordered_map<std::string, std::filesystem::path>
{
    auto lowerCaseFilesMap =
      std::unordered_map<std::string, std::filesystem::path>{};
    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(dirToSearch)) {
        if (entry.is_regular_file()) {
            auto path = entry.path();
            auto pathString = path.filename().string();
            std::ranges::transform(
              pathString, pathString.begin(), [](unsigned char c) {
                  return std::tolower(c);
              });
            lowerCaseFilesMap.emplace(pathString, path);
        }
    }
    return lowerCaseFilesMap;
}

auto
loadBmsSounds(sounds::AudioEngine* engine,
              const std::unordered_map<uint64_t, std::filesystem::path>& wavs,
              const std::filesystem::path& path)
  -> std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>
{
    auto start = std::chrono::high_resolution_clock::now();
    auto wavsActualPaths =
      std::unordered_map<uint64_t, std::filesystem::path>{};
    wavsActualPaths.reserve(wavs.size());
    auto uniqueSoundPaths = std::unordered_set<std::filesystem::path>{};
#ifndef _WIN32
    auto lowerCaseFilesMap = createLowerCaseFilesMap(path);
#endif
    for (const auto& [key, value] : wavs) {
        {
#ifdef _WIN32
            auto filePath = path / value;
            auto actualPath = getActualPathWindows(filePath);
#else
            auto valueLower = value.string();
            std::ranges::transform(
              valueLower, valueLower.begin(), [](unsigned char c) {
                  return std::tolower(c);
              });

            auto filePath = path / value;
            auto actualPath = getActualPath(lowerCaseFilesMap, valueLower);
#endif
            if (!actualPath) {
                spdlog::warn("File {} not found.", filePath.string());
                continue;
            }
            wavsActualPaths.emplace(key, *actualPath);
            uniqueSoundPaths.emplace(std::move(*actualPath));
        }
    }
    std::unordered_map<std::filesystem::path,
                       std::shared_ptr<const sounds::SoundBuffer>>
      buffers;
    buffers.reserve(uniqueSoundPaths.size());

    buffers = QtConcurrent::blockingMappedReduced<
      std::unordered_map<std::filesystem::path,
                         std::shared_ptr<const sounds::SoundBuffer>>>(
      uniqueSoundPaths,
      [engine](const auto& path)
        -> std::optional<
          std::pair<std::filesystem::path,
                    std::shared_ptr<const sounds::SoundBuffer>>> {
          try {
              return { { path,
                         std::make_shared<const sounds::NormalSoundBuffer>(
                           engine, path) } };
          } catch (const std::exception& e) {
              spdlog::warn(
                "Failed to load sound {}: {}", path.string(), e.what());
              return std::nullopt;
          }
      },
      [](auto& container, const auto& pair) -> void {
          if (pair) {
              container.emplace(pair->first, pair->second);
          }
      },
      std::move(buffers));

    auto sounds =
      std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>();
    sounds.reserve(wavsActualPaths.size());
    for (const auto& [key, actualPath] : wavsActualPaths) {
        auto buffer = buffers.find(actualPath);
        if (buffer != buffers.end()) {
            sounds.emplace(
              key,
              std::make_shared<sounds::NormalSound>(engine, buffer->second));
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    spdlog::info(
      "Loading {} sounds took: {} ms",
      buffers.size(),
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
        .count());
    return sounds;
}

auto
loadBmsonSounds(
  sounds::AudioEngine* engine,
  const std::unordered_map<uint64_t, std::filesystem::path>& channelPaths,
  const std::vector<BmsNotesData::BmsonSliceInfo>& slices,
  const std::unordered_map<uint64_t, std::vector<uint64_t>>& fusions,
  const std::filesystem::path& basePath)
  -> std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>
{
    auto start = std::chrono::high_resolution_clock::now();

    // 1. Resolve actual file paths for each channel
    auto channelActualPaths =
      std::unordered_map<uint64_t, std::filesystem::path>{};
    auto uniquePaths = std::unordered_set<std::filesystem::path>{};
#ifndef _WIN32
    auto lowerCaseFilesMap = createLowerCaseFilesMap(basePath);
#endif
    for (const auto& [idx, relPath] : channelPaths) {
#ifdef _WIN32
        auto actualPath = getActualPathWindows(basePath / relPath);
#else
        auto valueLower = relPath.string();
        std::ranges::transform(valueLower,
                               valueLower.begin(),
                               [](unsigned char c) { return std::tolower(c); });
        auto actualPath = getActualPath(lowerCaseFilesMap, valueLower);
#endif
        if (!actualPath) {
            spdlog::warn("Bmson sound not found: {}",
                         (basePath / relPath).string());
            continue;
        }
        channelActualPaths.emplace(idx, *actualPath);
        uniquePaths.emplace(std::move(*actualPath));
    }

    // 2. Load full buffers in parallel
    auto fullBuffers = QtConcurrent::blockingMappedReduced<
      std::unordered_map<std::filesystem::path,
                         std::shared_ptr<const sounds::SoundBuffer>>>(
      uniquePaths,
      [engine](const auto& path)
        -> std::optional<
          std::pair<std::filesystem::path,
                    std::shared_ptr<const sounds::SoundBuffer>>> {
          try {
              return std::pair{
                  path,
                  std::make_shared<const sounds::NormalSoundBuffer>(engine,
                                                                    path)
              };
          } catch (const std::exception& e) {
              spdlog::warn(
                "Failed to load bmson sound {}: {}", path.string(), e.what());
              return std::nullopt;
          }
      },
      [](auto& container, const auto& pair) {
          if (pair) {
              container.emplace(pair->first, pair->second);
          }
      });

    // Map channel index -> full buffer
    auto channelBuffers =
      std::unordered_map<uint64_t,
                         std::shared_ptr<const sounds::SoundBuffer>>{};
    for (const auto& [idx, actualPath] : channelActualPaths) {
        if (auto it = fullBuffers.find(actualPath); it != fullBuffers.end()) {
            channelBuffers[idx] = it->second;
        }
    }

    // 3. Create sliced sounds
    auto sampleRate = static_cast<double>(engine->getSampleRate());
    auto result =
      std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>{};

    for (const auto& slice : slices) {
        auto bufIt = channelBuffers.find(slice.channelIndex);
        if (bufIt == channelBuffers.end()) {
            continue;
        }
        const auto& fullBuffer = bufIt->second;
        auto totalFrames = fullBuffer->getFrames();

        auto startFrame =
          static_cast<ma_uint64>(slice.startSeconds * sampleRate);
        auto endFrame =
          slice.endSeconds < 0.0
            ? totalFrames
            : static_cast<ma_uint64>(slice.endSeconds * sampleRate);
        startFrame = std::min(startFrame, totalFrames);
        endFrame = std::clamp(endFrame, startFrame, totalFrames);
        auto frameCount = endFrame - startFrame;

        auto slicedBuf = std::make_shared<sounds::SlicedSoundBuffer>(
          fullBuffer, startFrame, frameCount);
        result.emplace(
          slice.soundId,
          std::make_shared<sounds::NormalSound>(engine, std::move(slicedBuf)));
    }

    // 4. Create MultiSounds for fused notes
    for (const auto& [fusedId, sliceIds] : fusions) {
        auto children = std::vector<std::shared_ptr<sounds::Sound>>{};
        for (auto sid : sliceIds) {
            if (auto it = result.find(sid); it != result.end()) {
                children.push_back(it->second);
            }
        }
        if (!children.empty()) {
            result.emplace(
              fusedId,
              std::make_shared<sounds::MultiSound>(std::move(children)));
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    spdlog::info(
      "Loading {} bmson sound slices took: {} ms",
      result.size(),
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
        .count());
    return result;
}

} // namespace charts
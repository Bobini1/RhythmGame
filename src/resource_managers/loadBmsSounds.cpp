//
// Created by bobini on 16.06.23.
//

#include "loadBmsSounds.h"
#include "BmsAssetResolver.h"

#include "sounds/MultiSound.h"
#include "sounds/NormalSound.h"
#include "sounds/NormalSoundBuffer.h"
#include "sounds/SlicedSoundBuffer.h"
#include "sounds/SoundBuffer.h"
#include "support/PathToQString.h"
#include "support/PathToUtfString.h"

#include <QtConcurrent>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <unordered_set>

namespace charts {

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
    const auto resolver = BmsAssetResolver::fromDirectory(path);
    if (!resolver.valid()) {
        throw std::runtime_error("BMS asset index failed: " +
                                 resolver.diagnostic());
    }
    for (const auto& [key, value] : wavs) {
        {
            auto filePath = path / value;
            const auto resolved =
              resolver.resolve(support::pathToUtfString(value));
            if (!resolved) {
                spdlog::warn("File {} not found.",
                             support::pathToUtfString(filePath));
                continue;
            }
            wavsActualPaths.emplace(key, resolved->actualPath);
            uniqueSoundPaths.emplace(resolved->actualPath);
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
              spdlog::warn("Failed to load sound {}: {}",
                           support::pathToUtfString(path),
                           e.what());
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
    const auto resolver = BmsAssetResolver::fromDirectory(basePath);
    if (!resolver.valid()) {
        throw std::runtime_error("BMSON asset index failed: " +
                                 resolver.diagnostic());
    }
    for (const auto& [idx, relPath] : channelPaths) {
        const auto resolved =
          resolver.resolve(support::pathToUtfString(relPath));
        if (!resolved) {
            spdlog::warn("Bmson sound not found: {}",
                         support::pathToUtfString(basePath / relPath));
            continue;
        }
        channelActualPaths.emplace(idx, resolved->actualPath);
        uniquePaths.emplace(resolved->actualPath);
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
              spdlog::warn("Failed to load bmson sound {}: {}",
                           support::pathToUtfString(path),
                           e.what());
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
        startFrame = (std::min)(startFrame, totalFrames);
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

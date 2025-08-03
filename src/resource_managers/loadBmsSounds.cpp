//
// Created by bobini on 16.06.23.
//

#include "loadBmsSounds.h"
#include "sounds/OpenAlSoundBuffer.h"
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
loadBmsSounds(const std::unordered_map<uint16_t, std::filesystem::path>& wavs,
              const std::filesystem::path& path)
  -> std::unordered_map<uint16_t, sounds::OpenALSound>
{
    auto start = std::chrono::high_resolution_clock::now();
    auto wavsActualPaths =
      std::unordered_map<uint16_t, std::filesystem::path>{};
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
                       std::shared_ptr<const sounds::OpenALSoundBuffer>>
      buffers;
    buffers.reserve(uniqueSoundPaths.size());

    buffers = QtConcurrent::blockingMappedReduced<
      std::unordered_map<std::filesystem::path,
                         std::shared_ptr<const sounds::OpenALSoundBuffer>>>(
      uniqueSoundPaths,
      [](const auto& path)
        -> std::optional<
          std::pair<std::filesystem::path,
                    std::shared_ptr<const sounds::OpenALSoundBuffer>>> {
          try {
              return { { path,
                         std::make_shared<const sounds::OpenALSoundBuffer>(
                           path) } };
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

    auto sounds = std::unordered_map<uint16_t, sounds::OpenALSound>();
    sounds.reserve(wavsActualPaths.size());
    for (const auto& [key, actualPath] : wavsActualPaths) {
        auto buffer = buffers.find(actualPath);
        if (buffer != buffers.end()) {
            sounds.emplace(key, sounds::OpenALSound(buffer->second));
        }
    }
    for (auto& sound : sounds) {
        sound.second.setVolume(0.5);
    }
    auto end = std::chrono::high_resolution_clock::now();
    spdlog::info(
      "Loading {} sounds took: {} ms",
      buffers.size(),
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
        .count());
    return sounds;
}

} // namespace charts
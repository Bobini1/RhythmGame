//
// Created by bobini on 16.06.23.
//

#include "loadBmsSounds.h"
#include "sounds/OpenAlSoundBuffer.h"
#include <optional>
#include <unordered_set>
#include <QtConcurrent>

namespace charts::helper_functions {

auto
getActualPathWindows(std::filesystem::path filePath)
  -> std::optional<std::filesystem::path>
{
    if (std::filesystem::exists(filePath)) {
        return filePath;
    }
    filePath.replace_extension(".wav");
    if (std::filesystem::exists(filePath)) {
        return filePath;
    }
    filePath.replace_extension(".flac");
    if (std::filesystem::exists(filePath)) {
        return filePath;
    }
    filePath.replace_extension(".ogg");
    if (std::filesystem::exists(filePath)) {
        return filePath;
    }
    filePath.replace_extension(".mp3");
    if (std::filesystem::exists(filePath)) {
        return filePath;
    }
    return std::nullopt;
}

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
    filePath.replace(filePath.end() - 3, filePath.end(), "ogg");
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
            std::transform(pathString.begin(),
                           pathString.end(),
                           pathString.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            lowerCaseFilesMap.emplace(pathString, path);
        }
    }
    return lowerCaseFilesMap;
}

auto
loadBmsSounds(const std::map<std::string, std::string>& wavs,
              const std::filesystem::path& path)
  -> std::unordered_map<std::string, sounds::OpenALSound>
{
    auto start = std::chrono::high_resolution_clock::now();
    auto wavsActualPaths = std::unordered_map<std::string, std::string>{};
    wavsActualPaths.reserve(wavs.size());
    auto uniqueSoundPaths = std::unordered_set<std::string>{};
#ifndef _WIN32
    auto lowerCaseFilesMap = createLowerCaseFilesMap(path);
#endif
    for (const auto& [key, value] : wavs) {
        {
#ifdef _WIN32
            auto filePath = path / QString::fromStdString(value).toWString();
            auto actualPath = getActualPathWindows(filePath);
#else
            // convert to lowercase first
            auto valueLower = value;
            std::transform(valueLower.begin(),
                           valueLower.end(),
                           valueLower.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            auto filePath = path / value;
            auto actualPath = getActualPath(lowerCaseFilesMap, valueLower);
#endif
            if (!actualPath) {
                spdlog::warn("File {} not found.", filePath.string());
                continue;
            }
            auto actualPathString = actualPath->string();
            wavsActualPaths.emplace(key, actualPathString);
            uniqueSoundPaths.emplace(std::move(actualPathString));
        }
    }
    std::unordered_map<std::string,
                       std::shared_ptr<const sounds::OpenALSoundBuffer>>
      buffers;
    buffers.reserve(uniqueSoundPaths.size());

    buffers = QtConcurrent::blockingMappedReduced<
      std::unordered_map<std::string,
                         std::shared_ptr<const sounds::OpenALSoundBuffer>>>(
      uniqueSoundPaths,
      [](const auto& path)
        -> std::optional<
          std::pair<std::string,
                    std::shared_ptr<const sounds::OpenALSoundBuffer>>> {
          try {
              return { { path,
                         std::make_shared<const sounds::OpenALSoundBuffer>(
                           path.c_str()) } };
          } catch (const std::exception& e) {
              spdlog::warn("Failed to load sound {}: {}", path, e.what());
              return std::nullopt;
          }
      },
      [](auto& container, const auto& pair) -> void {
          if (pair) {
              container.emplace(pair->first, pair->second);
          }
      },
      std::move(buffers));

    auto sounds = std::unordered_map<std::string, sounds::OpenALSound>();
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

} // namespace charts::helper_functions
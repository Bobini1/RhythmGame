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
getActualPath(std::filesystem::path filePath)
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
loadBmsSounds(const std::map<std::string, std::string>& wavs,
              const std::filesystem::path& path)
  -> std::unordered_map<std::string, sounds::OpenALSound>
{
    auto wavsActualPaths = std::unordered_map<std::string, std::string>{};
    wavsActualPaths.reserve(wavs.size());
    auto uniqueSoundPaths = std::unordered_set<std::string>{};
    for (const auto& [key, value] : wavs) {
        {
            auto filePath = path / value;
            auto actualPath = getActualPath(filePath);
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
        sound.second.play();
        spdlog::info("playing {}", sound.first);
        while (sound.second.isPlaying())
            ;
        std::this_thread::sleep_for(std::chrono::seconds{ 1 });
    }
    return sounds;
}

} // namespace charts::helper_functions
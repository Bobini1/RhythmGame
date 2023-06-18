//
// Created by bobini on 16.06.23.
//

#include "loadBmsSounds.h"
#include "sounds/OpenAlSoundBuffer.h"
#include <optional>

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
    auto sounds = std::unordered_map<std::string, sounds::OpenALSound>();
    sounds.reserve(wavs.size());
    std::unordered_map<std::string,
                       std::shared_ptr<const sounds::OpenALSoundBuffer>>
      buffers;
    for (const auto& [key, value] : wavs) {
        auto filePath = path / value;
        auto actualPath = getActualPath(filePath);
        if (!actualPath) {
            spdlog::warn("File {} not found.", filePath.string());
            continue;
        }
        auto actualPathString = actualPath->string();
        auto bufferPointer = [&buffers, &actualPathString] {
            auto buffer = buffers.find(actualPathString);
            if (buffer != buffers.end()) {
                return buffers.emplace(actualPathString, buffer->second)
                  .first->second;
            }

            return buffers
              .emplace(actualPathString,
                       std::make_shared<const sounds::OpenALSoundBuffer>(
                         actualPathString.c_str()))
              .first->second;
        }();
        sounds.emplace(key, sounds::OpenALSound(bufferPointer));
    }
    return sounds;
}

} // namespace charts::helper_functions
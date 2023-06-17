//
// Created by bobini on 16.06.23.
//

#include "loadBmsSounds.h"
#include "sounds/OpenAlSoundBuffer.h"

namespace charts::helper_functions {

auto
loadBmsSounds(const std::map<std::string, std::string>& wavs,
              const std::string& path)
  -> std::unordered_map<std::string, sounds::OpenALSound>
{
    auto sounds = std::unordered_map<std::string, sounds::OpenALSound>();
    sounds.reserve(wavs.size());
    std::unordered_map<std::filesystem::path,
                       std::shared_ptr<const sounds::OpenALSoundBuffer>>
      buffers;
    auto rootPath = std::filesystem::path(path).parent_path();
    for (const auto& [key, value] : wavs) {
        auto filePath = rootPath / value;
        auto buffer = buffers.find(filePath);
        auto bufferPointer = [&buffer, &buffers, &filePath] {
            if (buffer != buffers.end()) {
                return buffers.emplace(filePath, buffer->second).first->second;
            }
            return buffers
              .emplace(filePath,
                       std::make_shared<const sounds::OpenALSoundBuffer>(
                         filePath.c_str()))
              .first->second;
        }();
        sounds.emplace(key, sounds::OpenALSound(bufferPointer));
    }
}
} // namespace charts::helper_functions
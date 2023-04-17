//
// Created by bobini on 11.02.23.
//

#ifndef RHYTHMGAME_SOUNDLOADERIMPL_H
#define RHYTHMGAME_SOUNDLOADERIMPL_H
#include <string>
#include <filesystem>
#include <map>
#include <optional>
#include "sounds/OpenAlSound.h"
#include "SoundLoader.h"

namespace resource_managers {
class SoundLoaderImpl
{
    std::map<std::filesystem::path,
             std::shared_ptr<const sounds::OpenALSoundBuffer>>
      loadedSounds;
    std::map<std::string, std::string> redirects;
    std::filesystem::path soundFolder;

  public:
    explicit SoundLoaderImpl(std::filesystem::path soundFolder,
                             std::map<std::string, std::string> redirects);
    /**
     * @brief Loads a sound from a file.
     * @param path Path to the sound file.
     * @return Loaded sound. Always crates a new source but will reuse buffers.
     */
    auto load(const std::string& path) -> std::optional<sounds::OpenALSound>;
};

static_assert(SoundLoader<SoundLoaderImpl>);
} // namespace resource_managers

#endif // RHYTHMGAME_SOUNDLOADERIMPL_H

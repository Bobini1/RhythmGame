//
// Created by bobini on 11.02.23.
//

#include "SoundLoaderImpl.h"
#include "sounds/OpenAlSoundBuffer.h"
#include "sounds/OpenAlSound.h"
resource_managers::SoundLoaderImpl::SoundLoaderImpl(
  std::filesystem::path soundFolder,
  std::map<std::string, std::string> redirects)
  : redirects(std::move(redirects))
  , soundFolder(std::move(soundFolder))
{
}
auto
resource_managers::SoundLoaderImpl::load(std::string path)
  -> std::optional<sounds::OpenALSound>
{
    try {
        if (redirects.contains(path)) {
            path = redirects.at(path);
        }
        auto canonical = std::filesystem::canonical(soundFolder / path);

        if (!loadedSounds.contains(canonical)) {
            loadedSounds.emplace(
              canonical,
              std::make_shared<const sounds::OpenALSoundBuffer>(
                canonical.string().c_str()));
        }
        return sounds::OpenALSound(loadedSounds.at(canonical));
    } catch (std::exception& e) {
        spdlog::error("Failed to load sound: {}", e.what());
        return std::nullopt;
    }
}
//
// Created by bobini on 16.04.23.
//

#ifndef RHYTHMGAME_SOUNDLOADER_H
#define RHYTHMGAME_SOUNDLOADER_H

#include <string>
#include <optional>
#include "sounds/OpenAlSound.h"
namespace resource_managers {

template<typename T>
concept SoundLoader = requires(T soundLoader, std::string path) {
    {
        soundLoader.load(path)
    } -> std::convertible_to<std::optional<sounds::OpenALSound>>;
};

} // namespace resource_managers

#endif // RHYTHMGAME_SOUNDLOADER_H

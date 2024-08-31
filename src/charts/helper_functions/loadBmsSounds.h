//
// Created by bobini on 16.06.23.
//

#ifndef RHYTHMGAME_LOADBMSSOUNDS_H
#define RHYTHMGAME_LOADBMSSOUNDS_H

#include <unordered_map>
#include <map>
#include <string>
#include <filesystem>
#include "sounds/OpenAlSound.h"

namespace charts::helper_functions {
auto
loadBmsSounds(const std::unordered_map<uint16_t, std::filesystem::path>& wavs,
              const std::filesystem::path& path)
  -> std::unordered_map<uint16_t, sounds::OpenALSound>;
} // namespace charts::helper_functions

#endif // RHYTHMGAME_LOADBMSSOUNDS_H

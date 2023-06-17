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
loadBmsSounds(const std::map<std::string, std::string>& wavs,
              const std::filesystem::path& path)
  -> std::unordered_map<std::string, sounds::OpenALSound>;
} // namespace charts::helper_functions

#endif // RHYTHMGAME_LOADBMSSOUNDS_H

//
// Created by bobini on 16.06.23.
//

#ifndef RHYTHMGAME_LOADBMSSOUNDS_H
#define RHYTHMGAME_LOADBMSSOUNDS_H

#include <unordered_map>
#include <filesystem>
#include "sounds/Sound.h"

namespace charts {
auto
loadBmsSounds(GstElement* engine,
              const std::unordered_map<uint16_t, std::filesystem::path>& wavs,
              const std::filesystem::path& path)
  -> std::unordered_map<uint16_t, std::shared_ptr<sounds::Sound>>;
} // namespace charts

#endif // RHYTHMGAME_LOADBMSSOUNDS_H

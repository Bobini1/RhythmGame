#pragma once

#include "audio/PcmSoundBank.h"

#include <filesystem>

namespace web_playtest {

[[nodiscard]] auto
decodeOggVorbis(const std::filesystem::path& path) -> DecodedPcm;

} // namespace web_playtest

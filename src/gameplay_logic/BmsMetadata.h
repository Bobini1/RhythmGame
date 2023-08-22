//
// Created by bobini on 06.07.23.
//

#ifndef RHYTHMGAME_BMSMETADATA_H
#define RHYTHMGAME_BMSMETADATA_H

#include <string>
#include <filesystem>

#include <cryptopp/sha.h>

#include "support/Sha256.h"

namespace gameplay_logic {

// todo: merge this with ChartData
class BmsMetadata
{
    std::filesystem::path path;
    support::Sha256 hash;
    std::string title;
    std::string subtitle;
    std::string artist;
    std::string subartist;
    std::string genre;
    int noteCount;
    int level;
    int total;
    int bpm;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSMETADATA_H

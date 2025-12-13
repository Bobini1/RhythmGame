#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>

// Taken from Lunatic Vibes

namespace support {
struct DXArchiveSegment
{
    size_t size;
    std::shared_ptr<uint8_t[]> data;
};

using DXArchive = std::map<std::string, DXArchiveSegment>;

auto
extractDxaToMem(const std::filesystem::path& path) -> DXArchive;
auto
extractDxaToFile(const std::filesystem::path& path) -> int;
} // namespace support
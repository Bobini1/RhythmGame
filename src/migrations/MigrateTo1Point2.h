//
// Created by PC on 20/09/2025.
//

#ifndef RHYTHMGAME_MIGRATETO1POINT3_H
#define RHYTHMGAME_MIGRATETO1POINT3_H
#include <filesystem>

namespace migrations {

void migrateTo1Point2(const std::filesystem::path& dataFolder);

} // namespace migrations
#endif // RHYTHMGAME_MIGRATETO1POINT3_H

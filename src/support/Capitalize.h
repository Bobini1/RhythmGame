//
// Created by bobini on 26.12.22.
//

#ifndef RHYTHMGAME_CAPITALIZE_H
#define RHYTHMGAME_CAPITALIZE_H

#include <string_view>
namespace support {
[[nodiscard]] auto
capitalize(std::string_view str) -> std::string;
} // namespace support

#endif // RHYTHMGAME_CAPITALIZE_H

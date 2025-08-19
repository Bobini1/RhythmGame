//
// Created by bobini on 06.07.23.
//

#ifndef RHYTHMGAME_SHA256_H
#define RHYTHMGAME_SHA256_H

#include <string>
namespace support {
using Sha256 = std::string;
using Md5 = std::string;

auto
sha256(std::string_view str) -> Sha256;
auto
md5(std::string_view str) -> Md5;

} // namespace support

#endif // RHYTHMGAME_SHA256_H

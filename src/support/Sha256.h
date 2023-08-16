//
// Created by bobini on 06.07.23.
//

#ifndef RHYTHMGAME_SHA256_H
#define RHYTHMGAME_SHA256_H

#include <string>

#include <boost/serialization/strong_typedef.hpp>
#include <cryptopp/sha.h>
namespace support {
BOOST_STRONG_TYPEDEF(std::string, Sha256);

auto
sha256(std::string_view str) -> Sha256;

} // namespace support

#endif // RHYTHMGAME_SHA256_H

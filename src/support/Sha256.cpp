//
// Created by bobini on 06.07.23.
//

#include "Sha256.h"
namespace support {
auto
sha256(std::string_view str) -> Sha256
{
    CryptoPP::SHA256 hash;
    auto digest = std::array<CryptoPP::byte, CryptoPP::SHA256::DIGESTSIZE>{};
    hash.CalculateDigest(digest.data(),
                         reinterpret_cast<const CryptoPP::byte*>(str.data()),
                         str.size());
    return Sha256{ std::string(reinterpret_cast<const char*>(digest.data()),
                               CryptoPP::SHA256::DIGESTSIZE) };
}
} // namespace support
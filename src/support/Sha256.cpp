//
// Created by bobini on 06.07.23.
//

#include "Sha256.h"
#include <cryptopp/hex.h>
#include <array>
namespace support {
auto
sha256(std::string_view str) -> Sha256
{
    CryptoPP::SHA256 hash;
    auto digest = std::array<CryptoPP::byte, CryptoPP::SHA256::DIGESTSIZE>{};
    hash.CalculateDigest(digest.data(),
                         reinterpret_cast<const CryptoPP::byte*>(str.data()),
                         str.size());
    auto hexString = std::string{};
    CryptoPP::HexEncoder encoder;
    encoder.Attach(new CryptoPP::StringSink(hexString));
    encoder.Put(digest.data(), digest.size());
    encoder.MessageEnd();
    return Sha256{ std::move(hexString) };
}
} // namespace support
//
// Created by bobini on 06.07.23.
//

#include "Sha256.h"

#include <QCryptographicHash>
namespace support {
auto sha256(const std::string_view str) -> Sha256
{
    const QByteArray data = QByteArray::fromRawData(str.data(), static_cast<int>(str.size()));
    const QByteArray digest = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    const QByteArray hex = digest.toHex().toUpper();
    return Sha256{ std::string(hex.constData(), hex.size()) };
}

auto md5(const std::string_view str) -> Md5
{
    const QByteArray data = QByteArray::fromRawData(str.data(), static_cast<int>(str.size()));
    const QByteArray digest = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    const QByteArray hex = digest.toHex().toUpper();
    return std::string(hex.constData(), hex.size());
}
} // namespace support
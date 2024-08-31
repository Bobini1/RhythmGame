//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_COMPRESS_H
#define RHYTHMGAME_COMPRESS_H

#include <QByteArray>
#include <QIODevice>
#include <zstd.h>
#include <stdexcept>

namespace support {

template<typename T>
auto
compress(const T& data) -> QByteArray {
    using namespace std::string_literals;
    auto buffer = QByteArray{};
    auto stream = QDataStream{ &buffer, QIODevice::WriteOnly };
    stream << data;
    auto compressedBuffer = QByteArray{};
    compressedBuffer.resize(ZSTD_compressBound(buffer.size()));
    auto compressedSize = ZSTD_compress(compressedBuffer.data(),
                                        compressedBuffer.size(),
                                        buffer.data(),
                                        buffer.size(),
                                        1);
    if (ZSTD_isError(compressedSize)) {
        const auto* error = ZSTD_getErrorName(compressedSize);
        throw std::runtime_error{ "Failed to compress data: "s + error };
    }
    return compressedBuffer.left(compressedSize);
}

template<typename T>
void
decompress(QByteArray data, T& out) {
    using namespace std::string_literals;
    auto decompressedBuffer = QByteArray{};
    auto decompressedSize = ZSTD_getFrameContentSize(data.data(), data.size());
    if (ZSTD_isError(decompressedSize)) {
        const auto* error = ZSTD_getErrorName(decompressedSize);
        throw std::runtime_error{ "Failed to decompress data: "s + error };
    }
    decompressedBuffer.resize(decompressedSize);
    decompressedSize = ZSTD_decompress(decompressedBuffer.data(),
                                       decompressedBuffer.size(),
                                       data.data(),
                                       data.size());
    if (ZSTD_isError(decompressedSize)) {
        const auto* error = ZSTD_getErrorName(decompressedSize);
        throw std::runtime_error{ "Failed to decompress data: "s + error };
    }
    decompressedBuffer.resize(decompressedSize);
    auto stream = QDataStream{ &decompressedBuffer, QIODevice::ReadOnly };
    stream >> out;
}

template<typename T>
T
decompress(QByteArray data) {
    T t;
    decompress(data, t);
    return t;
}

} // namespace support

#endif // RHYTHMGAME_COMPRESS_H

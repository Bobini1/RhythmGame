//
// Created by bobini on 30.09.23.
//

#include <zstd.h>
#include "Compress.h"
#include <stdexcept>

namespace support {
auto
compress(QByteArray data) -> QByteArray
{
    using namespace std::string_literals;
    auto compressedBuffer = QByteArray{};
    compressedBuffer.resize(ZSTD_compressBound(data.size()));
    auto compressedSize = ZSTD_compress(compressedBuffer.data(),
                                        compressedBuffer.size(),
                                        data.data(),
                                        data.size(),
                                        1);
    if (ZSTD_isError(compressedSize)) {
        const auto* error = ZSTD_getErrorName(compressedSize);
        throw std::runtime_error{ "Failed to compress data: "s + error };
    }
    return compressedBuffer.left(compressedSize);
}
auto
decompress(QByteArray data) -> QByteArray
{
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
    return decompressedBuffer;
}
} // namespace support
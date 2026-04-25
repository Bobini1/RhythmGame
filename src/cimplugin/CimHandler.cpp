#include "CimHandler.h"

#include <QByteArray>
#include <QImage>
#include <QIODevice>
#include <QScopeGuard>
#include <QtEndian>
#include <zlib-ng.h>

#include <array>
#include <cstdint>
#include <limits>

namespace {

constexpr qsizetype MaxDecompressedBytes = 512 * 1024 * 1024;

auto
looksLikeZlib(const QByteArray& header) -> bool
{
    if (header.size() < 2) {
        return false;
    }
    const auto cmf = static_cast<unsigned char>(header[0]);
    const auto flg = static_cast<unsigned char>(header[1]);
    return (cmf & 0x0f) == 8 && (((cmf << 8) + flg) % 31) == 0;
}

auto
decompressZlib(const QByteArray& compressed) -> QByteArray
{
    if (compressed.isEmpty()
        || compressed.size() > std::numeric_limits<uint32_t>::max()) {
        return {};
    }

    zng_stream stream{};
    stream.next_in =
      reinterpret_cast<uint8_t*>(const_cast<char*>(compressed.constData()));
    stream.avail_in = static_cast<uint32_t>(compressed.size());

    if (zng_inflateInit(&stream) != Z_OK) {
        return {};
    }
    const auto cleanup = qScopeGuard([&stream] { zng_inflateEnd(&stream); });

    QByteArray output;
    std::array<char, 64 * 1024> buffer{};
    int rc = Z_OK;
    do {
        stream.next_out = reinterpret_cast<uint8_t*>(buffer.data());
        stream.avail_out = static_cast<uint32_t>(buffer.size());
        rc = zng_inflate(&stream, Z_NO_FLUSH);
        if (rc != Z_OK && rc != Z_STREAM_END) {
            return {};
        }
        const auto produced =
          static_cast<qsizetype>(buffer.size() - stream.avail_out);
        if (produced > 0) {
            if (output.size() + produced > MaxDecompressedBytes) {
                return {};
            }
            output.append(buffer.data(), produced);
        }
    } while (rc != Z_STREAM_END);

    return output;
}

auto
bytesPerPixelFor(quint32 format, qsizetype payloadSize, qint64 pixelCount)
  -> int
{
    if (pixelCount <= 0) {
        return 0;
    }
    for (const auto bpp : { 4, 3, 2, 1 }) {
        if (payloadSize == pixelCount * bpp) {
            return bpp;
        }
    }

    // Known CIMs in LR2 skins use 4 for RGBA8888 and 3 for RGB888.
    // Some tools describe older mappings as 0=RGBA, 1=RGB, 2=RGB565,
    // 3=grayscale, so only use this when the payload is long enough.
    const auto canHold = [payloadSize, pixelCount](int bpp) {
        return payloadSize >= pixelCount * bpp;
    };
    if ((format == 4 || format == 0) && canHold(4)) {
        return 4;
    }
    if ((format == 3 || format == 1) && canHold(3)) {
        return 3;
    }
    if (format == 2 && canHold(2)) {
        return 2;
    }
    if (canHold(1)) {
        return 1;
    }
    return 0;
}

void
copyRgb565ToRgba(QImage& image, const uchar* pixels)
{
    for (int y = 0; y < image.height(); ++y) {
        auto* out = image.scanLine(y);
        for (int x = 0; x < image.width(); ++x) {
            const auto offset = (y * image.width() + x) * 2;
            const auto value =
              static_cast<quint16>((pixels[offset] << 8) | pixels[offset + 1]);
            const auto r5 = static_cast<uchar>((value >> 11) & 0x1f);
            const auto g6 = static_cast<uchar>((value >> 5) & 0x3f);
            const auto b5 = static_cast<uchar>(value & 0x1f);
            out[x * 4 + 0] = static_cast<uchar>((r5 << 3) | (r5 >> 2));
            out[x * 4 + 1] = static_cast<uchar>((g6 << 2) | (g6 >> 4));
            out[x * 4 + 2] = static_cast<uchar>((b5 << 3) | (b5 >> 2));
            out[x * 4 + 3] = 255;
        }
    }
}

void
copyGrayToRgba(QImage& image, const uchar* pixels)
{
    for (int y = 0; y < image.height(); ++y) {
        auto* out = image.scanLine(y);
        for (int x = 0; x < image.width(); ++x) {
            const auto v = pixels[y * image.width() + x];
            out[x * 4 + 0] = v;
            out[x * 4 + 1] = v;
            out[x * 4 + 2] = v;
            out[x * 4 + 3] = 255;
        }
    }
}

} // namespace

bool
CimHandler::canRead() const
{
    return canRead(device());
}

bool
CimHandler::canRead(QIODevice* device)
{
    if (!device) {
        return false;
    }
    return looksLikeZlib(device->peek(2));
}

bool
CimHandler::read(QImage* image)
{
    if (!image || !device()) {
        return false;
    }

    const auto data = decompressZlib(device()->readAll());
    if (data.size() < 12) {
        return false;
    }

    const auto* header = reinterpret_cast<const uchar*>(data.constData());
    const auto width = qFromBigEndian<quint32>(header);
    const auto height = qFromBigEndian<quint32>(header + 4);
    const auto format = qFromBigEndian<quint32>(header + 8);
    if (width == 0 || height == 0
        || width > static_cast<quint32>(std::numeric_limits<int>::max())
        || height > static_cast<quint32>(std::numeric_limits<int>::max())) {
        return false;
    }

    const auto pixelCount = static_cast<qint64>(width) * height;
    if (pixelCount <= 0 || pixelCount > std::numeric_limits<int>::max()) {
        return false;
    }

    const auto payloadSize = data.size() - 12;
    const auto bpp = bytesPerPixelFor(format, payloadSize, pixelCount);
    if (bpp == 0 || payloadSize < pixelCount * bpp) {
        return false;
    }

    const auto* pixels = header + 12;
    const auto imageWidth = static_cast<int>(width);
    const auto imageHeight = static_cast<int>(height);

    if (bpp == 4) {
        const QImage wrapped(pixels,
                             imageWidth,
                             imageHeight,
                             imageWidth * 4,
                             QImage::Format_RGBA8888);
        *image = wrapped.copy();
        return !image->isNull();
    }

    if (bpp == 3) {
        const QImage wrapped(pixels,
                             imageWidth,
                             imageHeight,
                             imageWidth * 3,
                             QImage::Format_RGB888);
        *image = wrapped.copy();
        return !image->isNull();
    }

    QImage result(imageWidth, imageHeight, QImage::Format_RGBA8888);
    if (result.isNull()) {
        return false;
    }

    if (bpp == 2) {
        copyRgb565ToRgba(result, pixels);
    } else {
        copyGrayToRgba(result, pixels);
    }

    *image = result;
    return true;
}

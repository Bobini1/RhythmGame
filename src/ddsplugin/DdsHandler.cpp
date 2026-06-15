#include "DdsHandler.h"

#include <OpenImageIO/imageio.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QIODevice>
#include <QTemporaryFile>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

constexpr qint64 MaxImageBytes = 512 * 1024 * 1024;

auto
qStringToOiioPath(const QString& path) -> std::string
{
    return QDir::toNativeSeparators(path).toUtf8().toStdString();
}

auto
deviceFileName(QIODevice* device) -> QString
{
    if (const auto* file = qobject_cast<QFile*>(device)) {
        return file->fileName();
    }
    return {};
}

auto
readOiioImage(const QString& path, QImage* image) -> bool
{
    if (!image || path.isEmpty()) {
        return false;
    }

    auto input = OIIO::ImageInput::open(qStringToOiioPath(path));
    if (!input) {
        return false;
    }

    const auto closeInput = [&input] {
        input->close();
        input.reset();
    };

    const auto& spec = input->spec();
    if (spec.width <= 0 || spec.height <= 0 || spec.nchannels <= 0 ||
        spec.width > std::numeric_limits<int>::max() ||
        spec.height > std::numeric_limits<int>::max()) {
        closeInput();
        return false;
    }

    const auto width = static_cast<int>(spec.width);
    const auto height = static_cast<int>(spec.height);
    const auto channels = static_cast<int>(spec.nchannels);
    const auto pixelCount = static_cast<qint64>(width) * height;
    if (pixelCount <= 0 || pixelCount * 4 > MaxImageBytes ||
        pixelCount > std::numeric_limits<qsizetype>::max() / channels) {
        closeInput();
        return false;
    }

    std::vector<unsigned char> pixels(
      static_cast<std::size_t>(pixelCount) * static_cast<std::size_t>(channels));
    const bool readOk =
      input->read_image(0,
                        0,
                        0,
                        channels,
                        OIIO::TypeDesc::UINT8,
                        pixels.data());
    closeInput();
    if (!readOk) {
        return false;
    }

    QImage result(width, height, QImage::Format_RGBA8888);
    if (result.isNull()) {
        return false;
    }

    for (int y = 0; y < height; ++y) {
        auto* destination = result.scanLine(y);
        for (int x = 0; x < width; ++x) {
            const auto sourceIndex =
              (static_cast<std::size_t>(y) * static_cast<std::size_t>(width)
               + static_cast<std::size_t>(x))
              * static_cast<std::size_t>(channels);
            const auto* source = pixels.data() + sourceIndex;

            const auto r = source[0];
            const auto g = channels >= 3 ? source[1] : r;
            const auto b = channels >= 3 ? source[2] : r;
            const auto a = channels >= 4
              ? source[3]
              : (channels == 2 ? source[1] : static_cast<unsigned char>(255));

            const auto destinationIndex = x * 4;
            destination[destinationIndex + 0] = r;
            destination[destinationIndex + 1] = g;
            destination[destinationIndex + 2] = b;
            destination[destinationIndex + 3] = a;
        }
    }

    *image = result;
    return true;
}

auto
readDeviceWithTemporaryFile(QIODevice* device, QImage* image) -> bool
{
    if (!device) {
        return false;
    }
    if (!device->isSequential() && !device->seek(0)) {
        return false;
    }

    QTemporaryFile temporaryFile(QDir::tempPath() +
                                 QStringLiteral("/RhythmGameDdsXXXXXX.dds"));
    if (!temporaryFile.open()) {
        return false;
    }

    const auto data = device->readAll();
    if (data.isEmpty() || temporaryFile.write(data) != data.size()) {
        return false;
    }

    temporaryFile.close();
    return readOiioImage(temporaryFile.fileName(), image);
}

} // namespace

bool
DdsHandler::canRead() const
{
    return canRead(device());
}

bool
DdsHandler::canRead(QIODevice* device)
{
    return device && device->peek(4) == QByteArrayLiteral("DDS ");
}

bool
DdsHandler::read(QImage* image)
{
    if (!image || !device()) {
        return false;
    }

    const auto fileName = deviceFileName(device());
    if (!fileName.isEmpty()) {
        if (readOiioImage(fileName, image)) {
            return true;
        }
    }

    return readDeviceWithTemporaryFile(device(), image);
}

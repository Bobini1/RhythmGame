#include "DxaImageProvider.h"

#include "support/QStringToPath.h"
#include "support/dxa.h"
#include <SDL2/SDL_image.h>

#include <QBuffer>
#include <QImageReader>
#include <mutex>
#include <unordered_map>
#include <spdlog/spdlog.h>

namespace resource_managers {
void
DxaImageProvider::loadArchive(const std::filesystem::path& path)
{
    if (loadedArchives.find(path) != loadedArchives.end()) {
        return;
    }
    auto archive = support::extractDxaToMem(path);
    if (!archive.empty()) {
        loadedArchives[path] = std::move(archive);
    }
}

void
DxaImageProvider::unloadArchive(const std::string& name)
{
    loadedArchives.erase(name);
}

void
DxaImageProvider::clearArchives()
{
    loadedArchives.clear();
}

DxaImageProvider::DxaImageProvider()
  : QQuickImageProvider(Pixmap)
{
}

QPixmap
DxaImageProvider::requestPixmap(const QString& id,
                                QSize* size,
                                const QSize& requestedSize)
{
    std::lock_guard lock(archiveMutex);

    // Parse id: "archiveName:path/to/file.png"
    int separatorIndex = id.lastIndexOf(".dxa/", Qt::CaseInsensitive);
    if (separatorIndex == -1) {
        return {};
    }

    auto archiveName = support::qStringToPath(id.left(separatorIndex + 4));
    auto filePath = support::qStringToPath(id.mid(separatorIndex + 5));

    loadArchive(archiveName);
    auto archiveIt = loadedArchives.find(archiveName);
    if (archiveIt == loadedArchives.end()) {
        return {};
    }

    const auto& archive = archiveIt->second;

    // Look up file in archive
    auto it = archive.find(weakly_canonical(filePath).string());
    if (it == archive.end()) {
        return {};
    }

    const auto& segment = it->second;

    QByteArray imageData(reinterpret_cast<const char*>(segment.data.get()),
                         static_cast<qsizetype>(segment.size));
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::ReadOnly);

    QImageReader reader(&buffer);
    QImage image = reader.read();

    if (image.isNull()) {
        return {};
    }

    if (size) {
        *size = image.size();
    }

    QPixmap pixmap = QPixmap::fromImage(image);
    if (requestedSize.isValid() && requestedSize != pixmap.size()) {
        pixmap = pixmap.scaled(
          requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    return pixmap;
}

} // namespace resource_managers

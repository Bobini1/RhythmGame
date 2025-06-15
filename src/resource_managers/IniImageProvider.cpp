//
// Created by bobini on 04.09.23.
//

#include <QSettings>
#include "IniImageProvider.h"

#include <QFile>
#include <spdlog/spdlog.h>

namespace resource_managers {
auto
IniImageProvider::requestPixmap(const QString& id,
                                QSize* size,
                                const QSize& requestedSize) -> QPixmap
{
    // remove the last part of the url
    QString path = id;
    path.remove(path.lastIndexOf('/'), path.length());
    // load the ini file
    QString pathToIni = path;
    pathToIni.append(".ini");
    if (!QFile(pathToIni).exists()) {
        // use folder.ini if the file does not exist
        pathToIni = path;
        pathToIni.remove(path.lastIndexOf('/'), path.length());
        pathToIni.append("/folder.ini");
        if (!QFile(pathToIni).exists()) {
            spdlog::warn("Could not find the ini file for {}",
                         id.toStdString());
            return {};
        }
    }
    const auto settings = QSettings(pathToIni, QSettings::IniFormat);
    // get the key identified by the last part of the url
    QString rectName = id;
    rectName.remove(0, rectName.lastIndexOf('/') + 1);
    // get the size and position of the image
    auto rect = settings.value(rectName).toRect();
    if (size != nullptr) {
        *size = rect.size();
    }
    if (rect.isNull()) {
        return {};
    }
    const auto& pixmap = [this, rect, &path, &settings]() -> const QPixmap& {
        auto pixmapsLock = std::unique_lock{pixmapsMutex};
        const auto cachedPixmap = pixmaps.find(path);
        if (cachedPixmap != pixmaps.end()) {
            return cachedPixmap.value()[rect];
        }
        pixmapsLock.unlock();
        // loading to QImage first to automatically convert from RGBA to RGB in
        // QPixmap::fromImage
        const auto image = QImage(path);
        auto cuts = QHash<QRect, QPixmap>{};
        for (const auto& key : settings.allKeys()) {
            auto pixRect = settings.value(key).toRect();
            auto cut = image.copy(pixRect);
            cuts.emplace(pixRect, QPixmap::fromImage(cut));
        }
        pixmapsLock.lock();
        return (*pixmaps.emplace(path, std::move(cuts)))[rect];
    }();
    if (requestedSize.isValid()) {
        return pixmap.scaled(requestedSize);
    }
    return pixmap;
}
IniImageProvider::
IniImageProvider()
  : QQuickImageProvider(Pixmap)
{
}
} // namespace resource_managers
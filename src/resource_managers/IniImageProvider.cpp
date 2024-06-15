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
        path.remove(path.lastIndexOf('/'), path.length());
        pathToIni = path;
        pathToIni.append("/folder.ini");
        if (!QFile(pathToIni).exists()) {
            spdlog::warn("Could not find the ini file for {}",
                         id.toStdString());
            return {};
        }
    }
    const auto settings = QSettings(pathToIni, QSettings::IniFormat);
    // get the key identified by the last part of the url
    QString key = id;
    key.remove(0, key.lastIndexOf('/') + 1);
    // get the size and position of the image
    auto rect = settings.value(key).toRect();
    if (size != nullptr) {
        *size = rect.size();
    }
    if (rect.isNull()) {
        return {};
    }
    auto pixmap = [this, rect, &path] {
        const auto cachedPixmap = pixmaps.find(path);
        if (cachedPixmap == pixmaps.end()) {
            const auto pixmap = pixmaps.emplace(path, QPixmap(path));
            return pixmap->copy(rect);
        }
        return cachedPixmap->copy(rect);
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
//
// Created by bobini on 04.09.23.
//

#include <QSettings>
#include "IniImageProvider.h"

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
    auto settings = QSettings(pathToIni, QSettings::IniFormat);
    // get the key identified by the last part of the url
    QString key = id;
    key.remove(0, key.lastIndexOf('/') + 1);
    // get the size and position of the image
    auto rect = settings.value(key).toRect();
    if (size) {
        *size = rect.size();
    }
    auto pixmap = [this, rect, &path] {
        auto cachedPixmap = pixmaps.find(path);
        if (cachedPixmap == pixmaps.end()) {
            auto pixmap = pixmaps.emplace(path, QPixmap(path));
            return pixmap->copy(rect);
        }
        return cachedPixmap->copy(rect);
    }();
    // resize if requested
    if (requestedSize.isValid()) {
        // do not interpolate
        pixmap = pixmap.scaled(
          requestedSize, Qt::KeepAspectRatio, Qt::FastTransformation);
    }
    return pixmap;
}
IniImageProvider::IniImageProvider()
  : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}
} // namespace resource_managers
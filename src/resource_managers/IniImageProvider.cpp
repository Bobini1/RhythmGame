//
// Created by bobini on 04.09.23.
//

#include <QSettings>
#include "IniImageProvider.h"

namespace resource_managers {

QPixmap
IniImageProvider::requestPixmap(const QString& id,
                                QSize* size,
                                const QSize& requestedSize)
{
    // remove the last part of the url
    QString path = id;
    path.remove(path.lastIndexOf('/'), path.length());
    // load the image
    auto pixmap = QPixmap(path);
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
    // resize if requested
    if (requestedSize.isValid()) {
        pixmap = pixmap.scaled(requestedSize);
    }
    return pixmap;
}
IniImageProvider::IniImageProvider()
  : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}
} // namespace resource_managers
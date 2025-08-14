//
// Created by bobini on 27.12.22.
//

#include "FindAssetsFolder.h"

#include "support/QStringToPath.h"

#include <QDir>
#include <QGuiApplication>

#include <QString>
auto
resource_managers::findAssetsFolder() -> std::filesystem::path
{
    static const auto assetsFolder = []() -> std::filesystem::path {
        const auto appPath = QFileInfo(QGuiApplication::applicationFilePath());
        QDir dir = appPath.dir();
        dir.cdUp();
        const auto path = dir.absoluteFilePath("assets");
        return support::qStringToPath(path);
    }();
    return assetsFolder;
}
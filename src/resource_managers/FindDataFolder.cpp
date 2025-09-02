//
// Created by bobini on 27.12.22.
//

#include "FindDataFolder.h"

#include "support/QStringToPath.h"

#include <QDir>
#include <QGuiApplication>

#include <QString>
auto
resource_managers::findDataFolder() -> std::filesystem::path
{
    static const auto dataFolder = []() -> std::filesystem::path {
        const auto appPath = QFileInfo(QGuiApplication::applicationFilePath());
        const auto fsPath = appPath.dir().filesystemPath();
        return canonical(fsPath / RHYTHMGAME_DATA_PATH);
    }();
    return dataFolder;
}
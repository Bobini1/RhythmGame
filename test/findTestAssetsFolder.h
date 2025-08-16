//
// Created by bobini on 16.04.23.
//

#ifndef RHYTHMGAME_FINDTESTASSETSFOLDER_H
#define RHYTHMGAME_FINDTESTASSETSFOLDER_H
#include "support/QStringToPath.h"

#include <QGuiApplication>
#include <QDir>
#include <filesystem>
inline auto
findTestAssetsFolder() -> std::filesystem::path
{
    static const auto assetsFolder = []() -> std::filesystem::path {
        const auto appPath = QFileInfo(QGuiApplication::applicationFilePath());
        QDir dir = appPath.dir();
        dir.cdUp();
        const auto path = dir.absoluteFilePath("testOnlyAssets");
        return support::qStringToPath(path);
    }();
    return assetsFolder;
}

#endif // RHYTHMGAME_FINDTESTASSETSFOLDER_H

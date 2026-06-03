#pragma once

#include "gameplay_logic/lr2_skin/Lr2SkinParser.h"

#include <QVariantMap>

#include <filesystem>
#include <set>

namespace gameplay_logic::lr2_skin {

struct BeatorajaLuaSkinHeader
{
    QVariantMap raw;
    QString settingsData;
    int typeId = -1;
    QString title;
    QString maker;
    bool valid = false;
};

auto
isBeatorajaLuaSkinPath(const std::filesystem::path& path) -> bool;

auto
loadBeatorajaLuaSkinHeader(const std::filesystem::path& skinPath)
  -> BeatorajaLuaSkinHeader;

auto
parseBeatorajaLuaSkin(const std::filesystem::path& skinPath,
                      const QVariantMap& settingValues,
                      const std::set<int>& initialOptions) -> Lr2SkinData;

} // namespace gameplay_logic::lr2_skin

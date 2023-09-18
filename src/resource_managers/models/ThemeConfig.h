//
// Created by bobini on 13.08.23.
//

#ifndef RHYTHMGAME_THEMECONFIG_H
#define RHYTHMGAME_THEMECONFIG_H

#include <QUrl>
namespace resource_managers::models {
struct ThemeConfig
{
    QString mainScene;
    QString gameplayScene;
    QString songWheelScene;
    QString SettingScene;
};
} // namespace resource_managers::models

#endif // RHYTHMGAME_THEMECONFIG_H

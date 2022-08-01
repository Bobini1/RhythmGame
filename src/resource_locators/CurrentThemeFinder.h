//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_CURRENTTHEMEFINDER_H
#define RHYTHMGAME_CURRENTTHEMEFINDER_H

#include <filesystem>
namespace resource_locators {
class CurrentThemeFinder
{
  public:
    virtual auto findTheme() -> std::filesystem::path = 0;

    virtual ~CurrentThemeFinder() = default;
};
} // namespace resource_locators
#endif // RHYTHMGAME_CURRENTTHEMEFINDER_H

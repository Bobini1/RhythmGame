//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_THEMESECTIONSFINDER_H
#define RHYTHMGAME_THEMESECTIONSFINDER_H
#include <filesystem>
namespace resource_locators {
class ThemeSectionsFinder
{
  public:
    virtual auto findThemeSections(const std::string& screen)
      -> std::filesystem::path = 0;

    virtual ~ThemeSectionsFinder() = default;
};
} // namespace resource_locators

#endif // RHYTHMGAME_THEMESECTIONSFINDER_H

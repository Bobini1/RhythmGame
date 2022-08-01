//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_ROOTFINDER_H
#define RHYTHMGAME_ROOTFINDER_H

#include <filesystem>
namespace resource_locators {
class RootFinder
{
  public:
    virtual auto findRoot() -> std::filesystem::path;

    virtual ~RootFinder() = default;
};
} // namespace resource_locators

#endif // RHYTHMGAME_ROOTFINDER_H

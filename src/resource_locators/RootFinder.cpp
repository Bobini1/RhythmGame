//
// Created by bobini on 31.07.2022.
//

#include "RootFinder.h"
namespace resource_locators {
auto
RootFinder::findRoot() -> std::filesystem::path
{
    return std::filesystem::current_path();
}
} // namespace resource_locators
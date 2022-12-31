//
// Created by bobini on 16.11.22.
//

#ifndef RHYTHMGAME_FONTLOADERIMPL_H
#define RHYTHMGAME_FONTLOADERIMPL_H

#include <filesystem>
#include <SFML/Graphics/Font.hpp>
#include "resource_managers/AssetsFolderFinder.h"
namespace resource_managers {
class FontLoaderImpl
{
    std::map<std::filesystem::path, std::unique_ptr<sf::Font>> loadedFonts;
    std::map<std::string, std::string> redirects;
    std::filesystem::path fontFolder;

  public:
    // NOTE: expect this to change once we implement changing themes
    explicit FontLoaderImpl(std::filesystem::path fontFolder,
                            std::map<std::string, std::string> redirects);
    /**
     * @brief Loads a font from a file.
     * @param path Path to the font file.
     * @return Loaded font.
     */
    auto load(std::string path) -> const sf::Font*;
    auto getDefault() -> const sf::Font*;
};
} // namespace resource_managers

#endif // RHYTHMGAME_FONTLOADERIMPL_H

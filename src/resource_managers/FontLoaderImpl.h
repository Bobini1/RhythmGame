//
// Created by bobini on 16.11.22.
//

#ifndef RHYTHMGAME_FONTLOADERIMPL_H
#define RHYTHMGAME_FONTLOADERIMPL_H

#include <filesystem>
#include <SFML/Graphics/Font.hpp>
namespace resource_managers {
class FontLoaderImpl
{
    std::map<std::filesystem::path, std::unique_ptr<sf::Font>> loadedFonts;

  public:
    /**
     * @brief Loads a font from a file.
     * @param path Path to the font file.
     * @return Loaded font.
     */
    auto load(const std::string& path) -> const sf::Font*;
    auto getDefault() -> const sf::Font*;
};
} // namespace resource_managers

#endif // RHYTHMGAME_FONTLOADERIMPL_H

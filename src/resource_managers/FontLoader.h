//
// Created by bobini on 16.11.22.
//

#ifndef RHYTHMGAME_FONTLOADER_H
#define RHYTHMGAME_FONTLOADER_H

#include <filesystem>
#include <SFML/Graphics/Font.hpp>
namespace resource_managers {
template<typename T>
concept FontLoader = requires(T fontLoader, const std::string& path) {
                         {
                             fontLoader.load(path)
                         } -> std::convertible_to<const sf::Font*>;
                         {
                             fontLoader.getDefault()
                         } -> std::convertible_to<const sf::Font*>;
                     };
} // namespace resource_managers

#endif // RHYTHMGAME_FONTLOADER_H

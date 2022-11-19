//
// Created by bobini on 15.11.22.
//

#ifndef RHYTHMGAME_TEXTURELOADER_H
#define RHYTHMGAME_TEXTURELOADER_H

#include <filesystem>
#include <SFML/Graphics/Texture.hpp>
namespace resource_managers {
/**
 * @brief Loads textures from files.
 */
template<typename T>
concept TextureLoader = requires(T textureLoader, const std::string& path) {
                            /**
                             * @brief Loads a texture from a file.
                             * @param path Path to the texture file.
                             * @return Loaded texture.
                             */
                            {
                                textureLoader.load(path)
                            } -> std::convertible_to<const sf::Texture*>;
                        };
} // namespace resource_managers
#endif // RHYTHMGAME_TEXTURELOADER_H

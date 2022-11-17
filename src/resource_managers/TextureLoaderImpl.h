//
// Created by bobini on 14.11.22.
//

#ifndef RHYTHMGAME_TEXTURELOADERIMPL_H
#define RHYTHMGAME_TEXTURELOADERIMPL_H

#include <filesystem>
#include <map>
#include <SFML/Graphics/Texture.hpp>
#include "TextureLoader.h"
namespace resource_managers {
class TextureLoaderImpl
{
    std::map<std::filesystem::path, std::unique_ptr<sf::Texture>>
      loadedTextures;

  public:
    /**
     * @brief Loads a texture from a file.
     * @param path Path to the texture file.
     * @return Loaded texture.
     */
    auto load(const std::filesystem::path& path) -> const sf::Texture*;
};

static_assert(resource_managers::TextureLoader<TextureLoaderImpl>);
} // namespace resource_managers
#endif // RHYTHMGAME_TEXTURELOADERIMPL_H

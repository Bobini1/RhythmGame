//
// Created by bobini on 14.11.22.
//

#include "TextureLoaderImpl.h"
#include <spdlog/spdlog.h>
auto
resource_managers::TextureLoaderImpl::load(const std::string& path)
  -> const sf::Texture*
{
    if (!std::filesystem::exists(path)) {
        spdlog::error("Texture file {} does not exist", path);
        return nullptr;
    }
    auto absPath = std::filesystem::canonical(path);
    if (loadedTextures.find(absPath) == loadedTextures.end()) {
        auto texture = std::make_unique<sf::Texture>();
        if (!texture->loadFromFile(absPath.string())) {
            spdlog::error("Failed to load texture from file {}", path);
            return nullptr;
        }
        loadedTextures[absPath] = std::move(texture);
    }
    return loadedTextures[absPath].get();
}

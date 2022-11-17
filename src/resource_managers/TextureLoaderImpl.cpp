//
// Created by bobini on 14.11.22.
//

#include "TextureLoaderImpl.h"
#include <boost/log/trivial.hpp>
auto
resource_managers::TextureLoaderImpl::load(const std::filesystem::path& path)
  -> const sf::Texture*
{
    auto absPath = std::filesystem::canonical(path);
    if (loadedTextures.find(absPath) == loadedTextures.end()) {
        auto texture = std::make_unique<sf::Texture>();
        if (!texture->loadFromFile(absPath.string())) {
            BOOST_LOG_TRIVIAL(error)
              << "Failed to load texture from file " << path;
            return nullptr;
        }
        loadedTextures[absPath] = std::move(texture);
    }
    return loadedTextures[absPath].get();
}

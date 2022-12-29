//
// Created by bobini on 14.11.22.
//

#include "TextureLoaderImpl.h"
#include <spdlog/spdlog.h>
auto
resource_managers::TextureLoaderImpl::load(std::string path)
  -> const sf::Texture*
{
    if (redirects.contains(path)) {
        path = redirects.at(path);
    }
    auto fullPath = rootFolder / path;
    if (!std::filesystem::exists(fullPath)) {
        spdlog::error("Texture file {} does not exist", fullPath.string());
        return nullptr;
    }
    auto absPath = std::filesystem::canonical(fullPath);
    if (loadedTextures.find(absPath) == loadedTextures.end()) {
        auto texture = std::make_unique<sf::Texture>();
        if (!texture->loadFromFile(absPath.string())) {
            spdlog::error("Failed to load texture from file {}",
                          absPath.string());
            return nullptr;
        }
        loadedTextures[absPath] = std::move(texture);
    }
    return loadedTextures[absPath].get();
}
resource_managers::TextureLoaderImpl::TextureLoaderImpl(
  std::filesystem::path rootFolder,
  std::map<std::string, std::string> redirects)
  : redirects(std::move(redirects))
  , rootFolder(std::move(rootFolder))
{
}
auto
resource_managers::TextureLoaderImpl::getFallback() -> const sf::Texture*
{
    return load("Fallback");
}

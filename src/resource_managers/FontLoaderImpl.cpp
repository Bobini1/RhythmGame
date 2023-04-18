//
// Created by bobini on 16.11.22.
//

#include <spdlog/spdlog.h>
#include "FontLoaderImpl.h"
auto
resource_managers::FontLoaderImpl::load(std::string path) -> const sf::Font*
{
    if (redirects.contains(path)) {
        path = redirects.at(path);
    }
    auto fullPath = fontFolder / path;
    if (!std::filesystem::exists(fullPath)) {
        spdlog::error("Font file {} does not exist", fullPath.string());
        return nullptr;
    }
    auto pathAbs = std::filesystem::canonical(fullPath);
    if (const auto& font = loadedFonts.find(pathAbs);
        font != loadedFonts.end()) {
        return font->second.get();
    }

    auto font = std::make_unique<sf::Font>();
    if (!font->loadFromFile(pathAbs.string())) {
        return nullptr;
    }
    loadedFonts[pathAbs] = std::move(font);
    return loadedFonts[pathAbs].get();
}

auto
resource_managers::FontLoaderImpl::getDefault() -> const sf::Font*
{
    return load("Common");
}
resource_managers::FontLoaderImpl::FontLoaderImpl(
  std::filesystem::path fontFolder,
  std::map<std::string, std::string> redirects)
  : redirects(std::move(redirects))
  , fontFolder(std::move(fontFolder))
{
}

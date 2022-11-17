//
// Created by bobini on 16.11.22.
//

#include "FontLoaderImpl.h"
auto
resource_managers::FontLoaderImpl::load(const std::filesystem::path& path)
  -> const sf::Font*
{
    auto pathAbs = std::filesystem::canonical(path);
    if (loadedFonts.find(pathAbs) == loadedFonts.end()) {
        auto font = sf::Font{};
        if (!font.loadFromFile(pathAbs.string())) {
            return nullptr;
        }
        loadedFonts[pathAbs] = std::make_unique<sf::Font>(std::move(font));
    }
    return loadedFonts[pathAbs].get();
}

auto
resource_managers::FontLoaderImpl::getDefault() -> const sf::Font*
{
    return load("/home/bobini/RhythmGame/Roboto/Roboto-Regular.ttf");
}
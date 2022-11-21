//
// Created by bobini on 16.11.22.
//

#include <spdlog/spdlog.h>
#include "FontLoaderImpl.h"
auto
resource_managers::FontLoaderImpl::load(const std::string& path)
  -> const sf::Font*
{
    if (!std::filesystem::exists(path)) {
        spdlog::error("Font file {} does not exist", path);
        return nullptr;
    }
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
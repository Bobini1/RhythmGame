//
// Created by bobini on 10.08.2022.
//

#ifndef RHYTHMGAME_BOOTSTRAPPER_H
#define RHYTHMGAME_BOOTSTRAPPER_H

#include <sol/state.hpp>
#include "resource_managers/TextureLoader.h"
#include "resource_managers/FontLoader.h"
#include "drawing/actors/Sprite.h"
#include "SFML/Graphics/Font.hpp"
#include "drawing/actors/Text.h"

namespace lua {

template<typename T>
concept HasWritableProperties = std::derived_from<T, drawing::actors::Actor> &&
                                requires(T* actor, sol::object& self) {
                                    {
                                        actor->setMinHeight(0.F)
                                    } -> std::same_as<void>;
                                    {
                                        actor->setMinWidth(0.F)
                                    } -> std::same_as<void>;
                                    {
                                        actor->setIsWidthManaged(true)
                                    } -> std::same_as<void>;
                                    {
                                        actor->setIsHeightManaged(true)
                                    } -> std::same_as<void>;
                                };

/**
 * Since sol does not seem to allow overriding readonly properties from
 * base classes with read/write properties, we can't define those in the Actor
 * class directly. This function writes those properties as needed.
 */
template<HasWritableProperties T>
auto
createActorBaseProperties(sol::usertype<T> actorType) -> void
{
    actorType["minWidth"] = sol::property(&T::getMinWidth, &T::setMinWidth);
    actorType["minHeight"] = sol::property(&T::getMinHeight, &T::setMinHeight);
    actorType["isWidthManaged"] =
      sol::property(&T::getIsWidthManaged, &T::setIsWidthManaged);
    actorType["isHeightManaged"] =
      sol::property(&T::getIsHeightManaged, &T::setIsHeightManaged);
}

template<std::derived_from<drawing::actors::Actor> T>
auto
createActorBaseProperties(sol::usertype<T> actorType)
    requires(!HasWritableProperties<T>)
{
    actorType["minWidth"] = sol::property(&T::getMinWidth);
    actorType["minHeight"] = sol::property(&T::getMinHeight);
    actorType["isWidthManaged"] = sol::property(&T::getIsWidthManaged);
    actorType["isHeightManaged"] = sol::property(&T::getIsHeightManaged);
}

class Bootstrapper
{
  public:
    auto defineCommonTypes(sol::state& target) const -> void;

    template<resource_managers::FontLoader FontLoaderType>
    auto bindFontLoader(sol::state& target, FontLoaderType& loader) const
      -> void
    {
        defineFont(target, loader);
        defineText(target, loader);
    }

    template<resource_managers::FontLoader FontLoaderType>
    auto defineFont(sol::state& target, FontLoaderType& fontLoader) const
      -> void
    {
        auto fontType = target.new_usertype<sf::Font>(
          "Font", "get", sol::factories([&fontLoader](std::string filename) {
              return fontLoader.load(filename);
          }));
    }

    template<resource_managers::FontLoader FontLoaderType>
    auto defineText(sol::state& target, FontLoaderType& fontLoader) const
      -> void
    {
        auto textType = target.new_usertype<drawing::actors::Text>(
          "Text",
          sol::factories(
            []() { return std::make_shared<drawing::actors::Text>(); },
            [&fontLoader](const std::string& text) {
                auto* font = fontLoader.getDefault();
                if (font == nullptr) {
                    throw std::runtime_error("Default font not found");
                }
                return std::make_shared<drawing::actors::Text>(text, *font);
            },
            [](const std::string& text, sf::Font& font) {
                return std::make_shared<drawing::actors::Text>(text, font);
            },
            [](const std::string& text, sf::Font& font, int characterSize) {
                return std::make_shared<drawing::actors::Text>(
                  text, font, characterSize);
            },
            [&fontLoader](sol::table args) {
                const auto* defaultFont = fontLoader.getDefault();
                const auto* font = args.get<const sf::Font*>(
                  "font"); // get_or broken in this case as of 17.11.22.
                           // (https://github.com/ThePhD/sol2/issues/1421)
                if (font == nullptr) {
                    font = defaultFont;
                }
                if (font == nullptr) {
                    throw std::runtime_error("Default font not found");
                }
                auto result = std::make_shared<drawing::actors::Text>(
                  args.get_or<std::string>("text", ""),
                  *font,
                  args.get_or("characterSize",
                              drawing::actors::Text::defaultFontSize));
                if (args["fillColor"].valid()) {
                    result->setFillColor(args.get<sf::Color>("fillColor"));
                }
                if (args["outlineColor"].valid()) {
                    result->setOutlineColor(
                      args.get<sf::Color>("outlineColor"));
                }
                if (args["outlineThickness"].valid()) {
                    result->setOutlineThickness(
                      args.get<float>("outlineThickness"));
                }
                if (args["style"].valid()) {
                    result->setStyle(args.get<sf::Uint32>("style"));
                }
                if (args["lineSpacing"].valid()) {
                    result->setLineSpacing(args.get<float>("lineSpacing"));
                }
                if (args["letterSpacing"].valid()) {
                    result->setLetterSpacing(args.get<float>("letterSpacing"));
                }
                return result;
            }),
          sol::base_classes,
          sol::bases<drawing::actors::Actor,
                     drawing::actors::AbstractRectLeaf>());
        textType["string"] = sol::property(&drawing::actors::Text::getString,
                                           &drawing::actors::Text::setString);
        textType["font"] = sol::property(&drawing::actors::Text::getFont,
                                         &drawing::actors::Text::setFont);
        textType["characterSize"] =
          sol::property(&drawing::actors::Text::getCharacterSize,
                        &drawing::actors::Text::setCharacterSize);
        textType["fillColor"] =
          sol::property(&drawing::actors::Text::getFillColor,
                        &drawing::actors::Text::setFillColor);
        textType["outlineColor"] =
          sol::property(&drawing::actors::Text::getOutlineColor,
                        &drawing::actors::Text::setOutlineColor);
        textType["outlineThickness"] =
          sol::property(&drawing::actors::Text::getOutlineThickness,
                        &drawing::actors::Text::setOutlineThickness);
        textType["style"] = sol::property(&drawing::actors::Text::getStyle,
                                          &drawing::actors::Text::setStyle);
        textType["lineSpacing"] =
          sol::property(&drawing::actors::Text::getLineSpacing,
                        &drawing::actors::Text::setLineSpacing);
        textType["letterSpacing"] =
          sol::property(&drawing::actors::Text::getLetterSpacing,
                        &drawing::actors::Text::setLetterSpacing);
    }

    template<resource_managers::TextureLoader TextureLoaderType>
    auto bindTextureLoader(sol::state& target,
                           TextureLoaderType& textureLoader) const -> void
    {
        defineTexture(target, textureLoader);
        defineSprite(target, textureLoader);
    }

  private:
    template<resource_managers::TextureLoader TextureLoaderType>
    auto defineTexture(sol::state& target,
                       TextureLoaderType& textureLoader) const -> void
    {
        auto textureType = target.new_usertype<sf::Texture>(
          "Texture",
          "get",
          sol::factories([&textureLoader](const std::string& path) {
              return textureLoader.load(path);
          }));
        textureType["size"] = sol::property(&sf::Texture::getSize);
    }
    template<resource_managers::TextureLoader TextureLoaderType>
    auto defineSprite(sol::state& target,
                      TextureLoaderType& textureLoader) const -> void
    {
        auto spriteType = target.new_usertype<drawing::actors::Sprite>(
          "Sprite",
          sol::factories(
            []() { return std::make_shared<drawing::actors::Sprite>(); },
            [](sf::Texture& texture) {
                return std::make_shared<drawing::actors::Sprite>(texture);
            },
            [&textureLoader](const std::string& path) {
                auto* texture = textureLoader.load(path);
                if (texture == nullptr) {
                    return std::shared_ptr<drawing::actors::Sprite>();
                }
                return std::make_shared<drawing::actors::Sprite>(*texture);
            }),
          sol::base_classes,
          sol::bases<drawing::actors::Actor,
                     drawing::actors::AbstractRectLeaf>());
        spriteType["texture"] =
          sol::property(&drawing::actors::Sprite::getTexture,
                        &drawing::actors::Sprite::setTexture);
        spriteType["textureRect"] =
          sol::property(&drawing::actors::Sprite::getTextureRect,
                        &drawing::actors::Sprite::setTextureRect);
        spriteType["color"] = sol::property(&drawing::actors::Sprite::getColor,
                                            &drawing::actors::Sprite::setColor);
    }
};
} // namespace lua
#endif // RHYTHMGAME_BOOTSTRAPPER_H

//
// Created by bobini on 10.08.2022.
//

#ifndef RHYTHMGAME_BOOTSTRAPPER_H
#define RHYTHMGAME_BOOTSTRAPPER_H

#include "lua/Lua.h"
#include <spdlog/spdlog.h>
#include <utility>
#include "resource_managers/TextureLoader.h"
#include "resource_managers/FontLoader.h"
#include "drawing/actors/Sprite.h"
#include "SFML/Graphics/Font.hpp"
#include "drawing/actors/Text.h"
#include "drawing/animations/Animation.h"
#include "drawing/animations/AnimationPlayer.h"
#include "EventAttacher.h"
#include "drawing/actors/AbstractVectorCollection.h"
#include "drawing/actors/AbstractBox.h"

namespace lua {

template<drawing::animations::AnimationPlayer AnimationPlayerType>
auto
bindAnimationPlayer(sol::state& target, AnimationPlayerType& animationsPlayer)
  -> void
{
    target["playAnimation"] =
      [&animationsPlayer](drawing::animations::Animation* animation) {
          animationsPlayer.playAnimation(animation->shared_from_this());
      };
    target["stopAnimation"] =
      [&animationsPlayer](drawing::animations::Animation* animation) {
          animationsPlayer.stopAnimation(animation->shared_from_this());
      };
    target["isPlaying"] =
      [&animationsPlayer](drawing::animations::Animation* animation) {
          return animationsPlayer.isPlaying(animation->shared_from_this());
      };
}

/**
 * @brief Define all actors that are not templated.
 * @param target The state to which the types should be added.
 */
auto
defineCommonTypes(
  sol::state& target,
  const EventAttacher& eventAttacher,
  const std::function<void(const std::shared_ptr<drawing::actors::Actor>&,
                           sol::table)>& bindActorProperties,
  const std::function<
    void(const std::shared_ptr<drawing::actors::AbstractRectLeaf>&,
         sol::table)>& bindAbstractRectLeafProperties) -> void;

template<resource_managers::FontLoader FontLoaderType>
auto
defineFont(sol::state& target, FontLoaderType& fontLoader) -> void
{
    auto fontType = target.new_usertype<sf::Font>(
      "Font", "get", sol::factories([&fontLoader](std::string filename) {
          return fontLoader.load(filename);
      }));
}

template<resource_managers::FontLoader FontLoaderType>
auto
defineText(sol::state& target,
           FontLoaderType& fontLoader,
           auto bindAbstractRectLeafProperties) -> void
{
    auto textType = target.new_usertype<drawing::actors::Text>(
      "Text",
      sol::factories(
        []() { return drawing::actors::Text::make(); },
        [&fontLoader](const std::string& text) {
            const auto* font = fontLoader.getDefault();
            if (font == nullptr) {
                throw std::runtime_error("Default font not found");
            }
            return drawing::actors::Text::make(text, *font);
        },
        [&fontLoader, bindAbstractRectLeafProperties](sol::table args) {
            const auto* font = args.get<const sf::Font*>(
              "font"); // get_or broken in this case as of 17.11.22.
                       // (https://github.com/ThePhD/sol2/issues/1421)
            if (font == nullptr) {
                const auto* defaultFont = fontLoader.getDefault();
                font = defaultFont;
            }
            if (font == nullptr) {
                throw std::runtime_error("Default font not found");
            }
            auto result = drawing::actors::Text::make(
              args.get_or<std::string>("text", ""),
              *font,
              args.get_or("characterSize",
                          drawing::actors::Text::defaultFontSize));
            bindAbstractRectLeafProperties(result, args);
            if (args["fillColor"].valid()) {
                result->setFillColor(args.get<sf::Color>("fillColor"));
            }
            if (args["outlineColor"].valid()) {
                result->setOutlineColor(args.get<sf::Color>("outlineColor"));
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
      sol::bases<drawing::actors::AbstractRectLeaf, drawing::actors::Actor>());
    textType["text"] = sol::property(&drawing::actors::Text::getText,
                                     &drawing::actors::Text::setText);
    textType["font"] = sol::property(&drawing::actors::Text::getFont,
                                     &drawing::actors::Text::setFont);
    textType["characterSize"] =
      sol::property(&drawing::actors::Text::getCharacterSize,
                    &drawing::actors::Text::setCharacterSize);
    textType["fillColor"] = sol::property(&drawing::actors::Text::getFillColor,
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
auto
defineTexture(sol::state& target, TextureLoaderType& textureLoader) -> void
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
auto
defineSprite(sol::state& target,
             const EventAttacher& eventAttacher,
             TextureLoaderType& textureLoader,
             auto bindAbstractRectLeafProperties) -> void
{
    auto spriteType = target.new_usertype<drawing::actors::Sprite>(
      "Sprite",
      sol::factories(
        [](sf::Texture& texture) {
            return drawing::actors::Sprite::make(texture);
        },
        [&textureLoader](
          const std::string& path) -> std::shared_ptr<drawing::actors::Sprite> {
            auto* texture = textureLoader.load(path);
            if (texture == nullptr) {
                texture = textureLoader.getFallback();
            }
            if (texture == nullptr) {
                spdlog::error("Fallback texture not found");
                return nullptr;
            }
            return drawing::actors::Sprite::make(*texture);
        },
        [&textureLoader, eventAttacher, bindAbstractRectLeafProperties](
          sol::table args) -> std::shared_ptr<drawing::actors::Sprite> {
            const auto* texture = static_cast<sf::Texture*>(nullptr);
            if (args["texture"].valid()) {
                if (args["texture"].get_type() == sol::type::string) {
                    texture =
                      textureLoader.load(args.get<std::string>("texture"));
                } else {
                    texture = args.get<const sf::Texture*>("texture");
                }
            }

            if (texture == nullptr) {
                texture = textureLoader.getFallback();
            }
            if (texture == nullptr) {
                spdlog::error("Default texture not found");
                return nullptr;
            }

            auto result = drawing::actors::Sprite::make(*texture);
            if (args["textureRect"].valid()) {
                result->setTextureRect(args.get<sf::IntRect>("textureRect"));
            }
            if (args["color"].valid()) {
                result->setColor(args.get<sf::Color>("color"));
            }
            bindAbstractRectLeafProperties(result, args);
            return result;
        }),
      sol::base_classes,
      sol::bases<drawing::actors::AbstractRectLeaf, drawing::actors::Actor>());
    spriteType["texture"] = sol::property(&drawing::actors::Sprite::getTexture,
                                          &drawing::actors::Sprite::setTexture);
    spriteType["textureRect"] =
      sol::property(&drawing::actors::Sprite::getTextureRect,
                    &drawing::actors::Sprite::setTextureRect);
    spriteType["color"] = sol::property(&drawing::actors::Sprite::getColor,
                                        &drawing::actors::Sprite::setColor);
}

inline auto
getBindActorProperties(const EventAttacher& eventAttacher)
{
    return [eventAttacher](const std::shared_ptr<drawing::actors::Actor>& actor,
                           sol::table args) {
        if (args["width"].valid()) {
            actor->setWidth(args["width"].get<float>());
        }
        if (args["height"].valid()) {
            actor->setHeight(args["height"].get<float>());
        }
        if (args["isObstructing"].valid()) {
            actor->setIsObstructing(args["isObstructing"].get<bool>());
        }
        eventAttacher.attachAllEvents(actor.get(), args);
    };
}

inline auto
getBindAbstractRectLeafProperties(auto bindActorProperties)
{
    return [bindActorProperties](
             const std::shared_ptr<drawing::actors::AbstractRectLeaf>& actor,
             sol::table args) {
        bindActorProperties(actor, args);
        if (args["minWidth"].valid()) {
            actor->setMinWidth(args["minWidth"].get<float>());
        }
        if (args["minHeight"].valid()) {
            actor->setMinHeight(args["minHeight"].get<float>());
        }
        if (args["isWidthManaged"].valid()) {
            actor->setIsWidthManaged(args["isWidthManaged"].get<bool>());
        }
        if (args["isHeightManaged"].valid()) {
            actor->setIsHeightManaged(args["isHeightManaged"].get<bool>());
        }
    };
}

template<resource_managers::TextureLoader TextureLoaderType,
         resource_managers::FontLoader FontLoaderType,
         drawing::animations::AnimationPlayer AnimationPlayerType>
auto
defineAllTypes(sol::state& target,
               TextureLoaderType& textureLoader,
               FontLoaderType& fontLoader,
               AnimationPlayerType& animationPlayer,
               const EventAttacher& eventAttacher) -> void
{
    auto bindActorProperties = getBindActorProperties(eventAttacher);
    auto bindAbstractRectLeafProperties =
      getBindAbstractRectLeafProperties(bindActorProperties);
    defineCommonTypes(target,
                      eventAttacher,
                      bindActorProperties,
                      bindAbstractRectLeafProperties);
    defineFont(target, fontLoader);
    defineText(target, fontLoader, bindAbstractRectLeafProperties);
    defineTexture(target, textureLoader);
    defineSprite(
      target, eventAttacher, textureLoader, bindAbstractRectLeafProperties);
    bindAnimationPlayer(target, animationPlayer);
}
} // namespace lua
#endif // RHYTHMGAME_BOOTSTRAPPER_H

//
// Created by bobini on 10.08.2022.
//

#ifndef RHYTHMGAME_BOOTSTRAPPER_H
#define RHYTHMGAME_BOOTSTRAPPER_H

#include <sol/state.hpp>
#include <spdlog/spdlog.h>
#include <utility>
#include "resource_managers/TextureLoader.h"
#include "resource_managers/FontLoader.h"
#include "drawing/actors/Sprite.h"
#include "SFML/Graphics/Font.hpp"
#include "drawing/actors/Text.h"
#include "events/Event.h"
#include "drawing/animations/Animation.h"
#include "drawing/animations/AnimationPlayer.h"

namespace lua {

class Bootstrapper
{
    auto defineActor(sol::state& target) const -> void;
    auto defineParent(sol::state& target) const -> void;
    auto defineAbstractVectorCollection(sol::state& target) const -> void;
    auto defineAbstractBox(sol::state& target) const -> void;
    auto defineVBox(sol::state& target) const -> void;
    auto defineHBox(sol::state& target) const -> void;
    auto defineVector2(sol::state& target) const -> void;
    auto defineAbstractRectLeaf(sol::state& target) const -> void;
    auto defineQuad(sol::state& target) const -> void;
    auto defineColor(sol::state& target) const -> void;
    auto definePadding(sol::state& target) const -> void;
    auto defineAlign(sol::state& target) const -> void;
    auto defineLayers(sol::state& target) const -> void;

    auto defineAnimation(sol::state& target) const -> void;
    auto defineLinear(sol::state& target) const -> void;

  public:
    using CppEventInterface =
      std::function<void(std::shared_ptr<drawing::actors::Actor>,
                         sol::function)>;

  private:
    std::shared_ptr<std::map<std::string, CppEventInterface>>
      eventRegistrators =
        std::make_shared<std::map<std::string, CppEventInterface>>();

  public:
    /**
     * @brief Registers an event in lua.
     * @tparam EventType The event's type.
     * @tparam Args The event's invocation arguments.
     * @param target The state to which the event should be added.
     * @param event The event to be added.
     * @param name The name of the event.
     */
    template<typename EventType, typename... Args>
        requires events::Event<EventType, std::function<void(Args...)>, Args...>
    auto addEvent(sol::state& target, EventType& event, std::string name)
      -> void
    {
        (*eventRegistrators)[name + "Event"] =
          [&target, &event, name](
            const std::shared_ptr<drawing::actors::Actor>& actor,
            sol::function function) {
              actor->setEventSubscription(
                name + "Event",
                event.subscribe(
                  [&target,
                   actorWeak = std::weak_ptr<drawing::actors::Actor>(actor),
                   function = std::move(function)](Args&&... args) {
                      auto actor = actorWeak.lock();
                      function(actor->getLuaSelf(target), args...);
                  }));
          };
    }

  private:
    class EventAttacher
    {
        std::shared_ptr<std::map<std::string, CppEventInterface>>
          eventRegistrators;

      public:
        explicit EventAttacher(
          std::shared_ptr<std::map<std::string, CppEventInterface>>
            eventRegistrators);
        auto attachEvent(std::string eventName,
                         std::shared_ptr<drawing::actors::Actor> actor,
                         sol::function function) const -> void;

        auto attachAllEvents(
          const std::shared_ptr<drawing::actors::Actor>& actor,
          const sol::table& events) const -> void;
    };

    /**
     * @brief Goes through all registered events and adds them to the actor type
     * as properties.
     * @param actorType The actor type object to which the properties should be
     * added
     */
    auto registerAllEventProperties(
      sol::usertype<drawing::actors::Actor> actorType) const
    {
        for (const auto& [name, registrator] : *eventRegistrators) {
            std::ignore = registrator;
            actorType[name] = sol::property(
              [nameCopy = name,
               eventAttacher = EventAttacher(eventRegistrators)](
                drawing::actors::Actor* actor, const sol::object& callback) {
                  if (callback.is<sol::function>()) {
                      eventAttacher.attachEvent(nameCopy,
                                                actor->shared_from_this(),
                                                callback.as<sol::function>());
                  } else if (callback.is<sol::lua_nil_t>()) {
                      actor->setEventSubscription(nameCopy, nullptr);
                  } else {
                      spdlog::error(
                        "Invalid type for {}. Function or nil expected",
                        nameCopy);
                  }
              });
        }
    }

  public:
    template<drawing::animations::AnimationPlayer AnimationPlayerType>
    auto bindAnimationPlayer(sol::state& target,
                             AnimationPlayerType& animationsPlayer) const
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
    }

    /**
     * @brief Define all actors that are not templated.
     * @param target The state to which the types should be added.
     */
    auto defineCommonTypes(sol::state& target) const -> void;

    /**
     * @brief Define the actors templated on the font loader type.
     * @tparam FontLoaderType The font loader type.
     * @param target The state to which the types should be added.
     * @param fontLoader The font loader to be used.
     */
    template<resource_managers::FontLoader FontLoaderType>
    auto bindFontLoader(sol::state& target, FontLoaderType& loader) const
      -> void
    {
        defineFont(target, loader);
        defineText(target, loader);
    }

  private:
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
            []() { return drawing::actors::Text::make(); },
            [&fontLoader](const std::string& text) {
                auto* font = fontLoader.getDefault();
                if (font == nullptr) {
                    throw std::runtime_error("Default font not found");
                }
                return drawing::actors::Text::make(text, *font);
            },
            [](const std::string& text, sf::Font& font) {
                return drawing::actors::Text::make(text, font);
            },
            [](const std::string& text,
               sf::Font& font,
               unsigned int characterSize) {
                return drawing::actors::Text::make(text, font, characterSize);
            },
            [&fontLoader, eventAttacher = EventAttacher(eventRegistrators)](
              sol::table args) {
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
                auto result = drawing::actors::Text::make(
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
                if (args["width"].valid()) {
                    result->setWidth(args.get<float>("width"));
                }
                if (args["height"].valid()) {
                    result->setHeight(args.get<float>("height"));
                }
                if (args["minWidth"].valid()) {
                    result->setMinWidth(args.get<float>("minWidth"));
                }
                if (args["minHeight"].valid()) {
                    result->setMinHeight(args.get<float>("minHeight"));
                }
                if (args["isWidthManaged"].valid()) {
                    result->setIsWidthManaged(args.get<bool>("isWidthManaged"));
                }
                if (args["isHeightManaged"].valid()) {
                    result->setIsHeightManaged(
                      args.get<bool>("isHeightManaged"));
                }
                if (args["events"].valid()) {
                    eventAttacher.attachAllEvents(result, args["events"]);
                }
                return result;
            }),
          sol::base_classes,
          sol::bases<drawing::actors::AbstractRectLeaf,
                     drawing::actors::Actor>());
        textType["text"] = sol::property(&drawing::actors::Text::getText,
                                         &drawing::actors::Text::setText);
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

  public:
    /**
     * @brief Defines the actors templated on the texture loader type.
     * @tparam TextureLoaderType The type of the texture loader.
     * @param target The target state to define the actor types in.
     * @param textureLoader The texture loader to use.
     */
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
            []() { return drawing::actors::Sprite::make(); },
            [](sf::Texture& texture) {
                return drawing::actors::Sprite::make(texture);
            },
            [&textureLoader](const std::string& path) {
                auto* texture = textureLoader.load(path);
                if (texture == nullptr) {
                    return std::shared_ptr<drawing::actors::Sprite>();
                }
                return drawing::actors::Sprite::make(*texture);
            },
            [&textureLoader, eventAttacher = EventAttacher(eventRegistrators)](
              sol::table args) {
                auto result = drawing::actors::Sprite::make();
                if (args["texture"].valid()) {
                    if (args["texture"].get_type() == sol::type::string) {
                        auto* texture =
                          textureLoader.load(args.get<std::string>("texture"));
                        if (texture != nullptr) {
                            result->setTexture(*texture);
                        }
                    } else {
                        result->setTexture(args.get<sf::Texture>("texture"));
                    }
                }
                if (args["textureRect"].valid()) {
                    result->setTextureRect(
                      args.get<sf::IntRect>("textureRect"));
                }
                if (args["color"].valid()) {
                    result->setColor(args.get<sf::Color>("color"));
                }
                if (args["width"].valid()) {
                    result->setWidth(args.get<float>("width"));
                }
                if (args["height"].valid()) {
                    result->setHeight(args.get<float>("height"));
                }
                if (args["minWidth"].valid()) {
                    result->setMinWidth(args.get<float>("minWidth"));
                }
                if (args["minHeight"].valid()) {
                    result->setMinHeight(args.get<float>("minHeight"));
                }
                if (args["isWidthManaged"].valid()) {
                    result->setIsWidthManaged(args.get<bool>("isWidthManaged"));
                }
                if (args["isHeightManaged"].valid()) {
                    result->setIsHeightManaged(
                      args.get<bool>("isHeightManaged"));
                }
                if (args["events"].valid()) {

                    eventAttacher.attachAllEvents(result, args["events"]);
                }
                return result;
            }),
          sol::base_classes,
          sol::bases<drawing::actors::AbstractRectLeaf,
                     drawing::actors::Actor>());
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

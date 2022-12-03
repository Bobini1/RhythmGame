//
// Created by bobini on 10.08.2022.
//

#include <SFML/Graphics/Font.hpp>
#include <spdlog/spdlog.h>
#include "Bootstrapper.h"
#include "drawing/actors/Actor.h"
#include "drawing/actors/Parent.h"
#include "drawing/actors/VBox.h"
#include "drawing/actors/HBox.h"
#include "drawing/actors/Quad.h"
#include "drawing/actors/Padding.h"
#include "drawing/actors/Align.h"
#include "drawing/actors/Layers.h"
#include "drawing/animations/Linear.h"

namespace lua {

auto
Bootstrapper::defineActor(sol::state& target) const -> void
{
    auto actorType =
      target.new_usertype<drawing::actors::Actor>("Actor", sol::no_constructor);
    actorType["getParent"] = [&target](drawing::actors::Actor& self) {
        return self.getParent()->getLuaSelf(target);
    };

    actorType["width"] = sol::property(&drawing::actors::Actor::getWidth,
                                       &drawing::actors::Actor::setWidth);
    actorType["height"] = sol::property(&drawing::actors::Actor::getHeight,
                                        &drawing::actors::Actor::setHeight);
    actorType["minWidth"] = sol::property(&drawing::actors::Actor::getMinWidth);
    actorType["minHeight"] =
      sol::property(&drawing::actors::Actor::getMinHeight);
    actorType["isWidthManaged"] =
      sol::property(&drawing::actors::Actor::getIsWidthManaged);
    actorType["isHeightManaged"] =
      sol::property(&drawing::actors::Actor::getIsHeightManaged);
    registerAllEventProperties(actorType);
}

auto
Bootstrapper::defineParent(sol::state& target) const -> void
{
    auto parentType = target.new_usertype<drawing::actors::Parent>(
      "Parent",
      sol::no_constructor,
      sol::base_classes,
      sol::bases<drawing::actors::Actor>());
    parentType["removeChild"] = &drawing::actors::Parent::removeChild;
}

auto
Bootstrapper::defineAbstractVectorCollection(sol::state& target) const -> void
{
    auto abstractVectorCollectionType =
      target.new_usertype<drawing::actors::AbstractVectorCollection>(
        "AbstractVectorCollection",
        sol::no_constructor,
        sol::base_classes,
        sol::bases<drawing::actors::Actor, drawing::actors::Parent>());
    abstractVectorCollectionType["addChild"] =
      [](drawing::actors::AbstractVectorCollection* self,
         drawing::actors::Actor* child) {
          self->addChild(child->shared_from_this());
      };
    abstractVectorCollectionType["getChild"] =
      [&target](drawing::actors::AbstractVectorCollection* self, int index) {
          return (*self)[static_cast<std::size_t>(index) - 1]->getLuaSelf(
            target);
      };
    abstractVectorCollectionType["size"] =
      sol::property(&drawing::actors::AbstractVectorCollection::getSize);
}

auto
Bootstrapper::defineAbstractBox(sol::state& target) const -> void
{
    auto abstractBoxType = target.new_usertype<drawing::actors::AbstractBox>(
      "Box",
      sol::no_constructor,
      sol::base_classes,
      sol::bases<drawing::actors::Actor, drawing::actors::Parent>());
    abstractBoxType["horizontalSizeMode"] =
      sol::property(&drawing::actors::AbstractBox::getHorizontalSizeMode,
                    &drawing::actors::AbstractBox::setHorizontalSizeMode);
    abstractBoxType["verticalSizeMode"] =
      sol::property(&drawing::actors::AbstractBox::getVerticalSizeMode,
                    &drawing::actors::AbstractBox::setVerticalSizeMode);
    abstractBoxType["spacing"] =
      sol::property(&drawing::actors::AbstractBox::getSpacing,
                    &drawing::actors::AbstractBox::setSpacing);

    auto sizeModeType = target.new_enum<drawing::actors::AbstractBox::SizeMode>(
      "SizeMode",
      { { "Fixed", drawing::actors::AbstractBox::SizeMode::Fixed },
        { "Managed", drawing::actors::AbstractBox::SizeMode::Managed },
        { "WrapChildren",
          drawing::actors::AbstractBox::SizeMode::WrapChildren } });
}

auto
Bootstrapper::defineVBox(sol::state& target) const -> void
{
    auto vBoxType = target.new_usertype<drawing::actors::VBox>(
      "VBox",
      sol::factories(
        []() { return drawing::actors::VBox::make(); },
        [eventAttacher = EventAttacher(eventRegistrators)](sol::table args) {
            auto result = drawing::actors::VBox::make();
            if (args["children"].valid()) {
                auto children =
                  args["children"].get<std::vector<drawing::actors::Actor*>>();
                for (auto* child : children) {
                    result->addChild(child->shared_from_this());
                }
            }
            if (args["contentAlignment"].valid()) {
                result->setContentAlignment(
                  args["contentAlignment"]
                    .get<drawing::actors::VBox::ContentAlignment>());
            }
            if (args["horizontalSizeMode"].valid()) {
                result->setHorizontalSizeMode(
                  args["horizontalSizeMode"]
                    .get<drawing::actors::AbstractBox::SizeMode>());
            }
            if (args["verticalSizeMode"].valid()) {
                result->setVerticalSizeMode(
                  args["verticalSizeMode"]
                    .get<drawing::actors::AbstractBox::SizeMode>());
            }
            if (args["width"].valid()) {
                result->setWidth(args["width"].get<float>());
            }
            if (args["height"].valid()) {
                result->setHeight(args["height"].get<float>());
            }
            if (args["spacing"].valid()) {
                result->setSpacing(args["spacing"].get<float>());
            }
            if (args["events"].valid()) {
                eventAttacher.attachAllEvents(result, args["events"]);
            }
            return result;
        }),
      sol::base_classes,
      sol::bases<drawing::actors::Actor,
                 drawing::actors::Parent,
                 drawing::actors::AbstractVectorCollection,
                 drawing::actors::AbstractBox>());

    vBoxType["contentAlignment"] =
      sol::property(&drawing::actors::VBox::getContentAlignment,
                    &drawing::actors::VBox::setContentAlignment);

    auto alignType = target.new_enum<drawing::actors::VBox::ContentAlignment>(
      "VBoxContentAlignment",
      { { "Left", drawing::actors::VBox::ContentAlignment::Left },
        { "Center", drawing::actors::VBox::ContentAlignment::Center },
        { "Right", drawing::actors::VBox::ContentAlignment::Right } });
}

auto
Bootstrapper::defineHBox(sol::state& target) const -> void
{
    auto hBoxType = target.new_usertype<drawing::actors::HBox>(
      "HBox",
      sol::factories(
        []() { return drawing::actors::HBox::make(); },
        [eventAttacher = EventAttacher(eventRegistrators)](sol::table args) {
            auto result = drawing::actors::HBox::make();
            if (args["children"].valid()) {
                auto children =
                  args["children"].get<std::vector<drawing::actors::Actor*>>();
                for (auto* child : children) {
                    result->addChild(child->shared_from_this());
                }
            }
            if (args["contentAlignment"].valid()) {
                result->setContentAlignment(
                  args["contentAlignment"]
                    .get<drawing::actors::HBox::ContentAlignment>());
            }
            if (args["horizontalSizeMode"].valid()) {
                result->setHorizontalSizeMode(
                  args["horizontalSizeMode"]
                    .get<drawing::actors::AbstractBox::SizeMode>());
            }
            if (args["verticalSizeMode"].valid()) {
                result->setVerticalSizeMode(
                  args["verticalSizeMode"]
                    .get<drawing::actors::AbstractBox::SizeMode>());
            }
            if (args["width"].valid()) {
                result->setWidth(args["width"].get<float>());
            }
            if (args["height"].valid()) {
                result->setHeight(args["height"].get<float>());
            }
            if (args["spacing"].valid()) {
                result->setSpacing(args["spacing"].get<float>());
            }
            if (args["events"].valid()) {
                eventAttacher.attachAllEvents(result, args["events"]);
            }
            return result;
        }),
      sol::base_classes,
      sol::bases<drawing::actors::Actor,
                 drawing::actors::Parent,
                 drawing::actors::AbstractVectorCollection,
                 drawing::actors::AbstractBox>());
    hBoxType["contentAlignment"] =
      sol::property(&drawing::actors::HBox::getContentAlignment,
                    &drawing::actors::HBox::setContentAlignment);

    auto alignType = target.new_enum<drawing::actors::HBox::ContentAlignment>(
      "HBoxContentAlignment",
      { { "Top", drawing::actors::HBox::ContentAlignment::Top },
        { "Center", drawing::actors::HBox::ContentAlignment::Center },
        { "Bottom", drawing::actors::HBox::ContentAlignment::Bottom } });
}

auto
Bootstrapper::defineVector2(sol::state& target) const -> void
{
    auto vector2fType = target.new_usertype<sf::Vector2f>(
      "Vector2",
      sol::constructors<sf::Vector2f(), sf::Vector2f(float, float)>());
    vector2fType["x"] = sol::property(&sf::Vector2f::x, &sf::Vector2f::x);
    vector2fType["y"] = sol::property(&sf::Vector2f::y, &sf::Vector2f::y);
}

auto
Bootstrapper::defineAbstractRectLeaf(sol::state& target) const -> void
{
    auto abstractRectLeafType =
      target.new_usertype<drawing::actors::AbstractRectLeaf>(
        "AbstractRectLeaf",
        sol::no_constructor,
        sol::base_classes,
        sol::bases<drawing::actors::Actor>());
    abstractRectLeafType["minWidth"] =
      sol::property(&drawing::actors::AbstractRectLeaf::getMinWidth,
                    &drawing::actors::AbstractRectLeaf::setMinWidth);
    abstractRectLeafType["minHeight"] =
      sol::property(&drawing::actors::AbstractRectLeaf::getMinHeight,
                    &drawing::actors::AbstractRectLeaf::setMinHeight);
    abstractRectLeafType["isWidthManaged"] =
      sol::property(&drawing::actors::AbstractRectLeaf::getIsWidthManaged,
                    &drawing::actors::AbstractRectLeaf::setIsWidthManaged);
    abstractRectLeafType["isHeightManaged"] =
      sol::property(&drawing::actors::AbstractRectLeaf::getIsHeightManaged,
                    &drawing::actors::AbstractRectLeaf::setIsHeightManaged);
}

auto
Bootstrapper::defineQuad(sol::state& target) const -> void
{
    auto quadType = target.new_usertype<drawing::actors::Quad>(
      "Quad",
      sol::factories(
        []() { return drawing::actors::Quad::make(); },
        [](float width, float height) {
            auto result =
              drawing::actors::Quad::make(sf::Vector2f{ width, height });
            return result;
        },
        [](float width, float height, sf::Color color) {
            auto result =
              drawing::actors::Quad::make(sf::Vector2f{ width, height }, color);
            return result;
        },
        [eventAttacher = EventAttacher(eventRegistrators)](sol::table args) {
            auto result = drawing::actors::Quad::make(
              sf::Vector2f{ args.get_or("width", 0.F),
                            args.get_or("height", 0.F) },
              args.get_or<sf::Color>("fillColor", sf::Color::White));
            if (args["outlineColor"].valid()) {
                result->setOutlineColor(args.get<sf::Color>("outlineColor"));
            }
            if (args["outlineThickness"].valid()) {
                result->setOutlineThickness(
                  args.get<float>("outlineThickness"));
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
                result->setIsHeightManaged(args.get<bool>("isHeightManaged"));
            }
            if (args["events"].valid()) {
                eventAttacher.attachAllEvents(result, args["events"]);
            }
            return result;
        }),
      sol::base_classes,
      sol::bases<drawing::actors::AbstractRectLeaf, drawing::actors::Actor>());
    quadType["fillColor"] = sol::property(&drawing::actors::Quad::getFillColor,
                                          &drawing::actors::Quad::setFillColor);
    quadType["outlineColor"] =
      sol::property(&drawing::actors::Quad::getOutlineColor,
                    &drawing::actors::Quad::setOutlineColor);
    quadType["outlineThickness"] =
      sol::property(&drawing::actors::Quad::getOutlineThickness,
                    &drawing::actors::Quad::setOutlineThickness);
    quadType["getPoint"] = &drawing::actors::Quad::getPoint;
}

auto
Bootstrapper::defineColor(sol::state& target) const -> void
{
    auto colorType = target.new_usertype<sf::Color>(
      "Color", sol::constructors<sf::Color(), sf::Color(int, int, int, int)>());
    colorType["r"] = sol::property(&sf::Color::r, &sf::Color::r);
    colorType["g"] = sol::property(&sf::Color::g, &sf::Color::g);
    colorType["b"] = sol::property(&sf::Color::b, &sf::Color::b);
    colorType["a"] = sol::property(&sf::Color::a, &sf::Color::a);
}

auto
Bootstrapper::definePadding(sol::state& target) const -> void
{
    auto paddingType = target.new_usertype<drawing::actors::Padding>(
      "Padding",
      sol::factories(
        []() { return drawing::actors::Padding::make(); },
        [](drawing::actors::Actor* actor) {
            auto returnVal = drawing::actors::Padding::make();
            returnVal->setChild(actor->shared_from_this());
            return returnVal;
        },
        [](drawing::actors::Actor* actor, const sol::table& args) {
            auto returnVal =
              drawing::actors::Padding::make(args.get_or("top", 0.F),
                                             args.get_or("bottom", 0.F),
                                             args.get_or("left", 0.F),
                                             args.get_or("right", 0.F));
            returnVal->setChild(actor->shared_from_this());
            return returnVal;
        },
        [eventAttacher =
           EventAttacher(eventRegistrators)](const sol::table& args) {
            auto child = [&]() {
                if (args["child"].valid()) {
                    return args.get<drawing::actors::Actor*>("child")
                      ->shared_from_this();
                }
                return std::shared_ptr<drawing::actors::Actor>();
            }();
            auto returnVal =
              drawing::actors::Padding::make(args.get_or("top", 0.F),
                                             args.get_or("bottom", 0.F),
                                             args.get_or("left", 0.F),
                                             args.get_or("right", 0.F));
            returnVal->setChild(child);
            if (args["events"].valid()) {
                eventAttacher.attachAllEvents(returnVal, args["events"]);
            }
            return returnVal;
        }),
      sol::base_classes,
      sol::bases<drawing::actors::Actor, drawing::actors::Parent>());
    paddingType["top"] = sol::property(&drawing::actors::Padding::getTop,
                                       &drawing::actors::Padding::setTop);
    paddingType["right"] = sol::property(&drawing::actors::Padding::getRight,
                                         &drawing::actors::Padding::setRight);
    paddingType["bottom"] = sol::property(&drawing::actors::Padding::getBottom,
                                          &drawing::actors::Padding::setBottom);
    paddingType["left"] = sol::property(&drawing::actors::Padding::getLeft,
                                        &drawing::actors::Padding::setLeft);
    paddingType["child"] = sol::property(
      [&target](drawing::actors::Padding* self) {
          return self->getChild()->getLuaSelf(target);
      },
      [](drawing::actors::Padding* self, drawing::actors::Actor* actor) {
          self->setChild(actor->shared_from_this());
      });
}

auto
Bootstrapper::defineAlign(sol::state& target) const -> void
{
    auto modeType = target.new_enum("AlignMode",
                                    "TopLeft",
                                    drawing::actors::Align::Mode::TopLeft,
                                    "Top",
                                    drawing::actors::Align::Mode::Top,
                                    "TopRight",
                                    drawing::actors::Align::Mode::TopRight,
                                    "Left",
                                    drawing::actors::Align::Mode::Left,
                                    "Center",
                                    drawing::actors::Align::Mode::Center,
                                    "Right",
                                    drawing::actors::Align::Mode::Right,
                                    "BottomLeft",
                                    drawing::actors::Align::Mode::BottomLeft,
                                    "Bottom",
                                    drawing::actors::Align::Mode::Bottom,
                                    "BottomRight",
                                    drawing::actors::Align::Mode::BottomRight);
    auto alignType = target.new_usertype<drawing::actors::Align>(
      "Align",
      sol::factories(
        []() { return drawing::actors::Align::make(); },
        [](drawing::actors::Actor* actor) {
            auto returnVal = drawing::actors::Align::make();
            returnVal->setChild(actor->shared_from_this());
            return returnVal;
        },
        [](drawing::actors::Actor* actor, drawing::actors::Align::Mode mode) {
            auto returnVal = drawing::actors::Align::make(mode);
            returnVal->setChild(actor->shared_from_this());
            return returnVal;
        },
        [](drawing::actors::Align::Mode mode) {
            return drawing::actors::Align::make(mode);
        },
        [eventAttacher =
           EventAttacher(eventRegistrators)](const sol::table& args) {
            auto child = [&]() {
                if (args["child"].valid()) {
                    return args.get<drawing::actors::Actor*>("child")
                      ->shared_from_this();
                }
                return std::shared_ptr<drawing::actors::Actor>();
            }();
            auto returnVal = drawing::actors::Align::make(
              args.get_or("mode", drawing::actors::Align::Mode::Center));
            returnVal->setChild(std::move(child));

            if (args["events"].valid()) {
                eventAttacher.attachAllEvents(returnVal, args["events"]);
            }
            return returnVal;
        }),
      sol::base_classes,
      sol::bases<drawing::actors::Actor, drawing::actors::Parent>());
    alignType["child"] = sol::property(
      [&target](drawing::actors::Align* self) {
          return self->getChild()->getLuaSelf(target);
      },
      [](drawing::actors::Align* self, drawing::actors::Actor* actor) {
          self->setChild(actor->shared_from_this());
      });
    alignType["mode"] = sol::property(&drawing::actors::Align::getMode,
                                      &drawing::actors::Align::setMode);
}

auto
Bootstrapper::defineLayers(sol::state& target) const -> void
{
    auto layerType = target.new_usertype<drawing::actors::Layers>(
      "Layers",
      sol::factories(
        []() { return drawing::actors::Layers::make(); },
        [eventAttacher =
           EventAttacher(eventRegistrators)](const sol::table& args) {
            auto returnVal = drawing::actors::Layers::make();
            if (args["children"].valid()) {
                for (const auto& child :
                     args.get<std::vector<drawing::actors::Actor*>>(
                       "children")) {
                    returnVal->addChild(child->shared_from_this());
                }
            }
            if (args["mainLayer"].valid()) {
                returnVal->setMainLayer(
                  args.get<drawing::actors::Actor*>("mainLayer")
                    ->shared_from_this());
            }
            if (args["width"].valid()) {
                returnVal->setWidth(args.get<float>("width"));
            }
            if (args["height"].valid()) {
                returnVal->setHeight(args.get<float>("height"));
            }
            if (args["events"].valid()) {
                eventAttacher.attachAllEvents(returnVal, args["events"]);
            }
            return returnVal;
        }),
      sol::base_classes,
      sol::bases<drawing::actors::Actor, drawing::actors::Parent>());
    layerType["mainLayer"] = sol::property(
      [&target](drawing::actors::Layers* self) {
          return self->getMainLayer()->getLuaSelf(target);
      },
      [](drawing::actors::Layers* self, drawing::actors::Actor* actor) {
          self->setMainLayer(actor->shared_from_this());
      });
}

auto
Bootstrapper::defineAnimation(sol::state& target) const -> void
{
    auto animationType = target.new_usertype<drawing::animations::Animation>(
      "Animation", sol::no_constructor);
    animationType["reset"] = &drawing::animations::Animation::reset;
    animationType["isLooping"] =
      sol::property(&drawing::animations::Animation::getIsLooping,
                    &drawing::animations::Animation::setIsLooping);
    animationType["duration"] = sol::property(
      [](drawing::animations::Animation* self) {
          return static_cast<double>(self->getDuration().count()) * 1E-9;
      },
      [](drawing::animations::Animation* self, float duration) {
          self->setDuration(
            std::chrono::nanoseconds{ static_cast<int64_t>(duration * 1E9) });
      });
    animationType["getProgress"] =
      sol::property(&drawing::animations::Animation::getProgress,
                    &drawing::animations::Animation::setProgress);
    animationType["onFinished"] = sol::property(
      [&target](drawing::animations::Animation* self, sol::function callback) {
          self->setOnFinished(
            [callback, &target](std::shared_ptr<drawing::actors::Actor> actor) {
                callback(actor->getLuaSelf(target));
            });
      });
    animationType["isFinished"] =
      sol::property(&drawing::animations::Animation::getIsFinished,
                    [](drawing::animations::Animation* self, bool isFinished) {
                        if (isFinished) {
                            self->setProgress(1.0F);
                        } else {
                            self->setProgress(0.0F);
                        }
                    });
    animationType["actor"] = sol::property(
      [&target](drawing::animations::Animation* self) {
          return self->getActor()->getLuaSelf(target);
      },
      [](drawing::animations::Animation* self, drawing::actors::Actor* actor) {
          self->setActor(actor->weak_from_this());
      });
}
auto
Bootstrapper::defineLinear(sol::state& target) const -> void
{
    auto linearType = target.new_usertype<drawing::animations::Linear>(
      "Linear",
      sol::factories([&target](drawing::actors::Actor* actor,
                               sol::function updater,
                               float seconds,
                               float start,
                               float end) {
          constexpr auto secondsToNanos = 1E9;
          auto time = std::chrono::nanoseconds(
            static_cast<int64_t>(seconds * secondsToNanos));
          auto function = updater.as<std::function<void(sol::object, float)>>();
          auto wrappedFunction =
            [function, &target](std::shared_ptr<drawing::actors::Actor> actor,
                                float value) {
                function(actor->getLuaSelf(target), value);
            };
          return std::make_shared<drawing::animations::Linear>(
            actor->weak_from_this(),
            std::move(wrappedFunction),
            time,
            start,
            end);
      }),
      sol::base_classes,
      sol::bases<drawing::animations::Animation>());
}

auto
Bootstrapper::defineCommonTypes(sol::state& target) const -> void
{
    defineActor(target);
    defineParent(target);
    defineAbstractVectorCollection(target);
    defineAbstractBox(target);
    defineVBox(target);
    defineHBox(target);
    defineVector2(target);
    defineAbstractRectLeaf(target);
    defineQuad(target);
    defineColor(target);
    definePadding(target);
    defineAlign(target);
    defineLayers(target);
    defineAnimation(target);
    defineLinear(target);
}
auto
Bootstrapper::EventAttacher::attachAllEvents(
  const std::shared_ptr<drawing::actors::Actor>& actor,
  const sol::table& events) const -> void
{
    for (auto& [key, value] : events) {
        if (value.get_type() != sol::type::function) {
            spdlog::error("Event handler {} is not a function, skipping",
                          key.as<std::string>());
            continue;
        }
        attachEvent(key.as<std::string>(), actor, value.as<sol::function>());
    }
}
auto
Bootstrapper::EventAttacher::attachEvent(
  std::string eventName,
  std::shared_ptr<drawing::actors::Actor> actor,
  sol::function function) const -> void
{
    if (eventRegistrators->find(eventName) == eventRegistrators->end()) {
        spdlog::error("Event {} not found", eventName);
        return;
    }
    eventRegistrators->at(eventName)(std::move(actor), std::move(function));
}
Bootstrapper::EventAttacher::EventAttacher(
  std::shared_ptr<std::map<std::string, CppEventInterface>> eventRegistrators)
  : eventRegistrators(std::move(eventRegistrators))
{
}

} // namespace lua
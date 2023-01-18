//
// Created by bobini on 10.08.2022.
//

#include "Bootstrapper.h"
#include <SFML/Graphics/Font.hpp>
#include "drawing/actors/Actor.h"
#include "drawing/actors/Parent.h"
#include "drawing/actors/VBox.h"
#include "drawing/actors/HBox.h"
#include "drawing/actors/Quad.h"
#include "drawing/actors/Padding.h"
#include "drawing/actors/Align.h"
#include "drawing/actors/Layers.h"
#include "drawing/animations/Linear.h"
#include "drawing/animations/AnimationSequence.h"

namespace lua {

auto
defineActor(sol::state& target, const EventAttacher& eventAttacher) -> void
{
    auto actorType =
      target.new_usertype<drawing::actors::Actor>("Actor", sol::no_constructor);

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
    actorType["isObstructing"] =
      sol::property(&drawing::actors::Actor::getIsObstructing,
                    &drawing::actors::Actor::setIsObstructing);
    actorType["parent"] =
      sol::property([&target](drawing::actors::Actor& self) {
          return self.getParent()->getLuaSelf(target);
      });
    eventAttacher.registerAllEventProperties(actorType);
}

auto
defineParent(sol::state& target) -> void
{
    auto parentType = target.new_usertype<drawing::actors::Parent>(
      "Parent",
      sol::no_constructor,
      sol::base_classes,
      sol::bases<drawing::actors::Actor>());
    parentType["removeChild"] = &drawing::actors::Parent::removeChild;
}

auto
defineAbstractVectorCollection(sol::state& target) -> void
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
      [&target](drawing::actors::AbstractVectorCollection* self,
                int index) -> sol::object {
        auto child = (*self)[static_cast<std::size_t>(index) - 1];
        if (child) {
            return child->getLuaSelf(target);
        }
        return sol::lua_nil;
    };
    abstractVectorCollectionType["size"] =
      sol::property(&drawing::actors::AbstractVectorCollection::getSize);
}

auto
defineAbstractBox(sol::state& target) -> void
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
defineVBox(sol::state& target, auto bindAbstractBoxProperties) -> void
{
    auto vBoxType = target.new_usertype<drawing::actors::VBox>(
      "VBox",
      sol::factories(
        []() { return drawing::actors::VBox::make(); },
        [bindAbstractBoxProperties](sol::table args) {
            auto result = drawing::actors::VBox::make();
            bindAbstractBoxProperties(result, args);
            if (args["contentAlignment"].valid()) {
                result->setContentAlignment(
                  args["contentAlignment"]
                    .get<drawing::actors::VBox::ContentAlignment>());
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
defineHBox(sol::state& target, auto bindAbstractBoxProperties) -> void
{
    auto hBoxType = target.new_usertype<drawing::actors::HBox>(
      "HBox",
      sol::factories(
        []() { return drawing::actors::HBox::make(); },
        [bindAbstractBoxProperties](sol::table args) {
            auto result = drawing::actors::HBox::make();
            bindAbstractBoxProperties(result, args);
            if (args["contentAlignment"].valid()) {
                result->setContentAlignment(
                  args["contentAlignment"]
                    .get<drawing::actors::HBox::ContentAlignment>());
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
defineVector2(sol::state& target) -> void
{
    auto vector2fType = target.new_usertype<sf::Vector2f>(
      "Vector2",
      sol::constructors<sf::Vector2f(),
                        sf::Vector2f(float, float),
                        sf::Vector2f(const sf::Vector2f&)>());
    vector2fType["x"] = sol::property(&sf::Vector2f::x, &sf::Vector2f::x);
    vector2fType["y"] = sol::property(&sf::Vector2f::y, &sf::Vector2f::y);
    vector2fType[1] = sol::property(&sf::Vector2f::x, &sf::Vector2f::x);
    vector2fType[2] = sol::property(&sf::Vector2f::y, &sf::Vector2f::y);
}

auto
defineAbstractRectLeaf(sol::state& target) -> void
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
defineQuad(sol::state& target, auto bindAbstractRectLeafProperties) -> void
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
        [bindAbstractRectLeafProperties](sol::table args) {
            auto result = drawing::actors::Quad::make(
              sf::Vector2f{ 0, 0 },
              args.get_or<sf::Color>("fillColor", sf::Color::White));
            if (args["outlineColor"].valid()) {
                result->setOutlineColor(args.get<sf::Color>("outlineColor"));
            }
            if (args["outlineThickness"].valid()) {
                result->setOutlineThickness(
                  args.get<float>("outlineThickness"));
            }
            bindAbstractRectLeafProperties(result, args);
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
    quadType["getPoint"] = [](drawing::actors::Quad& self,
                              int index) -> sf::Vector2f {
        if (index < 1 || index > 4) {
            spdlog::error(
              "Quad.getPoint index must be between 1 and 4, was: {}", index);
            return {};
        }
        return self.getPoint(static_cast<size_t>(index - 1));
    };
}

auto
defineColor(sol::state& target) -> void
{
    auto colorType = target.new_usertype<sf::Color>(
      "Color",
      sol::constructors<sf::Color(),
                        sf::Color(sf::Uint8, sf::Uint8, sf::Uint8, sf::Uint8),
                        sf::Color(const sf::Color&)>());
    colorType["r"] = sol::property(&sf::Color::r, &sf::Color::r);
    colorType["g"] = sol::property(&sf::Color::g, &sf::Color::g);
    colorType["b"] = sol::property(&sf::Color::b, &sf::Color::b);
    colorType["a"] = sol::property(&sf::Color::a, &sf::Color::a);
    colorType[1] = sol::property(&sf::Color::r, &sf::Color::r);
    colorType[2] = sol::property(&sf::Color::g, &sf::Color::g);
    colorType[3] = sol::property(&sf::Color::b, &sf::Color::b);
    colorType[4] = sol::property(&sf::Color::a, &sf::Color::a);
}

auto
definePadding(sol::state& target, auto bindActorProperties) -> void
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
        [bindActorProperties](const sol::table& args) {
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
            bindActorProperties(returnVal, args);
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
defineAlign(sol::state& target, auto bindActorProperties) -> void
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
        [bindActorProperties](const sol::table& args) {
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
            bindActorProperties(returnVal, args);
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
defineLayers(sol::state& target, auto bindAbstractVectorCollectionProperties)
  -> void
{
    auto layerType = target.new_usertype<drawing::actors::Layers>(
      "Layers",
      sol::factories(
        []() { return drawing::actors::Layers::make(); },
        [bindAbstractVectorCollectionProperties](const sol::table& args) {
            auto returnVal = drawing::actors::Layers::make();
            bindAbstractVectorCollectionProperties(returnVal, args);
            if (args["mainLayer"].valid()) {
                returnVal->setMainLayer(
                  args.get<drawing::actors::Actor*>("mainLayer")
                    ->shared_from_this());
            }
            return returnVal;
        }),
      sol::base_classes,
      sol::bases<drawing::actors::Actor,
                 drawing::actors::Parent,
                 drawing::actors::AbstractVectorCollection>());
    layerType["mainLayer"] = sol::property(
      [&target](drawing::actors::Layers* self) {
          return self->getMainLayer()->getLuaSelf(target);
      },
      [](drawing::actors::Layers* self, drawing::actors::Actor* actor) {
          self->setMainLayer(actor->shared_from_this());
      });
}

auto
defineAnimation(sol::state& target) -> void
{
    auto animationType = target.new_usertype<drawing::animations::Animation>(
      "Animation", sol::no_constructor);
    animationType["reset"] = &drawing::animations::Animation::reset;
    animationType["isLooping"] =
      sol::property(&drawing::animations::Animation::getIsLooping,
                    &drawing::animations::Animation::setIsLooping);
    animationType["duration"] =
      sol::property(&drawing::animations::Animation::getDuration,
                    &drawing::animations::Animation::setDuration);
    animationType["progress"] =
      sol::property(&drawing::animations::Animation::getProgress,
                    &drawing::animations::Animation::setProgress);
    animationType["onFinished"] =
      sol::property(&drawing::animations::Animation::getOnFinished,
                    &drawing::animations::Animation::setOnFinished);
    animationType["isFinished"] =
      sol::property(&drawing::animations::Animation::getIsFinished);
}
auto
defineLinear(sol::state& target, auto bindAnimationProperties) -> void
{
    auto linearType = target.new_usertype<drawing::animations::Linear>(
      "Linear",
      sol::factories(
        [](std::function<void(float)> function,
           std::chrono::nanoseconds time,
           float start,
           float end) {
            return drawing::animations::Linear::make(
              std::move(function), time, start, end);
        },
        [bindAnimationProperties](sol::table args) {
            auto function = args.get_or(
              "function", std::function<void(float)>{ [](float) {} });
            auto time = args.get_or("duration", std::chrono::nanoseconds{});
            auto start = args.get_or("from", 0.F);
            auto end = args.get_or("to", 0.F);
            auto result = drawing::animations::Linear::make(
              std::move(function), time, start, end);
            bindAnimationProperties(result, args);
            return result;
        }),
      sol::base_classes,
      sol::bases<drawing::animations::Animation>());
    linearType["from"] = sol::property(&drawing::animations::Linear::getFrom,
                                       &drawing::animations::Linear::setFrom);
    linearType["to"] = sol::property(&drawing::animations::Linear::getTo,
                                     &drawing::animations::Linear::setTo);
    linearType["function"] =
      sol::property(&drawing::animations::Linear::getFunction,
                    &drawing::animations::Linear::setFunction);
}

auto
defineAnimationSequence(sol::state& target, auto bindAnimationProperties)
  -> void
{
    auto animationSequenceType =
      target.new_usertype<drawing::animations::AnimationSequence>(
        "AnimationSequence",
        sol::factories([bindAnimationProperties](const sol::table& args) {
            auto animations = args.get_or(
              "animations", std::vector<drawing::animations::Animation*>{});
            auto animationsShared =
              std::vector<std::shared_ptr<drawing::animations::Animation>>{};
            animationsShared.reserve(animations.size());
            for (auto* animation : animations) {
                animationsShared.push_back(animation->shared_from_this());
            }
            auto returnVal = drawing::animations::AnimationSequence::make(
              std::move(animationsShared));
            bindAnimationProperties(returnVal, args);
            return returnVal;
        }),
        sol::base_classes,
        sol::bases<drawing::animations::Animation>());
}
auto
defineCommonTypes(
  sol::state& target,
  const EventAttacher& eventAttacher,
  const std::function<void(const std::shared_ptr<drawing::actors::Actor>&,
                           sol::table)>& bindActorProperties,
  const std::function<
    void(const std::shared_ptr<drawing::actors::AbstractRectLeaf>&,
         sol::table)>& bindAbstractRectLeafProperties) -> void
{

    auto bindAbstractVectorCollectionProperties =
      [bindActorProperties](
        const std::shared_ptr<drawing::actors::AbstractVectorCollection>& actor,
        sol::table args) {
          bindActorProperties(actor, args);
          if (args["children"].valid()) {
              auto children =
                args["children"].get<std::vector<drawing::actors::Actor*>>();
              for (auto* child : children) {
                  actor->addChild(child->shared_from_this());
              }
          }
      };
    auto bindAbstractBoxProperties =
      [bindAbstractVectorCollectionProperties](
        const std::shared_ptr<drawing::actors::AbstractBox>& actor,
        sol::table args) {
          bindAbstractVectorCollectionProperties(actor, args);
          if (args["horizontalSizeMode"].valid()) {
              actor->setHorizontalSizeMode(
                args["horizontalSizeMode"]
                  .get<drawing::actors::AbstractBox::SizeMode>());
          }
          if (args["verticalSizeMode"].valid()) {
              actor->setVerticalSizeMode(
                args["verticalSizeMode"]
                  .get<drawing::actors::AbstractBox::SizeMode>());
          }
          if (args["width"].valid()) {
              actor->setWidth(args["width"].get<float>());
          }
          if (args["height"].valid()) {
              actor->setHeight(args["height"].get<float>());
          }
          if (args["spacing"].valid()) {
              actor->setSpacing(args["spacing"].get<float>());
          }
      };
    defineActor(target, eventAttacher);
    defineParent(target);
    defineAbstractVectorCollection(target);
    defineAbstractBox(target);
    defineVBox(target, bindAbstractBoxProperties);
    defineHBox(target, bindAbstractBoxProperties);
    defineVector2(target);
    defineAbstractRectLeaf(target);
    defineQuad(target, bindAbstractRectLeafProperties);
    defineColor(target);
    definePadding(target, bindActorProperties);
    defineAlign(target, bindActorProperties);
    defineLayers(target, bindAbstractVectorCollectionProperties);
    defineAnimation(target);

    auto bindAnimationProperties =
      [](const std::shared_ptr<drawing::animations::Animation>& animation,
         sol::table args) {
          if (args["onFinished"].valid()) {
              animation->setOnFinished(
                args["onFinished"].get<std::function<void()>>());
          }
          if (args["getIsLooping"].valid()) {
              animation->setIsLooping(args["getIsLooping"]);
          }
      };

    defineLinear(target, bindAnimationProperties);
    defineAnimationSequence(target, bindAnimationProperties);
}

} // namespace lua
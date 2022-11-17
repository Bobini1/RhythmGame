//
// Created by bobini on 10.08.2022.
//

#include <SFML/Graphics/Font.hpp>
#include "Bootstrapper.h"
#include "drawing/actors/Actor.h"
#include "drawing/actors/Parent.h"
#include "drawing/actors/VBox.h"
#include "drawing/actors/HBox.h"
#include "drawing/actors/Quad.h"
#include "drawing/actors/Padding.h"
#include "drawing/actors/Align.h"

auto
defineActor(sol::state& target) -> void
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
    actorType["matchParentWidth"] =
      sol::property(&drawing::actors::Actor::matchParentWidth);
    actorType["matchParentHeight"] =
      sol::property(&drawing::actors::Actor::matchParentHeight);
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
      [&target](drawing::actors::AbstractVectorCollection* self, int index) {
          return (*self)[static_cast<std::size_t>(index) - 1]->getLuaSelf(
            target);
      };
    abstractVectorCollectionType["size"] =
      sol::property(&drawing::actors::AbstractVectorCollection::getSize);
}

auto
defineVBox(sol::state& target) -> void
{
    auto vBoxType = target.new_usertype<drawing::actors::VBox>(
      "VBox",
      sol::factories([]() { return std::make_shared<drawing::actors::VBox>(); },
                     [](std::vector<drawing::actors::Actor*> children) {
                         auto result =
                           std::make_shared<drawing::actors::VBox>();
                         for (auto* child : children) {
                             result->addChild(child->shared_from_this());
                         }
                         return result;
                     }),
      sol::base_classes,
      sol::bases<drawing::actors::Actor,
                 drawing::actors::Parent,
                 drawing::actors::AbstractVectorCollection>());
}

auto
defineHBox(sol::state& target) -> void
{
    auto hBoxType = target.new_usertype<drawing::actors::HBox>(
      "HBox",
      sol::factories([]() { return std::make_shared<drawing::actors::HBox>(); },
                     [](std::vector<drawing::actors::Actor*> children) {
                         auto result =
                           std::make_shared<drawing::actors::HBox>();
                         for (auto* child : children) {
                             result->addChild(child->shared_from_this());
                         }
                         return result;
                     }),
      sol::base_classes,
      sol::bases<drawing::actors::Actor,
                 drawing::actors::Parent,
                 drawing::actors::AbstractVectorCollection>());
}

auto
defineVector2(sol::state& target) -> void
{
    auto vector2fType = target.new_usertype<sf::Vector2f>(
      "Vector2",
      sol::constructors<sf::Vector2f(), sf::Vector2f(float, float)>());
    vector2fType["x"] = sol::property(&sf::Vector2f::x, &sf::Vector2f::x);
    vector2fType["y"] = sol::property(&sf::Vector2f::y, &sf::Vector2f::y);
}

auto
defineQuad(sol::state& target) -> void
{
    auto quadType = target.new_usertype<drawing::actors::Quad>(
      "Quad",
      sol::factories(
        []() { return std::make_shared<drawing::actors::Quad>(); },
        [](sol::table args) {
            auto result = std::make_shared<drawing::actors::Quad>(
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
            return result;
        }),
      sol::base_classes,
      sol::bases<drawing::actors::Actor>());
    quadType["fillColor"] = sol::property(&drawing::actors::Quad::getFillColor,
                                          &drawing::actors::Quad::setFillColor);
    quadType["outlineColor"] =
      sol::property(&drawing::actors::Quad::getOutlineColor,
                    &drawing::actors::Quad::setOutlineColor);
    quadType["outlineThickness"] =
      sol::property(&drawing::actors::Quad::getOutlineThickness,
                    &drawing::actors::Quad::setOutlineThickness);
    quadType["getPoint"] = &drawing::actors::Quad::getPoint;
    quadType["size"] = sol::property(&drawing::actors::Quad::getSize,
                                     &drawing::actors::Quad::setSize);
}

auto
defineColor(sol::state& target) -> void
{
    auto colorType = target.new_usertype<sf::Color>(
      "Color", sol::constructors<sf::Color(), sf::Color(int, int, int, int)>());
    colorType["r"] = sol::property(&sf::Color::r, &sf::Color::r);
    colorType["g"] = sol::property(&sf::Color::g, &sf::Color::g);
    colorType["b"] = sol::property(&sf::Color::b, &sf::Color::b);
    colorType["a"] = sol::property(&sf::Color::a, &sf::Color::a);
}

auto
definePadding(sol::state& target) -> void
{
    auto paddingType = target.new_usertype<drawing::actors::Padding>(
      "Padding",
      sol::factories(
        []() { return std::make_shared<drawing::actors::Padding>(); },
        [](drawing::actors::Actor* actor) {
            auto returnVal = std::make_shared<drawing::actors::Padding>();
            returnVal->setChild(actor->shared_from_this());
            return returnVal;
        },
        [](drawing::actors::Actor* actor, const sol::table& args) {
            auto returnVal = std::make_shared<drawing::actors::Padding>(
              args.get_or("left", 0.F),
              args.get_or("top", 0.F),
              args.get_or("right", 0.F),
              args.get_or("bottom", 0.F));
            returnVal->setChild(actor->shared_from_this());
            return returnVal;
        },
        [](const sol::table& args) {
            auto child = [&]() {
                if (args["child"].valid()) {
                    return args.get<drawing::actors::Actor*>("child")
                      ->shared_from_this();
                }
                return std::shared_ptr<drawing::actors::Actor>();
            }();
            auto returnVal = std::make_shared<drawing::actors::Padding>(
              args.get_or("left", 0.F),
              args.get_or("top", 0.F),
              args.get_or("right", 0.F),
              args.get_or("bottom", 0.F));
            returnVal->setChild(child);
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
defineAlign(sol::state& target) -> void
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
        []() { return std::make_shared<drawing::actors::Align>(); },
        [](drawing::actors::Actor* actor) {
            auto returnVal = std::make_shared<drawing::actors::Align>();
            returnVal->setChild(actor->shared_from_this());
            return returnVal;
        },
        [](drawing::actors::Actor* actor, drawing::actors::Align::Mode mode) {
            auto returnVal = std::make_shared<drawing::actors::Align>(mode);
            returnVal->setChild(actor->shared_from_this());
            return returnVal;
        },
        [](drawing::actors::Align::Mode mode) {
            return std::make_shared<drawing::actors::Align>(mode);
        },
        [](const sol::table& args) {
            auto child = [&]() {
                if (args["child"].valid()) {
                    return args.get<drawing::actors::Actor*>("child")
                      ->shared_from_this();
                }
                return std::shared_ptr<drawing::actors::Actor>();
            }();
            auto returnVal = std::make_shared<drawing::actors::Align>(
              args.get_or("mode", drawing::actors::Align::Mode::Center));
            returnVal->setChild(std::move(child));
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
lua::Bootstrapper::defineCommonTypes(sol::state& target) const -> void
{
    defineActor(target);
    defineParent(target);
    defineAbstractVectorCollection(target);
    defineVBox(target);
    defineHBox(target);
    defineVector2(target);
    defineQuad(target);
    defineColor(target);
    definePadding(target);
    defineAlign(target);
}

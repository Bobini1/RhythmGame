//
// Created by bobini on 10.08.2022.
//

#include "Bootstrapper.h"
#include "drawing/actors/Actor.h"
#include "drawing/actors/Parent.h"
#include "drawing/actors/VBox.h"
#include "drawing/actors/HBox.h"
#include "drawing/actors/Quad.h"
auto
lua::Bootstrapper::defineTypes(sol::state& target) const -> void
{
    auto actorType =
      target.new_usertype<drawing::actors::Actor>("Actor", sol::no_constructor);
    actorType["getParent"] =
      [&target](drawing::actors::Actor* self) { return self->getParent()->getLuaSelf(target); };

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
    auto parentType = target.new_usertype<drawing::actors::Parent>(
      "Parent",
      sol::no_constructor,
      sol::base_classes,
      sol::bases<drawing::actors::Actor>());
    parentType["removeChild"] = &drawing::actors::Parent::removeChild;
    auto abstractVectorCollectionType =
      target.new_usertype<drawing::actors::AbstractVectorCollection>(
        "AbstractVectorCollection",
        sol::no_constructor,
        sol::base_classes,
        sol::bases<drawing::actors::Actor, drawing::actors::Parent>());
    abstractVectorCollectionType["addChild"] = [](drawing::actors::AbstractVectorCollection* self, drawing::actors::Actor* child) {
        self->addChild(child->shared_from_this());
    };
    abstractVectorCollectionType["removeChild"] =
      &drawing::actors::AbstractVectorCollection::removeChild;
    abstractVectorCollectionType["getChild"] =
      [&target](drawing::actors::AbstractVectorCollection* self, int index) {
          return (*self)[static_cast<std::size_t>(index)-1]->getLuaSelf(target);
      };
    auto vBoxType = target.new_usertype<drawing::actors::VBox>(
      "VBox",
      sol::factories(
        []() { return std::make_shared<drawing::actors::VBox>(); }),
      sol::base_classes,
      sol::bases<drawing::actors::Actor,
                 drawing::actors::Parent,
                 drawing::actors::AbstractVectorCollection>());
    auto hBoxType = target.new_usertype<drawing::actors::HBox>(
      "HBox",
      sol::factories(
        []() { return std::make_shared<drawing::actors::HBox>(); }),
      sol::base_classes,
      sol::bases<drawing::actors::Actor,
                 drawing::actors::Parent,
                 drawing::actors::AbstractVectorCollection>());
    auto vector2fType = target.new_usertype<sf::Vector2f>(
      "Vector2f",
      sol::constructors<sf::Vector2f(), sf::Vector2f(float, float)>());
    vector2fType["x"] = sol::property(&sf::Vector2f::x, &sf::Vector2f::x);
    vector2fType["y"] = sol::property(&sf::Vector2f::y, &sf::Vector2f::y);

    auto quadType = target.new_usertype<drawing::actors::Quad>(
      "Quad",
      sol::factories(
        []() { return std::make_shared<drawing::actors::Quad>(); }),
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

    auto colorType = target.new_usertype<sf::Color>(
      "Color", sol::constructors<sf::Color(), sf::Color(int, int, int, int)>());
    colorType["r"] = sol::property(&sf::Color::r, &sf::Color::r);
    colorType["g"] = sol::property(&sf::Color::g, &sf::Color::g);
    colorType["b"] = sol::property(&sf::Color::b, &sf::Color::b);
    colorType["a"] = sol::property(&sf::Color::a, &sf::Color::a);
}

//
// Created by bobini on 19.11.22.
//

#include "Layers.h"
#include "SFML/Graphics/RenderTarget.hpp"
#include <ranges>

auto
drawing::actors::Layers::getLuaSelf(sol::state& lua) -> sol::object
{
    return { lua,
             sol::in_place_type_t<std::shared_ptr<Layers>>(),
             sharedFromBase<Layers>() };
}
void
drawing::actors::Layers::setTransform(sf::Transform newTransform)
{
    transform = newTransform;

    setWidth(getWidth());
    setHeight(getHeight());

    for (const auto& child : *this) {
        child->setTransform(transform);
    }
}
auto
drawing::actors::Layers::getTransform() const -> sf::Transform
{
    return transform;
}
auto
drawing::actors::Layers::getIsWidthManaged() const -> bool
{
    return !mainLayer || mainLayer->getIsWidthManaged();
}
auto
drawing::actors::Layers::getIsHeightManaged() const -> bool
{
    return !mainLayer || mainLayer->getIsHeightManaged();
}
auto
drawing::actors::Layers::getMinWidth() const -> float
{
    return getMinSize().x;
}
auto
drawing::actors::Layers::getMinHeight() const -> float
{
    return getMinSize().y;
}
auto
drawing::actors::Layers::getWidth() const -> float
{
    return getCurrentSize().x;
}
auto
drawing::actors::Layers::getHeight() const -> float
{
    return getCurrentSize().y;
}
void
drawing::actors::Layers::draw(sf::RenderTarget& target,
                              sf::RenderStates states) const
{
    for (const auto& child : *this) {
        target.draw(*child, states);
    }
}
void
drawing::actors::Layers::setWidthImpl(float width)
{
    size.x = width;
    for (const auto& child : *this) {
        if (child->getIsWidthManaged()) {
            child->setWidth(width);
        }
    }
}
void
drawing::actors::Layers::setHeightImpl(float height)
{
    size.y = height;
    for (const auto& child : *this) {
        if (child->getIsHeightManaged()) {
            child->setHeight(height);
        }
    }
}
auto
drawing::actors::Layers::onChildRemoved(std::shared_ptr<Actor> child) -> void
{
    if (child == mainLayer) {
        mainLayer = nullptr;
    }
    setWidth(getWidth());
    setHeight(getHeight());
}
auto
drawing::actors::Layers::setMainLayer(std::shared_ptr<Actor> layer) -> void
{
    mainLayer = std::move(layer);
    addChild(mainLayer);
}
auto
drawing::actors::Layers::getMainLayer() const -> std::shared_ptr<Actor>
{
    return mainLayer;
}
auto
drawing::actors::Layers::getMinSize() const -> sf::Vector2f
{
    auto minSize = sf::Vector2f{ 0, 0 };
    if (mainLayer) {
        minSize.x = mainLayer->getIsWidthManaged() ? mainLayer->getMinWidth()
                                                   : mainLayer->getWidth();

        minSize.y = mainLayer->getIsHeightManaged() ? mainLayer->getMinHeight()
                                                    : mainLayer->getHeight();
    } else {
        for (const auto& child : *this) {
            minSize.x =
              std::max(minSize.x,
                       child->getIsWidthManaged() ? child->getMinWidth()
                                                  : child->getWidth());
            minSize.y =
              std::max(minSize.y,
                       child->getIsHeightManaged() ? child->getMinHeight()
                                                   : child->getHeight());
        }
    }
    return minSize;
}
auto
drawing::actors::Layers::getCurrentSize() const -> sf::Vector2f
{
    auto currentSize = size;
    if (mainLayer) {
        currentSize.x = mainLayer->getWidth();
        currentSize.y = mainLayer->getHeight();
    } else {
        for (const auto& child : *this) {
            currentSize.x = std::max(currentSize.x, child->getWidth());
            currentSize.y = std::max(currentSize.y, child->getHeight());
        }
    }
    return currentSize;
}
auto
drawing::actors::Layers::make() -> std::shared_ptr<Layers>
{
    return std::shared_ptr<Layers>(new Layers{});
}
auto
drawing::actors::Layers::getAllChildrenAtMousePosition(
  sf::Vector2f position,
  std::set<std::weak_ptr<const Actor>,
           std::owner_less<std::weak_ptr<const Actor>>>& result) const -> bool
{
    auto obstructed = false;
    for (auto child = crbegin(); child < crend(); ++child) {
        obstructed |= (*child)->getAllActorsAtMousePosition(position, result);
        if (obstructed) {
            break;
        }
    }
    return obstructed;
}

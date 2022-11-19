//
// Created by bobini on 19.11.22.
//

#include "Layers.h"
#include "SFML/Graphics/RenderTarget.hpp"

auto
drawing::actors::Layers::getLuaSelf(sol::state& lua) -> sol::object
{
    return { lua,
             sol::in_place_type_t<std::shared_ptr<Layers>>(),
             sharedFromBase<Layers>() };
}
void
drawing::actors::Layers::update(std::chrono::nanoseconds delta)
{
}
void
drawing::actors::Layers::setTransform(sf::Transform newTransform)
{
    transform = newTransform;
    setHeight(getHeight());
    setWidth(getWidth());

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
    for (const auto& child : *this) {
        if (child->getIsWidthManaged()) {
            child->setWidth(width);
        }
    }
}
void
drawing::actors::Layers::setHeightImpl(float height)
{
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
    auto size = sf::Vector2f{ 0, 0 };
    if (mainLayer) {
        size.x = mainLayer->getWidth();
        size.y = mainLayer->getHeight();
    } else {
        for (const auto& child : *this) {
            size.x = std::max(size.x, child->getWidth());
            size.y = std::max(size.y, child->getHeight());
        }
    }
    return size;
}

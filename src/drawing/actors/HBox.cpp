//
// Created by bobini on 05.11.22.
//

#include "HBox.h"
#include <SFML/Graphics/RenderTarget.hpp>
#include <set>
void
drawing::actors::HBox::update(std::chrono::nanoseconds delta)
{
}
void
drawing::actors::HBox::draw(sf::RenderTarget& target,
                            sf::RenderStates states) const
{
    for (const auto& child : *this) {
        target.draw(*child, states);
    }
}

auto
drawing::actors::HBox::matchParentWidth() const -> bool
{
    return false;
}
auto
drawing::actors::HBox::matchParentHeight() const -> bool
{
    return false;
}
auto
drawing::actors::HBox::getMinWidth() const -> float
{
    return getMinimumSizeOfChildren().x;
}
auto
drawing::actors::HBox::getMinHeight() const -> float
{
    return getMinimumSizeOfChildren().y;
}
auto
drawing::actors::HBox::getWidth() const -> float
{
    return size.x;
}
auto
drawing::actors::HBox::getHeight() const -> float
{
    return size.y;
}
void
drawing::actors::HBox::setWidthImpl(float width)
{
    size.x = std::max(width, getMinWidth());
}
void
drawing::actors::HBox::setHeightImpl(float height)
{
    size.y = std::max(height, getMinHeight());
}
auto
drawing::actors::HBox::getMinimumSizeOfChildren() const -> sf::Vector2f
{
    auto width = 0.F;
    auto height = 0.F;

    for (const auto& child : *this) {
        height = std::max(height,
                          child->matchParentHeight() ? child->getMinHeight()
                                                     : child->getHeight());
        width +=
          child->matchParentWidth() ? child->getMinWidth() : child->getWidth();
    }
    return { width, height };
}
void
drawing::actors::HBox::setTransform(sf::Transform newTransform)
{
    transform = newTransform;
    auto workingTransform = transform;
    auto minimumSize = getMinimumSizeOfChildren();

    auto childrenMatchingParentWidth =
      std::count_if(cbegin(), cend(), [](const auto& child) {
          return child->matchParentWidth();
      });
    auto growth = (size.x - minimumSize.x) /
                  static_cast<float>(childrenMatchingParentWidth);
    for (const auto& child : *this) {
        if (child->matchParentWidth()) {
            auto childMinWidth = child->getMinWidth();
            child->setHeight(childMinWidth + growth);
        }
        if (child->matchParentHeight()) {
            child->setHeight(size.y);
        }
        child->setTransform(workingTransform);
        workingTransform.translate(child->getWidth(), 0);
    }
}
auto
drawing::actors::HBox::getTransform() const -> sf::Transform
{
    return transform;
}
void
drawing::actors::HBox::recalculateSize()
{
    auto minimumSize = getMinimumSizeOfChildren();
    if (minimumSize.x > size.x) {
        setWidth(minimumSize.x);
    }
    if (minimumSize.y > size.y) {
        setHeight(minimumSize.y);
    }
}
auto
drawing::actors::HBox::getLuaSelf(sol::state& lua) -> sol::object
{
    return { lua,
             sol::in_place_type_t<std::shared_ptr<HBox>>(),
             sharedFromBase<HBox>() };
}

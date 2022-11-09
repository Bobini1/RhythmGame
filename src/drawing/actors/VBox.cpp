//
// Created by bobini on 13.10.22.
//

#include "VBox.h"
#include <SFML/Graphics/RenderTarget.hpp>
#include <set>
void
drawing::actors::VBox::update(std::chrono::nanoseconds delta)
{
}
auto
drawing::actors::VBox::getLuaSelf(sol::state& lua) -> sol::object
{
    return { lua,
             sol::in_place_type_t<std::shared_ptr<VBox>>(),
             sharedFromBase<VBox>() };
}
void
drawing::actors::VBox::draw(sf::RenderTarget& target,
                            sf::RenderStates states) const
{
    for (const auto& child : *this) {
        target.draw(*child, states);
    }
}
void
drawing::actors::VBox::setTransform(sf::Transform newTransform)
{
    transform = newTransform;
    auto workingTransform = transform;
    auto minimumSize = getMinimumSizeOfChildren();

    auto childrenMatchingParentHeight =
      std::count_if(cbegin(), cend(), [](const auto& child) {
          return child->matchParentHeight();
      });
    auto growth = (size.y - minimumSize.y) /
                  static_cast<float>(childrenMatchingParentHeight);
    for (const auto& child : *this) {
        if (child->matchParentHeight()) {
            auto childMinHeight = child->getMinHeight();
            child->setHeight(childMinHeight + growth);
        }
        if (child->matchParentWidth()) {
            child->setWidth(size.x);
        }
        child->setTransform(workingTransform);
        workingTransform.translate(0, child->getHeight());
    }
}
auto
drawing::actors::VBox::getTransform() const -> sf::Transform
{
    return transform;
}
auto
drawing::actors::VBox::getMinimumSizeOfChildren() const -> sf::Vector2f
{
    auto width = 0.F;
    auto height = 0.F;

    for (const auto& child : *this) {
        height += child->matchParentHeight() ? child->getMinHeight()
                                             : child->getHeight();
        width = std::max(width,
                         child->matchParentWidth() ? child->getMinWidth()
                                                   : child->getWidth());
    }
    return { width, height };
}
auto
drawing::actors::VBox::matchParentWidth() const -> bool
{
    return false;
}
auto
drawing::actors::VBox::matchParentHeight() const -> bool
{
    return false;
}
auto
drawing::actors::VBox::getMinWidth() const -> float
{
    return getMinimumSizeOfChildren().x;
}
auto
drawing::actors::VBox::getMinHeight() const -> float
{
    return getMinimumSizeOfChildren().y;
}
auto
drawing::actors::VBox::getWidth() const -> float
{
    return size.x;
}
auto
drawing::actors::VBox::getHeight() const -> float
{
    return size.y;
}
void
drawing::actors::VBox::setWidthImpl(float width)
{
    size.x = std::max(width, getMinWidth());
}
void
drawing::actors::VBox::setHeightImpl(float height)
{
    size.y = std::max(height, getMinHeight());
}
void
drawing::actors::VBox::recalculateSize()
{
    auto minimumSize = getMinimumSizeOfChildren();
    if (minimumSize.x > size.x) {
        setWidth(minimumSize.x);
    }
    if (minimumSize.y > size.y) {
        setHeight(minimumSize.y);
    }
}

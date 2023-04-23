//
// Created by bobini on 18.11.22.
//

#include "AbstractBox.h"

auto
drawing::actors::AbstractBox::setVerticalSizeMode(
  drawing::actors::AbstractBox::SizeMode mode) -> void
{
    verticalSizeMode = mode;
}
auto
drawing::actors::AbstractBox::getVerticalSizeMode() const -> SizeMode
{
    return verticalSizeMode;
}
auto
drawing::actors::AbstractBox::setHorizontalSizeMode(SizeMode mode) -> void
{
    horizontalSizeMode = mode;
}
auto
drawing::actors::AbstractBox::getHorizontalSizeMode() const -> SizeMode
{
    return horizontalSizeMode;
}
auto
drawing::actors::AbstractBox::setSpacing(float newSpacing) -> void
{
    spacing = newSpacing;
}
auto
drawing::actors::AbstractBox::getSpacing() const -> float
{
    return spacing;
}
auto
drawing::actors::AbstractBox::getAllChildrenAtMousePosition(
  sf::Vector2f position,
  std::set<std::weak_ptr<const Actor>,
           std::owner_less<std::weak_ptr<const Actor>>>& result) const -> bool
{
    auto blocked = false;
    for (const auto& child : *this) {
        blocked |= child->getAllActorsAtMousePosition(position, result);
    }
    return blocked;
}

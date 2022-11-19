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

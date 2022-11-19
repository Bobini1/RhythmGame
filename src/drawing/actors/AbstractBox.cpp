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
drawing::actors::AbstractBox::setTopPadding(float padding) -> void
{
    topPadding = padding;
}
auto
drawing::actors::AbstractBox::getTopPadding() const -> float
{
    return topPadding;
}
auto
drawing::actors::AbstractBox::setBottomPadding(float padding) -> void
{
    bottomPadding = padding;
}
auto
drawing::actors::AbstractBox::getBottomPadding() const -> float
{
    return bottomPadding;
}
auto
drawing::actors::AbstractBox::setLeftPadding(float padding) -> void
{
    leftPadding = padding;
}
auto
drawing::actors::AbstractBox::getLeftPadding() const -> float
{
    return leftPadding;
}
auto
drawing::actors::AbstractBox::setRightPadding(float padding) -> void
{
    rightPadding = padding;
}
auto
drawing::actors::AbstractBox::getRightPadding() const -> float
{
    return rightPadding;
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

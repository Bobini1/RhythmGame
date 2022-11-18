//
// Created by bobini on 18.11.22.
//

#include "AbstractRectLeaf.h"

namespace drawing::actors {
auto
AbstractRectLeaf::setIsWidthManaged(bool newIsWidthManaged) -> void
{
    isWidthManaged = newIsWidthManaged;
}
auto
AbstractRectLeaf::setIsHeightManaged(bool newIsHeightManaged) -> void
{
    isHeightManaged = newIsHeightManaged;
}
auto
AbstractRectLeaf::setMinWidth(float newMinWidth) -> void
{
    minWidth = newMinWidth;
}
auto
AbstractRectLeaf::setMinHeight(float newMinHeight) -> void
{
    minHeight = newMinHeight;
}
auto
AbstractRectLeaf::getIsWidthManaged() const -> bool
{
    return isWidthManaged;
}
auto
AbstractRectLeaf::getIsHeightManaged() const -> bool
{
    return isHeightManaged;
}
auto
AbstractRectLeaf::getMinWidth() const -> float
{
    return minWidth;
}
auto
AbstractRectLeaf::getMinHeight() const -> float
{
    return minHeight;
}
}
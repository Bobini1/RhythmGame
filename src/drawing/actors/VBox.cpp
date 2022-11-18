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
    recalculateSize();
    transform = newTransform;
    auto workingTransform = transform;
    auto minimumSize = getMinimumSizeOfChildren();

    auto childrenMatchingParentHeight =
      std::count_if(cbegin(), cend(), [](const auto& child) {
          return child->getIsHeightManaged();
      });

    auto growth = (size.y - minimumSize.y) /
                  static_cast<float>(childrenMatchingParentHeight);

    for (const auto& child : *this) {
        if (child->getIsHeightManaged()) {
            auto childMinHeight = child->getMinHeight();
            child->setHeight(childMinHeight + growth);
        }
        if (child->getIsWidthManaged()) {
            child->setWidth(size.x);
        }
        auto childTransform = workingTransform;
        switch (contentAlignment) {
            case ContentAlignment::Left:
                childTransform.translate(0, 0);
                break;
            case ContentAlignment::Center:
                childTransform.translate((size.x - child->getWidth()) / 2, 0);
                break;
            case ContentAlignment::Right:
                childTransform.translate(size.x - child->getWidth(), 0);
                break;
        }
        child->setTransform(childTransform);
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
        height += child->getIsHeightManaged() ? child->getMinHeight()
                                              : child->getHeight();
        width = std::max(width,
                         child->getIsWidthManaged() ? child->getMinWidth()
                                                    : child->getWidth());
    }
    return { width, height };
}
auto
drawing::actors::VBox::getIsWidthManaged() const -> bool
{
    return getHorizontalSizeMode() == SizeMode::Managed;
}
auto
drawing::actors::VBox::getIsHeightManaged() const -> bool
{
    return getVerticalSizeMode() == SizeMode::Managed;
}
auto
drawing::actors::VBox::getMinWidth() const -> float
{
    return getHorizontalSizeMode() == SizeMode::Fixed ? 0 : getMinimumSizeOfChildren().x;
}
auto
drawing::actors::VBox::getMinHeight() const -> float
{
    return getVerticalSizeMode() == SizeMode::Fixed ? 0 : getMinimumSizeOfChildren().y;
}
auto
drawing::actors::VBox::getWidth() const -> float
{
    return getHorizontalSizeMode() == SizeMode::Fixed
             ? size.x
             : std::max(size.x, getMinimumSizeOfChildren().x);
}
auto
drawing::actors::VBox::getHeight() const -> float
{
    return getVerticalSizeMode() == SizeMode::Fixed
             ? size.y
             : std::max(size.y, getMinimumSizeOfChildren().y);
}
void
drawing::actors::VBox::setWidthImpl(float width)
{
    size.x = width;
}
void
drawing::actors::VBox::setHeightImpl(float height)
{
    size.y = height;
}
void
drawing::actors::VBox::recalculateSize()
{
    auto minimumSize = getMinimumSizeOfChildren();
    if (getVerticalSizeMode() == SizeMode::WrapChildren) {
        size.y = minimumSize.y;
    } else if (getVerticalSizeMode() == SizeMode::Managed) {
        size.y = std::max(size.y, minimumSize.y);
    }
    if (getHorizontalSizeMode() == SizeMode::WrapChildren) {
        size.x = minimumSize.x;
    } else if (getHorizontalSizeMode() == SizeMode::Managed) {
        size.x = std::max(size.x, minimumSize.x);
    }
}
auto
drawing::actors::VBox::getLuaSelf(sol::state& lua) -> sol::object
{
    return { lua,
             sol::in_place_type_t<std::shared_ptr<VBox>>(),
             sharedFromBase<VBox>() };
}

auto
drawing::actors::VBox::setContentAlignment(
  drawing::actors::VBox::ContentAlignment alignment) -> void
{
    contentAlignment = alignment;
}
auto
drawing::actors::VBox::getContentAlignment() const
  -> drawing::actors::VBox::ContentAlignment
{
    return contentAlignment;
}

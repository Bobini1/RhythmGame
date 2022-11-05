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
auto
drawing::actors::HBox::getLuaSelf(sol::state& lua) -> sol::object
{
    return { lua,
             sol::in_place_type_t<std::shared_ptr<HBox>>(),
             sharedFromBase<HBox>() };
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
drawing::actors::HBox::measure(drawing::MeasurementSpec widthSpec,
                               drawing::MeasurementSpec heightSpec) const
  -> sf::Vector2f
{
    auto size = getTotalSizeOfChildrenWithoutMatchParent(widthSpec, heightSpec);

    if (std::find_if(cbegin(), cend(), [](const auto& child) {
            return child->matchParentWidth();
        }) != cend()) {
        size.x = widthSpec.value_or(std::numeric_limits<float>::max());
    }
    return size;
}
void
drawing::actors::HBox::setLayout(sf::FloatRect newLayout)
{
    layout = newLayout;
    auto left = layout.left;
    auto measurement = measure(layout.width, layout.height);
    auto childrenMatchingParentWidth = std::count_if(cbegin(), cend(), [](const auto& child) {
        return child->matchParentWidth();
    });
    for (const auto& child : *this) {
        auto width = std::optional<float>(layout.width);
        auto height = std::optional<float>(layout.height);
        if (child->matchParentWidth()) {
            width = (layout.width - measurement.x) /
                     static_cast<float>(childrenMatchingParentWidth);
        }
        auto childSize = child->measure(width, height);
        child->setLayout({ left, layout.top, childSize.x, childSize.y });
        left += childSize.x;
    }
}
auto
drawing::actors::HBox::getLayout() const -> sf::FloatRect
{
    return layout;
}
auto
drawing::actors::HBox::getTotalSizeOfChildrenWithoutMatchParent(drawing::MeasurementSpec widthSpec,
                                                                drawing::MeasurementSpec heightSpec) const
  -> sf::Vector2f
{
    auto width = 0.F;
    auto height = 0.F;

    for (const auto& child : *this) {
        auto childSize = child->measure(widthSpec, heightSpec);
        if (!child->matchParentWidth()) {
            width += childSize.x;
        }
        if (!child->matchParentHeight()) {
            height += std::max(height, childSize.y);
        }
    }
    return { width, height };
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

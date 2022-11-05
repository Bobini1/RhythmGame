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
auto
drawing::actors::VBox::measure(drawing::MeasurementSpec widthSpec,
                               drawing::MeasurementSpec heightSpec) const
  -> sf::Vector2f
{
    auto size = getTotalSizeOfChildrenWithoutMatchParent(widthSpec, heightSpec);

    if (std::find_if(cbegin(), cend(), [](const auto& child) {
            return child->matchParentHeight();
        }) != cend()) {
        size.y = heightSpec.value_or(std::numeric_limits<float>::max());
    }
    return size;
}
void
drawing::actors::VBox::setLayout(sf::FloatRect newLayout)
{
    layout = newLayout;
    auto top = layout.top;
    auto measurement = measure(layout.width, layout.height);
    auto childrenMatchingParentHeight = std::count_if(cbegin(), cend(), [](const auto& child) {
        return child->matchParentHeight();
    });
    for (const auto& child : *this) {
        auto width = std::optional<float>(layout.width);
        auto height = std::optional<float>(layout.height);
        if (child->matchParentHeight()) {
            height = (layout.height - measurement.y) /
                     static_cast<float>(childrenMatchingParentHeight);
        }
        auto childSize = child->measure(width, height);
        child->setLayout({ layout.left, top, childSize.x, childSize.y });
        top += childSize.y;
    }
}
auto
drawing::actors::VBox::getLayout() const -> sf::FloatRect
{
    return layout;
}
auto
drawing::actors::VBox::getTotalSizeOfChildrenWithoutMatchParent(drawing::MeasurementSpec widthSpec,
                                                                drawing::MeasurementSpec heightSpec) const
  -> sf::Vector2f
{
    auto width = 0.F;
    auto height = 0.F;

    for (const auto& child : *this) {
        auto childSize = child->measure(widthSpec, heightSpec);
        if (!child->matchParentWidth()) {
            width = std::max(width, childSize.x);
        }
        if (!child->matchParentHeight()) {
            height += childSize.y;
        }
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

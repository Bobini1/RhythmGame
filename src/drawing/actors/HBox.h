//
// Created by bobini on 05.11.22.
//

#ifndef RHYTHMGAME_HBOX_H
#define RHYTHMGAME_HBOX_H
#include "AbstractVectorCollection.h"
namespace drawing::actors {
class HBox : public AbstractVectorCollection
{
  public:
    [[nodiscard]] auto measure(MeasurementSpec widthSpec, MeasurementSpec heightSpec) const
      -> sf::Vector2f override;
    auto setLayout(sf::FloatRect layout) -> void override;
    [[nodiscard]] auto getLayout() const -> sf::FloatRect override;
    auto update(std::chrono::nanoseconds delta) -> void override;
    [[nodiscard]] auto getLuaSelf(sol::state& lua) -> sol::object override;
    [[nodiscard]] auto matchParentWidth() const -> bool override;
    [[nodiscard]] auto matchParentHeight() const -> bool override;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  private:
    sf::FloatRect layout;
    auto getTotalSizeOfChildrenWithoutMatchParent(
      drawing::MeasurementSpec widthSpec,
      drawing::MeasurementSpec heightSpec) const -> sf::Vector2f;
};
} // namespace drawing::actors

#endif // RHYTHMGAME_HBOX_H

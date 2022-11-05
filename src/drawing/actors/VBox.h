//
// Created by bobini on 13.10.22.
//

#ifndef RHYTHMGAME_VBOX_H
#define RHYTHMGAME_VBOX_H
#include "AbstractVectorCollection.h"
#include "Parent.h"
#include <SFML/Graphics/Texture.hpp>
namespace drawing::actors {
class VBox : public AbstractVectorCollection
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
#endif // RHYTHMGAME_VBOX_H

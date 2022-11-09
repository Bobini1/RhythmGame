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
    auto getMinWidth() const -> float override;
    auto getMinHeight() const -> float override;
    auto getWidth() const -> float override;
    auto getHeight() const -> float override;
    auto update(std::chrono::nanoseconds delta) -> void override;
    [[nodiscard]] auto getLuaSelf(sol::state& lua) -> sol::object override;
    [[nodiscard]] auto matchParentWidth() const -> bool override;
    [[nodiscard]] auto matchParentHeight() const -> bool override;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;


  private:
  public:
    auto setTransform(sf::Transform transform) -> void override;
    auto getTransform() const -> sf::Transform override;

  private:
    sf::Transform transform;
    sf::Vector2f size;
    auto getMinimumSizeOfChildren() const -> sf::Vector2f;
    void recalculateSize() override;
};
} // namespace drawing::actors

#endif // RHYTHMGAME_HBOX_H

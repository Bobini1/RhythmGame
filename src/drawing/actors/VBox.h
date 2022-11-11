//
// Created by bobini on 13.10.22.
//

#ifndef RHYTHMGAME_VBOX_H
#define RHYTHMGAME_VBOX_H
#include "AbstractVectorCollection.h"
#include "Parent.h"
#include <SFML/Graphics/Texture.hpp>
namespace drawing::actors {
/**
 * A VBox is a parent that arranges its children vertically.
 */
class VBox : public AbstractVectorCollection
{
  public:
    auto getMinWidth() const -> float override;
    auto getMinHeight() const -> float override;
    auto getWidth() const -> float override;
    auto getHeight() const -> float override;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;

  public:
    auto setTransform(sf::Transform newTransform) -> void override;
    [[nodiscard]] auto getTransform() const -> sf::Transform override;
    auto update(std::chrono::nanoseconds delta) -> void override;
    [[nodiscard]] auto matchParentWidth() const -> bool override;
    [[nodiscard]] auto matchParentHeight() const -> bool override;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  private:
    sf::Transform transform;
    sf::Vector2f size;
    auto getMinimumSizeOfChildren() const -> sf::Vector2f;
    void recalculateSize() override;
};
} // namespace drawing::actors
#endif // RHYTHMGAME_VBOX_H

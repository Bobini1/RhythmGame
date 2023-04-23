//
// Created by bobini on 19.11.22.
//

#ifndef RHYTHMGAME_LAYERS_H
#define RHYTHMGAME_LAYERS_H
#include "AbstractVectorCollection.h"
namespace drawing::actors {
/**
 * @brief A Layers is a collection of actors that are drawn in a specific order.
 *
 * The order is determined by the order in which the actors are added to the
 * layer. The size of Layers is the size of the largest child or the size of the
 * main layer if it is set. If you set a main layer that was not also added as a
 * child, it will be added to children at the top of the stack.
 */
class Layers : public AbstractVectorCollection
{
    std::shared_ptr<Actor> mainLayer;
    sf::Transform transform;
    sf::Vector2f size;

  public:
    static auto make() -> std::shared_ptr<Layers>;
    auto getLuaSelf(sol::state& lua) -> sol::object override;
    auto setTransform(sf::Transform transform) -> void override;
    [[nodiscard]] auto getTransform() const -> sf::Transform override;
    [[nodiscard]] auto getIsWidthManaged() const -> bool override;
    [[nodiscard]] auto getIsHeightManaged() const -> bool override;
    [[nodiscard]] auto getMinWidth() const -> float override;
    [[nodiscard]] auto getMinHeight() const -> float override;
    [[nodiscard]] auto getWidth() const -> float override;
    [[nodiscard]] auto getHeight() const -> float override;
    auto setMainLayer(std::shared_ptr<Actor> layer) -> void;
    [[nodiscard]] auto getMainLayer() const -> std::shared_ptr<Actor>;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    Layers() = default;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;
    auto getAllChildrenAtMousePosition(
      sf::Vector2f position,
      std::set<std::weak_ptr<const Actor>,
               std::owner_less<std::weak_ptr<const Actor>>>& result) const
      -> bool override;
    auto onChildRemoved(std::shared_ptr<Actor> child) -> void override;
    [[nodiscard]] auto getMinSize() const -> sf::Vector2f;
    [[nodiscard]] auto getCurrentSize() const -> sf::Vector2f;
};
} // namespace drawing::actors

#endif // RHYTHMGAME_LAYERS_H

//
// Created by bobini on 19.11.22.
//

#ifndef RHYTHMGAME_LAYERS_H
#define RHYTHMGAME_LAYERS_H
#include "AbstractVectorCollection.h"
namespace drawing::actors {
class Layers : public AbstractVectorCollection
{
    std::shared_ptr<Actor> mainLayer;
    sf::Transform transform;

  public:
    auto getLuaSelf(sol::state& lua) -> sol::object override;
    auto update(std::chrono::nanoseconds delta) -> void override;
    auto setTransform(sf::Transform transform) -> void override;
    auto getTransform() const -> sf::Transform override;
    auto getIsWidthManaged() const -> bool override;
    auto getIsHeightManaged() const -> bool override;
    auto getMinWidth() const -> float override;
    auto getMinHeight() const -> float override;
    auto getWidth() const -> float override;
    auto getHeight() const -> float override;
    auto setMainLayer(std::shared_ptr<Actor> layer) -> void;
    auto getMainLayer() const -> std::shared_ptr<Actor>;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;
    auto onChildRemoved(std::shared_ptr<Actor> child) -> void override;
    auto getMinSize() const -> sf::Vector2f;
    auto getCurrentSize() const -> sf::Vector2f;
};
} // namespace drawing::actors

#endif // RHYTHMGAME_LAYERS_H

//
// Created by bobini on 21.04.23.
//

#ifndef RHYTHMGAME_FRAME_H
#define RHYTHMGAME_FRAME_H

#include <SFML/Graphics/RenderTexture.hpp>
#include "Parent.h"
namespace drawing::actors {
/**
 * @brief A frame that forces the child to render within its bounds.
 * @details This is done by passing a separate render target to the child. That
 * render target is then drawn on the parent's render target. If the child is
 * completely outside the parent's bounds, it is not drawn at all.
 */
class Frame : public Parent
{
    std::shared_ptr<Actor> child;
    sf::Vector2f size{};
    sf::Vector2f minSize{};
    sf::Vector2f offset{};
    sf::Transform transform{};
    bool isWidthManaged{};
    bool isHeightManaged{};

  public:
    /**
     * @brief Creates a frame.
     * @param child Child actor.
     */
    static auto make(std::shared_ptr<Actor> child = nullptr)
      -> std::shared_ptr<Frame>;
    auto draw(sf::RenderTarget& target, sf::RenderStates states) const
      -> void override;
    auto getLuaSelf(sol::state& lua) -> sol::object override;
    auto setTransform(sf::Transform transform) -> void override;
    auto getTransform() const -> sf::Transform override;
    auto getIsWidthManaged() const -> bool override;
    auto getIsHeightManaged() const -> bool override;
    auto setIsWidthManaged(bool managed) -> void;
    auto setIsHeightManaged(bool managed) -> void;
    auto getMinWidth() const -> float override;
    auto getMinHeight() const -> float override;
    auto setMinWidth(float minWidth) -> void;
    auto setMinHeight(float minHeight) -> void;
    auto getWidth() const -> float override;
    auto getHeight() const -> float override;
    /**
     * @brief Sets the offset of the child.
     * @param offset Offset.
     * @details The offset is relative to the frame's origin (upper left
     * corner).
     */
    auto setOffset(sf::Vector2f offset) -> void;
    /**
     * @brief Gets the offset of the child.
     * @return Offset.
     * @details The offset is relative to the frame's origin (upper left
     * corner).
     */
    auto getOffset() const -> sf::Vector2f;
    /**
     * @brief Sets the child node that this
     * @param size Size.
     * @details The size is the size of the frame's render target. The child
     * will be drawn within this size.
     */
    auto setChild(std::shared_ptr<Actor> child) -> void;
    auto getChild() const -> const std::shared_ptr<Actor>&;
    auto removeChild(std::shared_ptr<Actor> child) -> void override;
    virtual auto getAllChildrenAtMousePosition(
      sf::Vector2f position,
      std::set<std::weak_ptr<const Actor>,
               std::owner_less<std::weak_ptr<const Actor>>>& result) const
      -> void override;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;
};
} // namespace drawing::actors

#endif // RHYTHMGAME_FRAME_H

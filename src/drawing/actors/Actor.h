//
// Created by bobini on 30.07.2022.
//

#ifndef RHYTHMGAME_ACTOR_H
#define RHYTHMGAME_ACTOR_H

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <chrono>
#include <map>
#include <memory>
#include <sol/state.hpp>
#include "support/EnableSharedFromBase.h"
#include "events/Connection.h"
#include "drawing/animations/Animation.h"
#include <boost/signals2/connection.hpp>

/**
 * @namespace drawing::actors
 * @brief Contains all the classes that are used to draw the game. The lua API
 * is not defined here!
 */
namespace drawing::actors {
class Parent;
/**
 * @brief Base class for all drawable objects.
 */
class Actor // NOLINT(fuchsia-multiple-inheritance)
  : public sf::Drawable
  , public support::EnableSharedFromBase<Actor>
{
    std::weak_ptr<Parent> parent{};
    std::map<std::string, std::unique_ptr<events::Connection>>
      eventSubscriptions{};
  protected:
    Actor() = default;
    Actor(const Actor& /*unused*/);
    auto operator=(const Actor& /*unused*/) -> Actor&;

  public:
    Actor(Actor&& otherActor) noexcept = delete;
    auto operator=(Actor&& otherActor) noexcept -> Actor& = delete;
    ~Actor() override = default;

    void addEventSubscription(const std::string& eventName,
                              std::unique_ptr<events::Connection> connection);
    auto removeEventSubscription(const std::string& eventName) -> void;



    /**
     * @brief Get the lua object of the type of this actor.
     * @param lua The lua state to use.
     * @return The lua object of the type of this actor.
     */
    [[nodiscard]] virtual auto getLuaSelf(sol::state& lua) -> sol::object = 0;

    /**
     * @brief The parent of this actor. Null for scene root or unattached
     * actors.
     */
    [[nodiscard]] virtual auto getParent() const -> std::shared_ptr<Parent>;

    /**
     * @brief Sets the parent of the actor.
     * @param parent The new parent.
     * This should only be called by the parent when this actor gets
     * added to it. Not exposed to Lua.
     */
    virtual auto setParent(const std::shared_ptr<Parent>& parent) -> void;
    /**
     * @brief used by the engine to update an actor's internals every frame. Not
     * exposed to Lua.
     * @param delta the time difference between this frame and the previous one.
     */
    auto update(std::chrono::nanoseconds delta) -> void;
    /**
     * @brief set the transform of the actor and all its children. Not exposed
     * to lua.
     *
     * Should be called every frame. This method is responsible for updating the
     * entire state of a parent component according to the state of its
     * children.
     *
     * @param transform The new global transform of the actor.
     */
    virtual auto setTransform(sf::Transform transform) -> void = 0;
    /**
     * @brief Get the global transform of the actor. Not exposed to lua.
     * @return The global transform of the actor.
     */
    [[nodiscard]] virtual auto getTransform() const -> sf::Transform = 0;
    /**
     * @brief Does this actor want to have its width managed by its parent?
     */
    [[nodiscard]] virtual auto getIsWidthManaged() const -> bool = 0;
    /**
     * @brief Does this actor want to have its height managed by its parent?
     */
    [[nodiscard]] virtual auto getIsHeightManaged() const -> bool = 0;
    /**
     * @brief Get the minimum width that this actor can get resized to.
     * @return The minimum width of the actor.
     */
    [[nodiscard]] virtual auto getMinWidth() const -> float = 0;
    /**
     * @brief Get the minimum height that this actor can get resized to.
     * @return The minimum height of the actor.
     */
    [[nodiscard]] virtual auto getMinHeight() const -> float = 0;
    /**
     * @brief Get the current width of the actor.
     * @return The width of the actor.
     */
    [[nodiscard]] virtual auto getWidth() const -> float = 0;
    /**
     * @brief Get the current height of the actor.
     * @return The height of the actor.
     */
    [[nodiscard]] virtual auto getHeight() const -> float = 0;
    /**
     * @brief Set the width of the actor. If used with a value below the
     * minWidth of the actor, the minWidth will be used instead.
     * @param width The new width of the actor.
     */
    auto setWidth(float width) -> void;
    /**
     * @brief Set the height of the actor. If used with a value below the
     * minHeight of the actor, the minHeight will be used instead.
     * @param height The new height of the actor.
     */
    auto setHeight(float height) -> void;

  private:
    /**
     * @brief Actors need to override this to set their width. The contract is
     * to always resize to the requested size.
     * @param width The new width of the actor.
     */
    virtual auto setWidthImpl(float width) -> void = 0;
    /**
     * @brief Actors need to override this to set their height. The contract is
     * to always resize to the requested size.
     * @param height The new height of the actor.
     */
    virtual auto setHeightImpl(float height) -> void = 0;
};
} // namespace drawing::actors
#endif // RHYTHMGAME_ACTOR_H

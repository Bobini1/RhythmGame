//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_SPLASHSCENE_H
#define RHYTHMGAME_SPLASHSCENE_H
#include "drawing/Scene.h"
#include "resource_managers/LuaScriptFinder.h"
#include "drawing/actors/Actor.h"
#include "drawing/actors/Quad.h"
#include "drawing/actors/VBox.h"
#include "events/Signals2Event.h"
#include <SFML/Graphics/RenderTarget.hpp>
#include <execution>

namespace drawing {
/**
 * @brief Scene that displays the splash screen while the game is being loaded.
 */
class SplashScene : public Scene
{
    // sol::state lua;
    std::shared_ptr<actors::Actor> root;
    events::Signals2Event<> init{};
    mutable bool initialized = false;

  public:
    auto defineEvents(sol::state& target, lua::Bootstrapper& bootstrapper)
      -> void override
    {
        bootstrapper.addEvent(target, init, "init");
    }
    auto setRoot(std::shared_ptr<actors::Actor> newRoot) -> void override
    {
        this->root = std::move(newRoot);
    }
    void update(std::chrono::nanoseconds /* delta */) final {}
    void draw(sf::RenderTarget& target, sf::RenderStates states) const final
    {
        if (root->getIsWidthManaged()) {
            root->setWidth(static_cast<float>(target.getSize().x));
        }
        if (root->getIsHeightManaged()) {
            root->setHeight(static_cast<float>(target.getSize().y));
        }
        root->setTransform(sf::Transform::Identity);

        if (!initialized) {
            init();
            initialized = true;
        }

        target.draw(*root, states);
    }
};
} // namespace drawing

#endif // RHYTHMGAME_SPLASHSCENE_H

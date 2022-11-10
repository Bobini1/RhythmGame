//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_SPLASHSCENE_H
#define RHYTHMGAME_SPLASHSCENE_H
#include "drawing/Scene.h"
#include "resource_locators/LuaScriptFinder.h"
#include "drawing/actors/Actor.h"
#include "drawing/actors/Quad.h"
#include "drawing/actors/VBox.h"
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

  public:
    explicit SplashScene(std::shared_ptr<actors::Actor> root)
        : root(std::move(root))
    {
    }
    void update(std::chrono::nanoseconds /* delta */) final {}
    void draw(sf::RenderTarget& target, sf::RenderStates states) const final
    {
        root->setTransform(sf::Transform::Identity);
        target.draw(*root, states);
    }
};
} // namespace drawing

#endif // RHYTHMGAME_SPLASHSCENE_H

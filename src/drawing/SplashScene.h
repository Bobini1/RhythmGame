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

constexpr auto lua = R"(
    local scene = {}

    function scene:load()
        self.quad = Quad.new()
        self.quad:setFillColor(sf.Color.new(255, 0, 0, 255))
        self.quad:setSize(sf.Vector2f.new(100, 100))
        self:addChild(self.quad)
    end

    function scene:update(delta)
        self.quad:setPosition(sf.Vector2f.new(self.quad:getPosition().x + 1, self.quad:getPosition().y))
    end

    return scene
)";

namespace drawing {
/**
 * @brief Scene that displays the splash screen while the game is being loaded.
 */
template<resource_locators::LuaScriptFinder LuaScriptFinderType>
class SplashScene : public Scene
{
    // sol::state lua;
    std::shared_ptr<actors::Actor> root;

  public:
    explicit SplashScene(LuaScriptFinderType luaScriptFinder)
    {
        // lua.open_libraries(sol::lib::jit, sol::lib::base, sol::lib::io);
        // lua.script(luaScriptFinder.findHandlerScript("splash"));
        auto quad = std::make_shared<actors::Quad>();
        auto quad2 = std::make_shared<actors::Quad>();
        quad2->setFillColor(sf::Color::Red);
        quad2->setSize(sf::Vector2f(100, 100));
        quad->setFillColor(sf::Color::Green);
        quad->setSize({ 100, 100 });
        auto vbox = std::make_shared<actors::VBox>();
        vbox->addChild(quad);
        vbox->addChild(quad2);
        auto vbox2 = std::make_shared<actors::VBox>();
        vbox2->addChild(vbox);
        auto quad3 = std::make_shared<actors::Quad>();
        quad3->setFillColor(sf::Color::Blue);
        quad3->setSize({ 100, 100 });
        vbox2->addChild(quad3);
        root = vbox2;
    }
    void update(std::chrono::nanoseconds /* delta */) final {}
    void draw(sf::RenderTarget& target, sf::RenderStates states) const final
    {
        root->setLayout(sf::FloatRect(0, 0, target.getSize().x, target.getSize().y));
        target.draw(*root, states);
    }
};
} // namespace drawing

#endif // RHYTHMGAME_SPLASHSCENE_H

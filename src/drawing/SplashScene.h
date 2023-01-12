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
#include "events/GlobalEvent.h"
#include "events/MouseClickEvent.h"
#include "events/MouseHoverEvents.h"
#include "drawing/Window.h"
#include <execution>
#include <SFML/Window/Event.hpp>

namespace drawing {
/**
 * @brief Scene that displays the splash screen while the game is being loaded.
 */
template<animations::AnimationPlayer AnimationPlayerType,
         resource_managers::TextureLoader TextureLoaderType,
         resource_managers::FontLoader FontLoaderType>
class SplashScene : public Scene
{
    std::shared_ptr<actors::Actor> root;
    bool initialized = false;
    sol::state state;
    AnimationPlayerType animationPlayer;
    std::shared_ptr<TextureLoaderType> textureLoader;
    std::shared_ptr<FontLoaderType> fontLoader;
    events::GlobalEvent<> init;
    events::GlobalEvent<float> onUpdate;
    events::MouseClickEvent leftClick;
    events::MouseClickEvent rightClick;
    events::MouseHoverEvents mouseHover;

  public:
    explicit SplashScene(sol::state state,
                         AnimationPlayerType animationPlayer,
                         TextureLoaderType* textureLoader,
                         FontLoaderType* fontLoader,
                         const std::filesystem::path& script)
      : state(std::move(state))
      , animationPlayer(std::move(animationPlayer))
      , textureLoader(textureLoader)
      , fontLoader(fontLoader)
      , init(&this->state)
      , onUpdate(&this->state)
      , leftClick(&this->state)
      , rightClick(&this->state)
      , mouseHover(&this->state)
    {
        lua::EventAttacher eventAttacher(&this->state);
        eventAttacher.addEvent(init, "init");
        eventAttacher.addEvent(onUpdate, "update");
        eventAttacher.addEvent(leftClick, "leftClick");
        eventAttacher.addEvent(rightClick, "rightClick");
        eventAttacher.addEvent(mouseHover.onMouseEnter, "mouseEnter");
        eventAttacher.addEvent(mouseHover.onMouseLeave, "mouseLeave");
        lua::defineAllTypes(this->state,
                            *this->textureLoader,
                            *this->fontLoader,
                            this->animationPlayer,
                            eventAttacher);
        auto luaRoot = this->state.script_file(script.string());
        root =
          luaRoot.template get<drawing::actors::Actor*>()->shared_from_this();
    }

    void update(std::chrono::nanoseconds delta, drawing::Window& window) final
    {
        if (!initialized) {
            init();
            initialized = true;
        }
        if (root->getIsWidthManaged()) {
            root->setWidth(static_cast<float>(window.getSize().x));
        }
        if (root->getIsHeightManaged()) {
            root->setHeight(static_cast<float>(window.getSize().y));
        }
        root->setTransform(sf::Transform::Identity);

        onUpdate(static_cast<float>(delta.count()) / 1e9F);
        animationPlayer.update(delta);
        sf::Event event{};
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::MouseButtonPressed:
                    switch (event.mouseButton.button) {
                        case sf::Mouse::Left:
                            leftClick(
                              *root,
                              window.mapPixelToCoords(
                                { event.mouseButton.x, event.mouseButton.y }));
                            break;
                        case sf::Mouse::Right:
                            rightClick(
                              *root,
                              window.mapPixelToCoords(
                                { event.mouseButton.x, event.mouseButton.y }));
                            break;
                        default:
                            break;
                    }
                    break;
                case sf::Event::MouseMoved:
                    mouseHover.update(
                      *root,
                      window.mapPixelToCoords(
                        { event.mouseMove.x, event.mouseMove.y }));
                    break;
                default:
                    break;
            }
        }
    }
    void draw(sf::RenderTarget& target, sf::RenderStates states) const final
    {
        target.draw(*root, states);
    }
};
} // namespace drawing

#endif // RHYTHMGAME_SPLASHSCENE_H

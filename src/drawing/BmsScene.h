//
// Created by bobini on 20.06.23.
//

#ifndef RHYTHMGAME_BMSSCENE_H
#define RHYTHMGAME_BMSSCENE_H

#include <SFML/Graphics/RenderTarget.hpp>
#include <utility>

#include "Scene.h"
#include "events/GlobalEvent.h"
#include "events/MouseClickEvent.h"
#include "events/MouseHoverEvents.h"
#include "input/InputQueue.h"
#include "drawing/Window.h"
#include "charts/gameplay_models/BmsChart.h"
#include "gameplay_logic/BmsGameReferee.h"

namespace drawing {
template<animations::AnimationPlayer AnimationPlayerType,
         resource_managers::TextureLoader TextureLoaderType,
         resource_managers::FontLoader FontLoaderType,
         resource_managers::SoundLoader SoundLoaderType>
class BmsScene : public Scene
{
    std::shared_ptr<actors::Actor> root;
    bool initialized = false;
    sol::state state;
    AnimationPlayerType animationPlayer;
    TextureLoaderType* textureLoader;
    FontLoaderType* fontLoader;
    SoundLoaderType* soundLoader;
    input::InputQueue* inputQueue;
    gameplay_logic::BmsGameReferee referee;
    events::GlobalEvent<> init;
    events::GlobalEvent<std::chrono::nanoseconds> onUpdate;
    events::MouseClickEvent leftClick;
    events::MouseClickEvent rightClick;
    events::MouseHoverEvents mouseHover;

  public:
    void update(std::chrono::nanoseconds delta,
                drawing::Window& window) override
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

        referee.update(delta);

        onUpdate(delta);
        animationPlayer.update(delta);
    }

    explicit BmsScene(sol::state state,
                      AnimationPlayerType animationPlayer,
                      TextureLoaderType* textureLoader,
                      FontLoaderType* fontLoader,
                      SoundLoaderType* soundLoader,
                      input::InputQueue* inputQueue,
                      const std::filesystem::path& script,
                      gameplay_logic::BmsGameReferee referee)
      : state(std::move(state))
      , animationPlayer(std::move(animationPlayer))
      , textureLoader(textureLoader)
      , fontLoader(fontLoader)
      , soundLoader(soundLoader)
      , inputQueue(inputQueue)
      , init(&this->state)
      , onUpdate(&this->state)
      , leftClick(&this->state)
      , rightClick(&this->state)
      , mouseHover(&this->state)
      , referee(std::move(referee))
    {
        auto eventAttacher = lua::EventAttacher{};
        eventAttacher.addEvent(init, "init");
        eventAttacher.addEvent(onUpdate, "update");
        eventAttacher.addEvent(leftClick, "leftClick");
        eventAttacher.addEvent(rightClick, "rightClick");
        eventAttacher.addEvent(mouseHover.onMouseEnter, "mouseEnter");
        eventAttacher.addEvent(mouseHover.onMouseLeave, "mouseLeave");
        lua::defineAllTypes(this->state,
                            *this->textureLoader,
                            *this->fontLoader,
                            *this->soundLoader,
                            this->animationPlayer,
                            eventAttacher);
        auto luaRoot = this->state.script_file(script.string());
        root =
          luaRoot.template get<drawing::actors::Actor*>()->shared_from_this();
    }

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(*root, states);
    }
};
} // namespace drawing
#endif // RHYTHMGAME_BMSSCENE_H

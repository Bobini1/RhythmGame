//
// Created by bobini on 20.06.23.
//

#ifndef RHYTHMGAME_BMSSCENE_H
#define RHYTHMGAME_BMSSCENE_H

#include "Scene.h"
#include "events/GlobalEvent.h"
#include "events/MouseClickEvent.h"
#include "events/MouseHoverEvents.h"
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
    events::GlobalEvent<sol::table> init;
    events::GlobalEvent<float> onUpdate;
    events::MouseClickEvent leftClick;
    events::MouseClickEvent rightClick;
    events::MouseHoverEvents mouseHover;

  public:
    void update(std::chrono::nanoseconds delta,
                drawing::Window& window) override;

    explicit BmsScene(sol::state state,
                      AnimationPlayerType animationPlayer,
                      TextureLoaderType* textureLoader,
                      FontLoaderType* fontLoader,
                      SoundLoaderType* soundLoader,
                      const std::filesystem::path& script)
      : state(std::move(state))
      , animationPlayer(std::move(animationPlayer))
      , textureLoader(textureLoader)
      , fontLoader(fontLoader)
      , soundLoader(soundLoader)
      , init(&this->state)
      , onUpdate(&this->state)
      , leftClick(&this->state)
      , rightClick(&this->state)
      , mouseHover(&this->state)
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
};
} // namespace drawing
#endif // RHYTHMGAME_BMSSCENE_H

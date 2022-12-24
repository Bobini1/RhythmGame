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
#include "events/Event.h"
#include <SFML/Graphics/RenderTarget.hpp>
#include <execution>

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
    mutable bool initialized = false;
    sol::state state;
    AnimationPlayerType animationPlayer;
    std::shared_ptr<TextureLoaderType> textureLoader;
    std::shared_ptr<FontLoaderType> fontLoader;
    events::Event<> init;
    events::Event<float> onUpdate;

  public:
    explicit SplashScene(sol::state state,
                         AnimationPlayerType animationPlayer,
                         std::shared_ptr<TextureLoaderType> textureLoader,
                         std::shared_ptr<FontLoaderType> fontLoader,
                         std::string_view script)
      : state(std::move(state))
      , animationPlayer(std::move(animationPlayer))
      , textureLoader(std::move(textureLoader))
      , fontLoader(std::move(fontLoader))
      , init(&this->state)
      , onUpdate(&this->state)
    {
        lua::EventAttacher eventAttacher(&this->state);
        eventAttacher.addEvent(init, "init");
        eventAttacher.addEvent<decltype(onUpdate), float>(onUpdate, "update");
        lua::defineAllTypes(this->state,
                            *this->textureLoader,
                            *this->fontLoader,
                            this->animationPlayer,
                            eventAttacher);
        auto luaRoot = this->state.script(script);
        root =
          luaRoot.template get<drawing::actors::Actor*>()->shared_from_this();
    }
    void update(std::chrono::nanoseconds delta) final
    {
        if (!initialized) {
            init();
            initialized = true;
        }
        root->setTransform(sf::Transform::Identity);

        onUpdate(static_cast<float>(delta.count()) / 1e9F);
        animationPlayer.update(delta);
    }
    void draw(sf::RenderTarget& target, sf::RenderStates states) const final
    {
        if (root->getIsWidthManaged()) {
            root->setWidth(static_cast<float>(target.getSize().x));
        }
        if (root->getIsHeightManaged()) {
            root->setHeight(static_cast<float>(target.getSize().y));
        }
        target.draw(*root, states);
    }
};
} // namespace drawing

#endif // RHYTHMGAME_SPLASHSCENE_H

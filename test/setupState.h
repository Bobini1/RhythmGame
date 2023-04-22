#ifndef RHYTHMGAME_SETUPSTATE_H
#define RHYTHMGAME_SETUPSTATE_H

#include <SFML/Graphics/Texture.hpp>
#include "lua/Bootstrapper.h"
#include "resource_managers/TextureLoaderImpl.h"
#include "resource_managers/FontLoaderImpl.h"
#include "drawing/animations/AnimationPlayerImpl.h"
#include "resource_managers/FindAssetsFolderBoost.h"
#include "resource_managers/LoadConfig.h"
#include "resource_managers/SoundLoaderImpl.h"

class StateSetup
{
    std::filesystem::path assetsFolder =
      resource_managers::findAssetsFolder() / "themes" / "Default";
    sol::state state;
    lua::EventAttacher eventAttacher;
    resource_managers::TextureLoaderImpl textureLoader;
    resource_managers::FontLoaderImpl fontLoader;
    resource_managers::SoundLoaderImpl soundLoader;
    drawing::animations::AnimationPlayerImpl animationPlayer = {};

  public:
    StateSetup()
      : textureLoader(
          assetsFolder / "textures",
          resource_managers::loadConfig(assetsFolder / "textures" /
                                        "textures.ini")["TextureNames"])
      , fontLoader(assetsFolder / "fonts",
                   resource_managers::loadConfig(assetsFolder / "fonts" /
                                                 "fonts.ini")["FontNames"])
      , soundLoader(assetsFolder / "sounds",
                    resource_managers::loadConfig(assetsFolder / "sounds" /
                                                  "sounds.ini")["SoundNames"])
    {
        state.open_libraries(sol::lib::jit, sol::lib::base, sol::lib::io);
    }

    auto addEventToState(auto& event, std::string name) -> void
    {
        eventAttacher.addEvent(event, std::move(name));
    }

    drawing::animations::AnimationPlayerImpl& getAnimationPlayer()
    {
        return animationPlayer;
    }

    sol::state& getState() { return state; }

    void defineTypes()
    {
        lua::defineAllTypes(state,
                            textureLoader,
                            fontLoader,
                            soundLoader,
                            animationPlayer,
                            eventAttacher);
    }

    explicit operator sol::state() &&
    {
        defineTypes();
        return std::move(state);
    }
};

#endif // RHYTHMGAME_SETUPSTATE_H
//
// Created by bobini on 18.04.23.
//

#include <catch2/catch_test_macros.hpp>
#include "sounds/OpenAlSound.h"
#include "../setupState.h"

static constexpr auto script = R"(
    local sound = Sound.new("click.wav")
    local isLooping = sound.isLooping
    local isPlaying = sound.isPlaying
    local isPaused = sound.isPaused
    local isStopped = sound.isStopped
    local volume = sound.volume
    local rate = sound.rate
    local timePoint = sound.timePoint
    local frequency = sound.frequency
    local duration = sound.duration
    local channels = sound.channels
    return sound
)";

TEST_CASE("Sound properties work", "[lua][sound]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    sounds::OpenALSound result = state.script(script);
}

TEST_CASE("Non-existent sound returns nil", "[lua][sound]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    REQUIRE(state.script("return nil == Sound.new('non-existent.wav')"));
}
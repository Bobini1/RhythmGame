//
// Created by bobini on 24.09.22.
//

#include "events/EventManagerImpl.h"
#include <catch2/catch_all.hpp>

namespace {

class NoArgsListener : public events::Listener
{
  public:
    void onEvent(std::span<std::any> args) override { REQUIRE(args.empty()); }
};

class ThreeArgsListener : public events::Listener
{
  public:
    void onEvent(std::span<std::any> args) override
    {
        using namespace Catch::literals; // NOLINT(google-build-using-namespace)
        REQUIRE(args.size() == 3);
        REQUIRE(std::any_cast<int>(args[0]) == 1);
        REQUIRE(std::any_cast<float>(args[1]) == 2.0_a);
        REQUIRE(std::any_cast<std::string>(args[2]) == "3");
    }
};

} // namespace

TEST_CASE("EventManager can add and remove listeners", "[events]")
{
    events::EventManagerImpl eventManager;
    auto listener = std::make_shared<NoArgsListener>();
    eventManager.connect(events::Event::Init, listener);
    eventManager.disconnect(events::Event::Init, listener);
}

TEST_CASE("EventManager can call listeners", "[events]")
{
    events::EventManagerImpl eventManager;
    auto listener = std::make_shared<NoArgsListener>();
    eventManager.connect(events::Event::Init, listener);
    eventManager.call(events::Event::Init, {});
}

TEST_CASE("EventManager can call listeners with arguments", "[events]")
{
    events::EventManagerImpl eventManager;
    auto listener = std::make_shared<ThreeArgsListener>();
    eventManager.connect(events::Event::Init, listener);
    auto args = std::vector{ std::any{ 1 },
                             std::any{ 2.0F },
                             std::any{ std::string{ "3" } } };
    eventManager.call(events::Event::Init, args);
}

TEST_CASE("EventManager can call multiple listeners", "[events]")
{
    events::EventManagerImpl eventManager;
    auto listener1 = std::make_shared<NoArgsListener>();
    auto listener2 = std::make_shared<NoArgsListener>();
    eventManager.connect(events::Event::Init, listener1);
    eventManager.connect(events::Event::Init, listener2);
    eventManager.call(events::Event::Init, {});
}

TEST_CASE("EventManager can call multiple listeners with arguments", "[events]")
{
    events::EventManagerImpl eventManager;
    auto listener1 = std::make_shared<ThreeArgsListener>();
    auto listener2 = std::make_shared<ThreeArgsListener>();
    eventManager.connect(events::Event::Init, listener1);
    eventManager.connect(events::Event::Init, listener2);
    auto args = std::vector{ std::any{ 1 },
                             std::any{ 2.0F },
                             std::any{ std::string{ "3" } } };
    eventManager.call(events::Event::Init, args);
}

TEST_CASE("Converting from string to enum", "[events]")
{
    REQUIRE(events::enumToEventName(events::Event::Init) == "Init");
    REQUIRE(events::enumToEventName(events::Event::Exit) == "Exit");
    REQUIRE(events::enumToEventName(events::Event::Update) == "Update");
    REQUIRE(events::enumToEventName(events::Event::Draw) == "Draw");
    REQUIRE(events::enumToEventName(events::Event::KeyPressed) == "KeyPressed");
    REQUIRE(events::enumToEventName(events::Event::KeyReleased) ==
            "KeyReleased");
    REQUIRE(events::enumToEventName(events::Event::MouseMoved) == "MouseMoved");
    REQUIRE(events::enumToEventName(events::Event::MouseButtonPressed) ==
            "MouseButtonPressed");
    REQUIRE(events::enumToEventName(events::Event::MouseButtonReleased) ==
            "MouseButtonReleased");
    REQUIRE(events::enumToEventName(events::Event::MouseWheelScrolled) ==
            "MouseWheelScrolled");
}

TEST_CASE("Converting from enum to string", "[events]")
{
    REQUIRE(events::eventNameToEnum("Init") == events::Event::Init);
    REQUIRE(events::eventNameToEnum("Exit") == events::Event::Exit);
    REQUIRE(events::eventNameToEnum("Update") == events::Event::Update);
    REQUIRE(events::eventNameToEnum("Draw") == events::Event::Draw);
    REQUIRE(events::eventNameToEnum("KeyPressed") == events::Event::KeyPressed);
    REQUIRE(events::eventNameToEnum("KeyReleased") ==
            events::Event::KeyReleased);
    REQUIRE(events::eventNameToEnum("MouseMoved") == events::Event::MouseMoved);
    REQUIRE(events::eventNameToEnum("MouseButtonPressed") ==
            events::Event::MouseButtonPressed);
    REQUIRE(events::eventNameToEnum("MouseButtonReleased") ==
            events::Event::MouseButtonReleased);
    REQUIRE(events::eventNameToEnum("MouseWheelScrolled") ==
            events::Event::MouseWheelScrolled);
}
#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-named-parameter"
#pragma ide diagnostic ignored "readability-identifier-naming"
//
// Created by bobini on 26.12.22.
//

#ifndef RHYTHMGAME_GETPUSHLUASPECIALIZATIONS_H
#define RHYTHMGAME_GETPUSHLUASPECIALIZATIONS_H
#include <sol/sol.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <chrono>

namespace sol {

inline auto
sol_lua_get(sol::types<sf::Color>,
            lua_State* L,
            int index,
            sol::stack::record& tracking) -> sf::Color
{
    const int absoluteIndex = lua_absindex(L, index);

    const sol::table table = sol::stack::get<sol::table>(L, absoluteIndex);
    const auto r =
      table.get_or("r", table.get_or(1, static_cast<sf::Uint8>(0)));
    const auto g =
      table.get_or("g", table.get_or(2, static_cast<sf::Uint8>(0)));
    const auto b =
      table.get_or("b", table.get_or(3, static_cast<sf::Uint8>(0)));
    const auto a = table.get_or(
      "a",
      table.get_or(
        4, static_cast<sf::Uint8>(std::numeric_limits<sf::Uint8>::max())));

    tracking.use(1);

    return { r, g, b, a };
}

inline auto
sol_lua_push(lua_State* L, const sf::Color& color) -> int
{
    lua_createtable(L, 0, 4);
    lua_getglobal(L, "Color");
    lua_setmetatable(L, -2);

    sol::stack_table table(L);
    table["r"] = color.r;
    table["g"] = color.g;
    table["b"] = color.b;
    table["a"] = color.a;

    return 1;
}

template<>
struct lua_type_of<sf::Color>
  : std::integral_constant<sol::type, sol::type::table>
{
};

template<typename Handler>
inline auto
sol_lua_check(sol::types<sf::Color>,
              lua_State* L,
              int index,
              Handler&& handler,
              sol::stack::record& tracking) -> bool
{
    int absoluteIndex = lua_absindex(L, index);
    if (!stack::check<sol::table>(L, absoluteIndex, handler)) {
        tracking.use(1);
        return false;
    }

    tracking.use(1);
    return true;
}

inline auto
sol_lua_get(sol::types<sf::Vector2f>,
            lua_State* L,
            int index,
            sol::stack::record& tracking) -> sf::Vector2f
{
    const int absoluteIndex = lua_absindex(L, index);

    const sol::table table = sol::stack::get<sol::table>(L, absoluteIndex);
    const auto x = table.get_or("x", table.get_or(1, static_cast<float>(0)));
    const auto y = table.get_or("y", table.get_or(2, static_cast<float>(0)));

    tracking.use(1);

    return { x, y };
}

inline auto
sol_lua_push(lua_State* L, const sf::Vector2f& vector) -> int
{
    lua_createtable(L, 0, 4);
    lua_getglobal(L, "Vector2");
    lua_setmetatable(L, -2);

    sol::stack_table table(L);
    table["x"] = vector.x;
    table["y"] = vector.y;

    return 1;
}

template<>
struct lua_type_of<sf::Vector2f>
  : std::integral_constant<sol::type, sol::type::table>
{
};

template<typename Handler>
inline auto
sol_lua_check(sol::types<sf::Vector2f>,
              lua_State* L,
              int index,
              Handler&& handler,
              sol::stack::record& tracking) -> bool
{
    int absoluteIndex = lua_absindex(L, index);
    if (!stack::check<sol::table>(L, absoluteIndex, handler)) {
        tracking.use(1);
        return false;
    }

    tracking.use(1);
    return true;
}

inline auto
sol_lua_get(sol::types<std::chrono::nanoseconds>,
            lua_State* L,
            int index,
            sol::stack::record& tracking) -> std::chrono::nanoseconds
{
    const int absoluteIndex = lua_absindex(L, index);

    const float duration = sol::stack::get<float>(L, absoluteIndex);

    constexpr auto secondsToNanos = 1E9;

    auto nanos =
      std::chrono::nanoseconds{ static_cast<int64_t>(duration * secondsToNanos) };

    tracking.use(1);

    return nanos;
}

inline auto
sol_lua_push(lua_State* L, const std::chrono::nanoseconds& nanos) -> int
{
    constexpr auto nanosToSeconds = 1E-9;
    const double duration = static_cast<double>(nanos.count()) * nanosToSeconds;
    sol::stack::push(L, duration);

    return 1;
}

template<>
struct lua_type_of<std::chrono::nanoseconds>
  : std::integral_constant<sol::type, sol::type::number>
{
};

template<typename Handler>
inline auto
sol_lua_check(sol::types<std::chrono::nanoseconds>,
              lua_State* L,
              int index,
              Handler&& handler,
              sol::stack::record& tracking) -> bool
{
    int absoluteIndex = lua_absindex(L, index);
    if (!stack::check<float>(L, absoluteIndex, handler)) {
        tracking.use(1);
        return false;
    }

    tracking.use(1);
    return true;
}

} // namespace sol

#endif // RHYTHMGAME_GETPUSHLUASPECIALIZATIONS_H

#pragma clang diagnostic pop
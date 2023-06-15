//
// Created by bobini on 14.06.23.
//

#ifndef RHYTHMGAME_GET_H
#define RHYTHMGAME_GET_H

#include <boost/pfr/tuple_size.hpp>
#include <boost/pfr/core.hpp>

namespace support {
template<std::size_t N, typename T>
constexpr auto
get(T&& t) -> auto&
    requires requires { std::get<N>(std::declval<T>()); }
{
    return std::get<N>(t);
}

template<std::size_t N, typename T>
constexpr auto
get(T&& aggregate) -> auto&
    requires std::is_aggregate_v<std::remove_cvref_t<T>>
{
    return boost::pfr::get<N>(aggregate);
}

} // namespace support

#endif // RHYTHMGAME_GET_H

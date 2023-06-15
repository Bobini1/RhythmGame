//
// Created by bobini on 14.06.23.
//

#ifndef RHYTHMGAME_GET_H
#define RHYTHMGAME_GET_H

#include <boost/pfr/tuple_size.hpp>
#include <boost/pfr/core.hpp>

namespace support {
/**
 * @brief Returns the nth element of a type that supports std::get.
 * @tparam N Index of the element to get.
 * @tparam T Type of the object to get the element from.
 * @param t Object to get the element from.
 * @return A reference to nth element of the object.
 */
template<std::size_t N, typename T>
constexpr auto
get(T&& t) -> auto&
    requires requires { std::get<N>(std::declval<T>()); }
{
    return std::get<N>(t);
}

/**
 * @brief Returns the nth member of an aggregate.
 * @tparam N Index of the element to get.
 * @tparam T Type of the aggregate.
 * @param aggregate Aggregate to get the element from.
 * @return A reference to nth member of the aggregate.
 */
template<std::size_t N, typename T>
constexpr auto
get(T&& aggregate) -> auto&
    requires std::is_aggregate_v<std::remove_cvref_t<T>>
{
    return boost::pfr::get<N>(aggregate);
}

} // namespace support

#endif // RHYTHMGAME_GET_H

//
// Created by bobini on 14.06.23.
//

#ifndef RHYTHMGAME_TUPLESIZE_H
#define RHYTHMGAME_TUPLESIZE_H

#include <boost/pfr/tuple_size.hpp>
namespace support {

/**
 * @brief Used to get the size of a tuple or an aggregate.
 * @tparam T Type of the tuple or aggregate to check.
 */
template<typename T>
    requires requires {
        {
            std::tuple_size<T>::value
        };
    } || std::is_aggregate_v<T>
class TupleSize
{
  static constexpr auto tupleSizeImpl() -> std::size_t
    {
        if constexpr (requires {
                          {
                              std::tuple_size<T>::value
                          };
                      }) {
            return std::tuple_size<T>::value;
        } else {
            return boost::pfr::tuple_size<T>::value;
        }
    }
public:
    static constexpr auto
    value = tupleSizeImpl();
};

template<typename T>
constexpr auto tupleSizeV = TupleSize<T>::value;

} // namespace support

#endif // RHYTHMGAME_TUPLESIZE_H

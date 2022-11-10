//
// Created by bobini on 15.10.22.
//

#ifndef RHYTHMGAME_ENABLESHAREDFROMBASE_H
#define RHYTHMGAME_ENABLESHAREDFROMBASE_H
#include <memory>

/**
 * @namespace support
 * @brief Contains various helper classes and functions
 */
namespace support {
/**
 * @class EnableSharedFromBase
 * @brief A helper class for creating a shared pointer to self
 * @tparam T The type of the base class that inherits from this class (CRTP)
 * The classic std::enable_shared_from_this<T> can be used to create a
 * shared_ptr to self. However, derived classes would need to cast that
 * shared_ptr to their own type to use it. This class wraps that casting.
 */
template<class Base>
class EnableSharedFromBase : public std::enable_shared_from_this<Base>
{
  protected:
    template<class Derived>
    auto sharedFromBase() -> std::shared_ptr<Derived>
    {
        return std::static_pointer_cast<Derived>(this->shared_from_this());
    }
};

} // namespace support
#endif // RHYTHMGAME_ENABLESHAREDFROMBASE_H

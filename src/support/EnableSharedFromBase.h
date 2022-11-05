//
// Created by bobini on 15.10.22.
//

#ifndef RHYTHMGAME_ENABLESHAREDFROMBASE_H
#define RHYTHMGAME_ENABLESHAREDFROMBASE_H
#include <memory>

namespace support {
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

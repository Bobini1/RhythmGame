//
// Created by bobini on 20.11.22.
//

#ifndef RHYTHMGAME_CONNECTION_H
#define RHYTHMGAME_CONNECTION_H

#include <any>
#include "support/FunctionReference.h"
namespace events {
/**
 * @brief A scoped connection object
 */
class Connection
{
  public:
    virtual ~Connection() = default;

    [[nodiscard]] virtual auto getFunctionReference() const
      -> support::FunctionReference = 0;
};
} // namespace events
#endif // RHYTHMGAME_CONNECTION_H

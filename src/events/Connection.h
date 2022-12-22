//
// Created by bobini on 20.11.22.
//

#ifndef RHYTHMGAME_CONNECTION_H
#define RHYTHMGAME_CONNECTION_H

namespace events {
/**
 * @brief A scoped connection object
 */
class Connection
{
  public:
    virtual ~Connection() = default;
};
} // namespace events
#endif // RHYTHMGAME_CONNECTION_H

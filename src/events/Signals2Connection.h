//
// Created by bobini on 20.11.22.
//

#ifndef RHYTHMGAME_SIGNALS2CONNECTION_H
#define RHYTHMGAME_SIGNALS2CONNECTION_H

#include <boost/signals2/connection.hpp>
#include "Connection.h"
namespace events {
class Signals2Connection : public Connection
{
  public:
    explicit Signals2Connection(boost::signals2::scoped_connection connection);

  private:
    boost::signals2::scoped_connection connection;
};
} // namespace events
#endif // RHYTHMGAME_SIGNALS2CONNECTION_H

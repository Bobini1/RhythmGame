//
// Created by bobini on 20.11.22.
//

#include "Signals2Connection.h"
events::Signals2Connection::Signals2Connection(
  boost::signals2::scoped_connection connection)
  : connection{ std::move(connection) }
{
}

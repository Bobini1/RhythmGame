//
// Created by bobini on 20.11.22.
//

#include "Signals2Connection.h"
events::Signals2Connection::Signals2Connection(
  boost::signals2::scoped_connection connection,
  support::FunctionReference function)
  : connection{ std::move(connection) }
  , function{ std::move(function) }
{
}
auto
events::Signals2Connection::getFunctionReference() const
  -> support::FunctionReference
{
    return function;
}
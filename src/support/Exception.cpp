//
// Created by bobini on 23.04.24.
//

#include "Exception.h"

namespace support {
Exception::Exception(std::string message)
  : message(std::move(message))
{
}
const char*
Exception::what() const noexcept
{
    return message.c_str();
}
} // namespace support
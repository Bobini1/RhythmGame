//
// Created by bobini on 23.04.24.
//

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <string>
#include <utility>

namespace support {

class Exception : public std::exception
{
    std::string message;

  public:
    explicit Exception(std::string message);

    auto what() const noexcept -> const char* override;
};

} // namespace support

#endif // EXCEPTION_H

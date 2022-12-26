//
// Created by bobini on 26.12.22.
//

#include <locale>
#include <string>
#include "Capitalize.h"

auto
support::capitalize(std::string_view str) -> std::string
{
    if (str.empty()) {
        return {};
    }
    std::string result;
    result.reserve(str.size());
    result.push_back(
      static_cast<char>(std::toupper(str.front()))); // possibly unsafe
    result.append(str.substr(1));
    return result;
}
//
// Created by bobini on 19.07.2022.
//

#ifndef RHYTHMGAME_TOCHARS_H
#define RHYTHMGAME_TOCHARS_H
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>

template<unsigned int N>
constexpr auto
getCh(char const (&str)[N], unsigned int index) -> char
{
    return index >= N ? '\0' : str[index];
}

#define STRING_TO_CHARS_EXTRACT(z, n, data) BOOST_PP_COMMA_IF(n) getCh(data, n)

#define STRING_TO_CHARS(STRLEN, STR)                                           \
    BOOST_PP_REPEAT(STRLEN, STRING_TO_CHARS_EXTRACT, STR)

#define RHYTHMGAME_TO_CHARS(STRLEN, STR) STRING_TO_CHARS(STRLEN, STR)

#endif // RHYTHMGAME_TOCHARS_H

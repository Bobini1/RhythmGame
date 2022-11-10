//
// Created by bobini on 11/9/22.
//

#ifndef RHYTHMGAME_LUAOVERLOADFORALLACTORS_H
#define RHYTHMGAME_LUAOVERLOADFORALLACTORS_H

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/push_front.hpp>
#include <boost/preprocessor/array/size.hpp>
#include <boost/preprocessor/variadic/to_array.hpp>
#include <boost/preprocessor/seq/rest_n.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/arithmetic.hpp>
#include <boost/preprocessor/punctuation.hpp>
#include <boost/preprocessor/logical.hpp>
#include <boost/preprocessor/inc.hpp>
#include <boost/preprocessor/seq/to_list.hpp>
#include <boost/preprocessor/list/at.hpp>
#include <boost/preprocessor/comparison/not_equal.hpp>
#include <boost/preprocessor/list/size.hpp>
#include <boost/preprocessor/facilities/is_empty_variadic.hpp>
#include <boost/preprocessor/list/for_each_i.hpp>
#include <utility>
#include <functional>


#include "sol/overload.hpp"
#include "drawing/actors/Quad.h"
#include "drawing/actors/VBox.h"
#include "drawing/actors/HBox.h"

#define DEF_TYPES (drawing::actors::Quad)(drawing::actors::VBox)(drawing::actors::HBox)

#define DEF_LOG_COMMA_BEFORE_PAIR(i) \
  BOOST_PP_COMMA_IF(BOOST_PP_NOT(BOOST_PP_MOD(i,2)))

#define DEF_LOG_EXPAND_PARAM(r, data, i, elem) \
  DEF_LOG_COMMA_BEFORE_PAIR(i) elem

#define DEF_LOG_EXPAND_CALL(r, data, i, elem) \
  DEF_LOG_COMMA_BEFORE_PAIR(i) \
  BOOST_PP_IF( BOOST_PP_MOD(i,2), \
    (elem), \
    std::forward<elem> )
#define DEF_MAKE_LAMBDA(r, FUNCTION_AND_ARGS, i, CLASS) \
  [fun = BOOST_PP_LIST_AT(FUNCTION_AND_ARGS, 1)](std::shared_ptr<CLASS> self BOOST_PP_LIST_FOR_EACH_I(DEF_LOG_EXPAND_PARAM, ~, BOOST_PP_LIST_REST_N(2, FUNCTION_AND_ARGS))) \
  {                                           \
       return std::invoke(fun, std::move(self) BOOST_PP_LIST_FOR_EACH_I(DEF_LOG_EXPAND_CALL, ~, BOOST_PP_LIST_REST_N(2, FUNCTION_AND_ARGS)));\
  } BOOST_PP_COMMA_IF(BOOST_PP_NOT_EQUAL(BOOST_PP_DEC(BOOST_PP_LIST_FIRST(FUNCTION_AND_ARGS)), i))\


#define DEF_OVERLOAD_FUNC(TYPES, ...) \
sol::overload(               \
    BOOST_PP_SEQ_FOR_EACH_I(DEF_MAKE_LAMBDA, BOOST_PP_SEQ_TO_LIST(BOOST_PP_SEQ_PUSH_FRONT(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), BOOST_PP_SEQ_SIZE(TYPES)))\
, TYPES)) \


#endif // RHYTHMGAME_LUAOVERLOADFORALLACTORS_H

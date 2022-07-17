//
// Created by bobini on 07.07.2022.
//

#include "BmsChartReader.h"
#include <tao/pegtl.hpp>
#include <string>

namespace {

} // namespace

namespace charts::chart_readers::bms {
namespace {
namespace pegtl = tao::pegtl;
// clang-format off
// double

struct plus_minus : pegtl::opt< pegtl::one< '+', '-' > > {};
struct dot : pegtl::one< '.' > {};

struct inf : pegtl::seq< pegtl::istring< 'i', 'n', 'f' >,
                        pegtl::opt< pegtl::istring< 'i', 'n', 'i', 't', 'y' > > > {};

struct nan : pegtl::seq< pegtl::istring< 'n', 'a', 'n' >,
                        pegtl::opt< pegtl::one< '(' >,
                                   pegtl::plus< pegtl::alnum >,
                                   pegtl::one< ')' > > > {};

template< typename D >
struct number : pegtl::if_then_else< dot,
                             pegtl::plus< D >,
                                    pegtl::seq< pegtl::plus< D >, pegtl::opt< dot, pegtl::star< D > > > > {};

struct e : pegtl::one< 'e', 'E' > {};
struct p : pegtl::one< 'p', 'P' > {};
struct f : pegtl::one< 'f', 'F' > {};
struct exponent : pegtl::seq< plus_minus, pegtl::plus< pegtl::digit > > {};

struct decimal : pegtl::seq< number< pegtl::digit >, pegtl::opt< e, exponent > > {};
struct hexadecimal : pegtl::seq< pegtl::one< '0' >, pegtl::one< 'x', 'X' >, number< pegtl::xdigit >, pegtl::opt< p, exponent > > {};

struct floating : pegtl::seq< plus_minus, pegtl::sor< hexadecimal, decimal, inf, nan, pegtl::opt<f> > > {};

// double end

struct metaString : pegtl::plus<pegtl::utf8::any> {};

template<typename AllowedValue, typename TagName>
struct metaTag : pegtl::seq<pegtl::bol, pegtl::one<'#'>, TagName, pegtl::one<' '>, AllowedValue, pegtl::eolf> {};

template<typename TagName>
struct metaStringTag: metaTag<metaString, TagName> {};

struct playerValidDigit : pegtl::range<'1', '4'> {};
struct player : metaTag<playerValidDigit, pegtl::istring<'P', 'L', 'A', 'Y', 'E', 'R'>> {};
struct bpm : metaTag<floating, pegtl::istring<'B', 'P', 'M'>> {};
struct genre : metaStringTag< pegtl::istring<'G', 'E', 'N', 'R', 'E'>> {};
struct artist : metaStringTag< pegtl::istring<'A', 'R', 'T', 'I', 'S', 'T'>> {};
struct title : metaStringTag< pegtl::istring<'T', 'I', 'T', 'L', 'E'>> {};
struct subArtist : metaStringTag< pegtl::istring<'S', 'U', 'B', 'A', 'R', 'T', 'I', 'S', 'T'>> {};
struct subTitle : metaStringTag< pegtl::istring<'S', 'U', 'B', 'T', 'I', 'T', 'L', 'E'>> {};

struct filename : pegtl::plus<pegtl::utf8::any> {};

struct wavXX : metaTag<filename, pegtl::seq<pegtl::istring<'W', 'A', 'V'>, pegtl::rep<2, pegtl::alnum>>> {};

// clang-format on

} // namespace
auto
BmsChartReader::readBmsChart(std::string& chart) -> models::Chart
{
    using namespace std::string_literals;
    using namespace std::chrono_literals;
    return { "", "", "", BmsMeta{ "", "", "" } };
};
} // namespace charts::chart_readers::bms

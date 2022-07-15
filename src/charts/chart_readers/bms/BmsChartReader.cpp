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

struct Floating : pegtl::seq< plus_minus, pegtl::sor< hexadecimal, decimal, inf, nan, pegtl::opt<f> > > {};

// double end

using MetaString = pegtl::plus<pegtl::utf8::any>;

template<typename AllowedValue, typename TagName>
struct MetaTag : pegtl::seq<pegtl::bol, pegtl::one<'#'>, TagName, pegtl::one<' '>, AllowedValue, pegtl::eolf> {};

template<typename TagName>
struct MetaStringTag: MetaTag<MetaString, TagName> {};

struct PlayerValidDigit : pegtl::range<'1', '4'> {};
struct Player : MetaTag<PlayerValidDigit, pegtl::istring<'P', 'L', 'A', 'Y', 'E', 'R'>> {};
struct Bpm : MetaTag<Floating, pegtl::istring<'B', 'P', 'M'>> {};
struct Genre : MetaStringTag< pegtl::istring<'G', 'E', 'N', 'R', 'E'>> {};
struct Artist : MetaStringTag< pegtl::istring<'A', 'R', 'T', 'I', 'S', 'T'>> {};
struct Title : MetaStringTag< pegtl::istring<'T', 'I', 'T', 'L', 'E'>> {};
struct SubArtist : MetaStringTag< pegtl::istring<'S', 'U', 'B', 'A', 'R', 'T', 'I', 'S', 'T'>> {};
struct SubTitle : MetaStringTag< pegtl::istring<'S', 'U', 'B', 'T', 'I', 'T', 'L', 'E'>> {};

struct Filename : pegtl::plus<pegtl::utf8::any> {};

struct WavXX : MetaTag<Filename, pegtl::seq<pegtl::istring<'W', 'A', 'V'>, pegtl::rep<2, pegtl::alnum>>> {};

// clang-format on

} // namespace
auto
BmsChartReader::readBmsChart(std::string& chart) -> models::bms::BmsChart
{
    using namespace std::string_literals;
    using namespace std::chrono_literals;
    return models::bms::BmsChart{ models::ChartInfo{
      models::Title{ "" },
      models::Artist{ "" },
      models::Genre{ "" },
      models::ChartType{ "BMS" },
      models::Difficulty{ "" },
      models::Level{ 0 },
      models::NoteCount{ 0U },
      boost::icl::interval_map<models::Offset, models::Bpm>{},
      models::Offset{ 0s } } };
};
} // namespace charts::chart_readers::bms

//
// Created by bobini on 07.07.2022.
//

#include "BmsChartReader.h"
#include <string>
#include <map>
#include "charts/models/BmsMeta.h"
#include <random>
#include <utility>
#include "charts/chart_readers/ToChars.h"
#include <iostream>
#include <tao/pegtl.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace charts::chart_readers {
namespace pegtl = tao::pegtl;

template<typename>
struct action
{
};

using RandomRange = std::uniform_int_distribution<long>;
using IfBlock = long;

struct tags
{
    std::optional<std::string> title;
    std::optional<std::string> artist;
    std::optional<double> bpm;
    std::optional<std::string> subTitle;
    std::optional<std::string> subArtist;
    std::optional<std::string> genre;
};
// double

struct plusMinus : pegtl::opt<pegtl::one<'+', '-'>>
{
};
struct dot : pegtl::one<'.'>
{
};

struct inf
  : pegtl::seq<pegtl::istring<'i', 'n', 'f'>,
               pegtl::opt<pegtl::istring<'i', 'n', 'i', 't', 'y'>>>
{
};

struct nan
  : pegtl::seq<
      pegtl::istring<'n', 'a', 'n'>,
      pegtl::opt<pegtl::one<'('>, pegtl::plus<pegtl::alnum>, pegtl::one<')'>>>
{
};

template<typename D>
struct number
  : pegtl::if_then_else<
      dot,
      pegtl::plus<D>,
      pegtl::seq<pegtl::plus<D>, pegtl::opt<dot, pegtl::star<D>>>>
{
};

struct e : pegtl::one<'e', 'E'>
{
};
struct p : pegtl::one<'p', 'P'>
{
};
struct f : pegtl::one<'f', 'F'>
{
};
struct d : pegtl::one<'d', 'D'>
{
};
struct exponent : pegtl::seq<plusMinus, pegtl::plus<pegtl::digit>>
{
};

struct decimal : pegtl::seq<number<pegtl::digit>, pegtl::opt<e, exponent>>
{
};
struct hexadecimal
  : pegtl::seq<pegtl::one<'0'>,
               pegtl::one<'x', 'X'>,
               number<pegtl::xdigit>,
               pegtl::opt<p, exponent>>
{
};

struct floating
  : pegtl::seq<
      plusMinus,
      pegtl::sor<hexadecimal, decimal, inf, nan, pegtl::opt<pegtl::sor<f, d>>>>
{
};

// double end

struct spacesUntilEndOfLine
  : pegtl::seq<pegtl::star<pegtl::minus<pegtl::space, pegtl::eolf>>,
               pegtl::at<pegtl::eolf>>
{
};

struct metaString : pegtl::until<spacesUntilEndOfLine>
{
};

template<typename AllowedValue, typename TagName>
struct metaTag
  : pegtl::seq<pegtl::star<pegtl::space>,
               pegtl::one<'#'>,
               TagName,
               pegtl::star<pegtl::space>,
               AllowedValue,
               pegtl::star<pegtl::minus<pegtl::space, pegtl::eolf>>>
{
};

template<typename TagName>
struct blockTag
  : pegtl::seq<pegtl::bol,
               pegtl::one<'#'>,
               TagName,
               pegtl::star<pegtl::space>,
               pegtl::eolf>
{
};

template<typename TagName>
struct metaStringTag : metaTag<metaString, TagName>
{
};

struct playerValidDigit : pegtl::range<'1', '4'>
{
};
struct filename : pegtl::plus<pegtl::utf8::any>
{
};

struct wavXX
  : metaTag<
      filename,
      pegtl::seq<pegtl::istring<'W', 'A', 'V'>, pegtl::rep<2, pegtl::alnum>>>
{
};

// struct ifBlock : pegtl::seq<blockTag<pegtl::istring<'I', 'F'>>,
//                      pegtl::list<pegtl::sor<commonTag, randomBlock>,
//                      pegtl::eolf>,
//                  pegtl::sor<pegtl::eof, blockTag<pegtl::istring<'E', 'N',
//                  'D', 'I', 'F'>>>> {};
//
//
// struct randomBlock : pegtl::seq<blockTag<pegtl::istring<'R', 'A', 'N', 'D',
// 'O', 'M'>>,
//                         pegtl::list<pegtl::sor<commonTag, ifBlock>,
//                         pegtl::eolf>, pegtl::sor<pegtl::eof,
//                      blockTag<pegtl::istring<'E', 'N', 'D', 'R', 'A', 'N',
//                      'D', 'O', 'M'>>>> {};

#define RHYTHMGAME_TAG_PARSER(                                                 \
  tag, tagstrlen, allowedValue, memberPointer, parser)                         \
    struct tag##_allowedValue : allowedValue                                   \
    {                                                                          \
    };                                                                         \
                                                                               \
    struct tag                                                                 \
      : metaTag<tag##_allowedValue,                                            \
                pegtl::istring<RHYTHMGAME_TO_CHARS(tagstrlen, #tag)>>          \
    {                                                                          \
    };                                                                         \
                                                                               \
    template<>                                                                 \
    struct action<tag##_allowedValue>                                          \
    {                                                                          \
                                                                               \
        template<typename ActionInput>                                         \
        static auto apply(const ActionInput& input, tags& chart) -> void       \
        {                                                                      \
            chart.*memberPointer = parser(input.string());                     \
        }                                                                      \
    };

namespace {
constexpr auto identity = std::identity();
constexpr auto trimR = [](auto&& str) {
    return std::string_view{ str }.substr(0, str.find_last_not_of(' ') + 1);
};
} // namespace

RHYTHMGAME_TAG_PARSER(title, 5, metaString, &tags::title, trimR);
RHYTHMGAME_TAG_PARSER(artist, 6, metaString, &tags::artist, trimR);
RHYTHMGAME_TAG_PARSER(subArtist, 9, metaString, &tags::subArtist, trimR);
RHYTHMGAME_TAG_PARSER(subTitle, 8, metaString, &tags::subTitle, trimR);
RHYTHMGAME_TAG_PARSER(bpm, 3, floating, &tags::bpm, std::stod);
RHYTHMGAME_TAG_PARSER(genre, 5, metaString, &tags::genre, trimR);

struct commonTag : pegtl::sor<title, subTitle, artist, subArtist, bpm, genre>
{
};

struct file : pegtl::list<commonTag, pegtl::plus<pegtl::space>>
{
};

auto
BmsChartReader::readBmsChart(std::string chart) const -> charts::models::Chart
{
    using namespace std::string_literals;
    using namespace std::chrono_literals;

    auto input = pegtl::string_input<>(std::move(chart), "BMS Chart"s);
    auto tagsInput = tags{};
    auto parsed = pegtl::parse<file, action>(input, tagsInput);
    auto chartRes = charts::models::Chart{
        tagsInput.title.has_value() ? tagsInput.title.value() : "Untitled",
        tagsInput.artist.has_value() ? tagsInput.artist.value() : "Unknown",
        tagsInput.bpm.has_value() ? tagsInput.bpm.value() : 0.0,
        BmsMeta{ tagsInput.genre, tagsInput.subTitle, tagsInput.subArtist }
    };

    return chartRes;
}
} // namespace charts::chart_readers

//
// Created by bobini on 07.07.2022.
//

#include <string>
#include <map>
#include "charts/models/BmsMeta.h"
#include <random>
#include <utility>
#include "charts/chart_readers/ToChars.h"
#include <tao/pegtl.hpp>
#include <stack>
#include "BmsChartReader.h"

#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))

namespace charts::chart_readers {
namespace pegtl = tao::pegtl;

class TagsWriter
{
    std::stack<std::pair<IfTag, tags>> ifStack;
    std::stack<std::pair<RandomRange, std::vector<std::pair<IfTag, tags>>>>
      randomStack;

  public:
    TagsWriter() { ifStack.push(std::make_pair(0, tags{})); }
    void setTitle(std::string title)
    {
        ifStack.top().second.title = std::move(title);
    }
    void setArtist(std::string artist)
    {
        ifStack.top().second.artist = std::move(artist);
    }
    void setBpm(double bpm) { ifStack.top().second.bpm = bpm; }
    void setSubTitle(std::string subTitle)
    {
        ifStack.top().second.subTitle = std::move(subTitle);
    }
    void setSubArtist(std::string subArtist)
    {
        ifStack.top().second.subArtist = std::move(subArtist);
    }
    void setGenre(std::string genre)
    {
        ifStack.top().second.genre = std::move(genre);
    }

    void enterIf(IfTag tag) { ifStack.push(std::make_pair(tag, tags{})); }

    void leaveIf()
    {
        randomStack.top().second.emplace_back(ifStack.top().first,
                                              std::move(ifStack.top().second));
        ifStack.pop();
    }

    void enterRandom(RandomRange range)
    {
        randomStack.push(
          std::make_pair(range, std::vector<std::pair<IfTag, tags>>{}));
    }

    void leaveRandom()
    {
        auto ifs = std::move(randomStack.top().second);
        auto randomDistribution = randomStack.top().first;
        randomStack.pop();
        auto ifsMap = std::multimap<IfTag, tags>{};
        for (auto& ifTag : ifs) {
            ifsMap.emplace(ifTag.first, std::move(ifTag.second));
        }
        ifStack.top().second.randomBlocks.emplace_back(std::make_pair(
          randomDistribution,
          std::make_unique<std::multimap<IfTag, tags>>(std::move(ifsMap))));
    }

    auto getTags() -> tags& { return ifStack.top().second; }
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
struct noValueTag
  : pegtl::seq<pegtl::star<pegtl::space>,
               pegtl::one<'#'>,
               TagName,
               pegtl::star<pegtl::minus<pegtl::space, pegtl::eolf>>>
{
};

struct playerValidDigit : pegtl::range<'1', '4'>
{
};

template<typename>
struct action
{
};

#define RHYTHMGAME_TAG_PARSER(                                                 \
  tag, tagstrlen, allowedValue, memberFnPointer, parser)                       \
    struct tag##_allowedValue : allowedValue                                   \
    {                                                                          \
    };                                                                         \
                                                                               \
    struct tag##_                                                              \
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
        static auto apply(const ActionInput& input, TagsWriter& chart) -> void \
        {                                                                      \
            CALL_MEMBER_FN(chart, memberFnPointer)((parser)(input.string()));  \
        }                                                                      \
    };

namespace {
constexpr auto identity = std::identity();
constexpr auto trimR = [](auto&& str) {
    return std::string(
      std::string_view{ str }.substr(0, str.find_last_not_of(' ') + 1));
};
} // namespace

RHYTHMGAME_TAG_PARSER(title, 5, metaString, &TagsWriter::setTitle, trimR);
RHYTHMGAME_TAG_PARSER(artist, 6, metaString, &TagsWriter::setArtist, trimR);
RHYTHMGAME_TAG_PARSER(subArtist,
                      9,
                      metaString,
                      &TagsWriter::setSubArtist,
                      trimR);
RHYTHMGAME_TAG_PARSER(subTitle, 8, metaString, &TagsWriter::setSubTitle, trimR);
RHYTHMGAME_TAG_PARSER(bpm, 3, floating, &TagsWriter::setBpm, std::stod);
RHYTHMGAME_TAG_PARSER(genre, 5, metaString, &TagsWriter::setGenre, trimR);
RHYTHMGAME_TAG_PARSER(random,
                      6,
                      pegtl::plus<pegtl::digit>,
                      &TagsWriter::enterRandom,
                      ([](auto&& value) {
                          return RandomRange{ 0, std::stol(trimR(value)) };
                      }))
RHYTHMGAME_TAG_PARSER(if,
                      2,
                      pegtl::plus<pegtl::digit>,
                      &TagsWriter::enterIf,
                      ([](auto&& value) { return std::stol(trimR(value)); }))

struct commonTag
  : pegtl::sor<title_, subTitle_, artist_, subArtist_, bpm_, genre_>
{
};

struct randomEnd
  : pegtl::sor<
      noValueTag<pegtl::istring<'E', 'N', 'D', 'R', 'A', 'N', 'D', 'O', 'M'>>,
      pegtl::eof>
{
};

struct ifBlock;

struct randomBlock
  : pegtl::
      seq<random_, pegtl::eol, pegtl::list<ifBlock, pegtl::eolf>, randomEnd>
{
};

struct ifEnd
  : pegtl::sor<pegtl::eof, noValueTag<pegtl::istring<'E', 'N', 'D', 'I', 'F'>>>
{
};

struct ifBlock
  : pegtl::seq<if_,
               pegtl::eol,
               pegtl::list<pegtl::sor<commonTag, randomBlock>, pegtl::eolf>,
               ifEnd>
{
};

struct file
  : pegtl::list<pegtl::sor<randomBlock, commonTag>, pegtl::plus<pegtl::space>>
{
};

template<>
struct action<randomEnd>
{
    template<typename ActionInput>
    static auto apply(const ActionInput& /*input*/, TagsWriter& chart) -> void
    {
        chart.leaveRandom();
    }
};

template<>
struct action<ifEnd>
{
    template<typename ActionInput>
    static auto apply(const ActionInput& /*input*/, TagsWriter& chart) -> void
    {
        chart.leaveIf();
    }
};

// TODO: change the entire "Chart" class to fit actual usage. This is unusable
// atm.
auto
BmsChartReader::readBmsChart(std::string chart) const -> charts::models::Chart
{
    auto tagsOut = readBmsChartTags(std::move(chart));

    return charts::models::Chart{
        tagsOut.title.has_value() ? tagsOut.title.value() : "Untitled",
        tagsOut.artist.has_value() ? tagsOut.artist.value() : "Unknown",
        tagsOut.bpm.has_value() ? tagsOut.bpm.value() : 0.0,
        BmsMeta{ tagsOut.genre, tagsOut.subTitle, tagsOut.subArtist }
    };
}
auto
BmsChartReader::readBmsChartTags(std::string chart) const -> tags
{
    using namespace std::string_literals;
    using namespace std::chrono_literals;

    auto input = pegtl::string_input<>(std::move(chart), "BMS Chart"s);
    auto writer = TagsWriter{};
    std::ignore = pegtl::parse<file, action>(input, writer);

    auto& tagsOut = writer.getTags();

    return std::move(tagsOut);
}
} // namespace charts::chart_readers

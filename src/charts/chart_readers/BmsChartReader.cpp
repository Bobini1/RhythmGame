//
// Created by bobini on 07.07.2022.
//

#include <utility>
#include "charts/chart_readers/ToChars.h"
#include <tao/pegtl.hpp>
#include <stack>
#include "BmsChartReader.h"

#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))

namespace charts::chart_readers {
namespace pegtl = tao::pegtl;

using IfTag = models::BmsChart::IfTag;
using RandomRange = models::BmsChart::RandomRange;
using Tags = models::BmsChart::Tags;

class TagsWriter
{
    std::stack<std::pair<IfTag, Tags>> ifStack;
    std::stack<std::pair<RandomRange, std::vector<std::pair<IfTag, Tags>>>>
      randomStack;

  public:
    TagsWriter() { ifStack.push(std::make_pair(0, Tags{})); }
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

    void enterIf(IfTag tag) { ifStack.push(std::make_pair(tag, Tags{})); }

    void leaveIf()
    {
        randomStack.top().second.emplace_back(ifStack.top().first,
                                              std::move(ifStack.top().second));
        ifStack.pop();
    }

    void enterRandom(RandomRange range)
    {
        randomStack.push(
          std::make_pair(range, std::vector<std::pair<IfTag, Tags>>{}));
    }

    void leaveRandom()
    {
        auto ifs = std::move(randomStack.top().second);
        auto randomDistribution = randomStack.top().first;
        randomStack.pop();
        auto ifsMap = std::multimap<IfTag, Tags>{};
        for (auto& ifTag : ifs) {
            ifsMap.emplace(ifTag.first, std::move(ifTag.second));
        }
        ifStack.top().second.randomBlocks.emplace_back(std::make_pair(
          randomDistribution,
          std::make_unique<std::multimap<IfTag, Tags>>(std::move(ifsMap))));
    }

    auto getTags() -> Tags& { return ifStack.top().second; }
};
// double

struct PlusMinus : pegtl::opt<pegtl::one<'+', '-'>>
{
};
struct Dot : pegtl::one<'.'>
{
};

struct Inf
  : pegtl::seq<pegtl::istring<'i', 'n', 'f'>,
               pegtl::opt<pegtl::istring<'i', 'n', 'i', 't', 'y'>>>
{
};

struct Nan
  : pegtl::seq<
      pegtl::istring<'n', 'a', 'n'>,
      pegtl::opt<pegtl::one<'('>, pegtl::plus<pegtl::alnum>, pegtl::one<')'>>>
{
};

template<typename D>
struct Number
  : pegtl::if_then_else<
      Dot,
      pegtl::plus<D>,
      pegtl::seq<pegtl::plus<D>, pegtl::opt<Dot, pegtl::star<D>>>>
{
};

struct E : pegtl::one<'e', 'E'>
{
};
struct P : pegtl::one<'p', 'P'>
{
};
struct F : pegtl::one<'f', 'F'>
{
};
struct D : pegtl::one<'d', 'D'>
{
};
struct Exponent : pegtl::seq<PlusMinus, pegtl::plus<pegtl::digit>>
{
};

struct Decimal : pegtl::seq<Number<pegtl::digit>, pegtl::opt<E, Exponent>>
{
};
struct Hexadecimal
  : pegtl::seq<pegtl::one<'0'>,
               pegtl::one<'x', 'X'>,
               Number<pegtl::xdigit>,
               pegtl::opt<P, Exponent>>
{
};

struct Floating
  : pegtl::seq<
      PlusMinus,
      pegtl::sor<Hexadecimal, Decimal, Inf, Nan, pegtl::opt<pegtl::sor<F, D>>>>
{
};

// double end

struct SpacesUntilEndOfLine
  : pegtl::seq<pegtl::star<pegtl::minus<pegtl::space, pegtl::eolf>>,
               pegtl::at<pegtl::eolf>>
{
};

struct MetaString : pegtl::until<SpacesUntilEndOfLine>
{
};

template<typename AllowedValue, typename TagName>
struct MetaTag
  : pegtl::seq<pegtl::star<pegtl::space>,
               pegtl::one<'#'>,
               TagName,
               pegtl::star<pegtl::space>,
               AllowedValue,
               pegtl::star<pegtl::minus<pegtl::space, pegtl::eolf>>>
{
};

template<typename TagName>
struct NoValueTag
  : pegtl::seq<pegtl::star<pegtl::space>,
               pegtl::one<'#'>,
               TagName,
               pegtl::star<pegtl::minus<pegtl::space, pegtl::eolf>>>
{
};

struct PlayerValidDigit : pegtl::range<'1', '4'>
{
};

template<typename>
struct Action
{
};

/**
 * @brief defines a bms-style tag and an action to be performed when it is
 * found.
 * @param tag the tag to be searched for. It is both the name of the resulting
 * class and the search pattern. Example: for tag **player**, the matching tag
 * will be `#PLAYER <value>` (case-insensitive).
 * @param strlen the length of the tag name provided in the first argument.
 * Required for internal handling, sadly.
 * @param allowedValue the pegtl parser for the value that is allowed for this
 * tag.
 * @param memberFnPointer the function to be called when the tag is found.
 * @param parser the function to apply to the allowedValue when it is found.
 * Kind of a preprocessor.
 */
#define RHYTHMGAME_TAG_PARSER(                                                 \
  tag, tagstrlen, allowedValue, memberFnPointer, parser)                       \
    struct tag##_allowedValue : allowedValue                                   \
    {                                                                          \
    };                                                                         \
                                                                               \
    struct tag                                                                 \
      : MetaTag<tag##_allowedValue,                                            \
                pegtl::istring<RHYTHMGAME_TO_CHARS(tagstrlen, #tag)>>          \
    {                                                                          \
    };                                                                         \
                                                                               \
    template<>                                                                 \
    struct Action<tag##_allowedValue>                                          \
    {                                                                          \
                                                                               \
        template<typename ActionInput>                                         \
        static auto apply(const ActionInput& input, TagsWriter& chart) -> void \
        {                                                                      \
            CALL_MEMBER_FN(chart, memberFnPointer)((parser)(input.string()));  \
        }                                                                      \
    };

namespace {
// constexpr auto identity = std::identity();
constexpr auto trimR = [](auto&& str) {
    return std::string(
      std::string_view{ str }.substr(0, str.find_last_not_of(' ') + 1));
};
} // namespace

RHYTHMGAME_TAG_PARSER(Title, 5, MetaString, &TagsWriter::setTitle, trimR)
RHYTHMGAME_TAG_PARSER(Artist, 6, MetaString, &TagsWriter::setArtist, trimR)
RHYTHMGAME_TAG_PARSER(SubArtist,
                      9,
                      MetaString,
                      &TagsWriter::setSubArtist,
                      trimR)
RHYTHMGAME_TAG_PARSER(SubTitle, 8, MetaString, &TagsWriter::setSubTitle, trimR)
RHYTHMGAME_TAG_PARSER(Bpm, 3, Floating, &TagsWriter::setBpm, std::stod)
RHYTHMGAME_TAG_PARSER(Genre, 5, MetaString, &TagsWriter::setGenre, trimR)
RHYTHMGAME_TAG_PARSER(Random,
                      6,
                      pegtl::plus<pegtl::digit>,
                      &TagsWriter::enterRandom,
                      ([](auto&& value) {
                          return RandomRange{ 0, std::stol(trimR(value)) };
                      }))
RHYTHMGAME_TAG_PARSER(If,
                      2,
                      pegtl::plus<pegtl::digit>,
                      &TagsWriter::enterIf,
                      ([](auto&& value) { return std::stol(trimR(value)); }))

struct CommonTag : pegtl::sor<Title, SubTitle, Artist, SubArtist, Bpm, Genre>
{
};

struct RandomEnd
  : pegtl::sor<
      NoValueTag<pegtl::istring<'E', 'N', 'D', 'R', 'A', 'N', 'D', 'O', 'M'>>,
      pegtl::eof>
{
};

struct IfBlock;

struct RandomBlock
  : pegtl::seq<Random, pegtl::eol, pegtl::list<IfBlock, pegtl::eolf>, RandomEnd>
{
};

struct IfEnd
  : pegtl::sor<pegtl::eof, NoValueTag<pegtl::istring<'E', 'N', 'D', 'I', 'F'>>>
{
};

struct IfBlock
  : pegtl::seq<If,
               pegtl::eol,
               pegtl::list<pegtl::sor<CommonTag, RandomBlock>, pegtl::eolf>,
               IfEnd>
{
};

struct File
  : pegtl::list<pegtl::sor<RandomBlock, CommonTag>, pegtl::plus<pegtl::space>>
{
};

template<>
struct Action<RandomEnd>
{
    template<typename ActionInput>
    static auto apply(const ActionInput& /*input*/, TagsWriter& chart) -> void
    {
        chart.leaveRandom();
    }
};

template<>
struct Action<IfEnd>
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
BmsChartReader::readBmsChart(const std::string& chart) const
  -> std::unique_ptr<charts::models::BmsChart>
{
    auto tagsOutRead = readBmsChartTags(chart);
    if (!tagsOutRead) {
        return nullptr;
    }
    return std::make_unique<charts::models::BmsChart>(
      std::move(tagsOutRead.value()));
}
auto
BmsChartReader::readBmsChartTags(const std::string& chart) const
  -> std::optional<models::BmsChart::Tags>
{
    using namespace std::string_literals;

    auto input = pegtl::string_input<>(chart, "BMS Chart"s);
    auto writer = TagsWriter{};
    if (!pegtl::parse<File, Action>(input, writer)) {
        return std::nullopt;
    }

    auto& tagsOut = writer.getTags();

    return std::move(tagsOut);
}
} // namespace charts::chart_readers

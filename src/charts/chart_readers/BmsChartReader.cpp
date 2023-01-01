//
// Created by bobini on 07.07.2022.
//

#include <utility>
#include "charts/chart_readers/ToChars.h"
#include <tao/pegtl.hpp>
#include <stack>
#include <functional>
#include "BmsChartReader.h"

#define TO_CHARS(STRLEN, STR) RHYTHMGAME_TO_CHARS(STRLEN, STR)

namespace charts::chart_readers {
namespace pegtl = tao::pegtl;

using IfTag = models::BmsChart::IfTag;
using RandomRange = models::BmsChart::RandomRange;
using Tags = models::BmsChart::Tags;

class TagsWriter
{
    std::stack<std::pair<IfTag, Tags>> ifStack{};
    std::stack<std::pair<RandomRange, std::vector<std::pair<IfTag, Tags>>>>
      randomStack{};

    unsigned int currentMeasure{};
    unsigned int currentColumn{};

  public:
    TagsWriter() { ifStack.emplace(0, Tags{}); }
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

    void enterIf(IfTag tag) { ifStack.emplace(tag, Tags{}); }

    void leaveIf()
    {
        randomStack.top().second.emplace_back(ifStack.top().first,
                                              std::move(ifStack.top().second));
        ifStack.pop();
    }

    void enterRandom(RandomRange range)
    {
        randomStack.emplace(range, std::vector<std::pair<IfTag, Tags>>{});
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
        ifStack.top().second.randomBlocks.emplace_back(
          randomDistribution,
          std::make_unique<std::multimap<IfTag, Tags>>(std::move(ifsMap)));
    }

    auto setMeasure(unsigned int measure) -> void { currentMeasure = measure; }

    auto setColumn(unsigned int column) -> void { currentColumn = column; }

    auto addP1VisibleNoteValue(std::string value) -> void
    {
        ifStack.top()
          .second.measures[currentMeasure]
          .p1VisibleNotes[currentColumn]
          .push_back(std::move(value));
    }

    auto getTags() -> Tags& { return ifStack.top().second; }
    void addBgmVector()
    {
        ifStack.top().second.measures[currentMeasure].bgmNotes.emplace_back();
    }

    void addBgmNoteValue(std::string value)
    {
        ifStack.top().second.measures[currentMeasure].bgmNotes.back().push_back(
          std::move(value));
    }
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
  : pegtl::seq<PlusMinus,
               pegtl::sor<Hexadecimal, Decimal, Inf, Nan>,
               pegtl::opt<pegtl::sor<F, D>>>
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
#define TAG_PARSER(tag, tagstrlen, allowedValue, memberFnPointer, parser)      \
    struct tag##_allowedValue : allowedValue                                   \
    {                                                                          \
    };                                                                         \
                                                                               \
    struct tag                                                                 \
      : MetaTag<tag##_allowedValue, pegtl::istring<TO_CHARS(tagstrlen, #tag)>> \
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
            std::invoke(memberFnPointer, chart, (parser)(input.string()));     \
        }                                                                      \
    };

namespace {
// constexpr auto identity = std::identity();
constexpr auto trimR = [](auto&& str) {
    return std::string(
      std::string_view{ str }.substr(0, str.find_last_not_of(' ') + 1));
};
} // namespace

TAG_PARSER(Title, 5, MetaString, &TagsWriter::setTitle, trimR)
TAG_PARSER(Artist, 6, MetaString, &TagsWriter::setArtist, trimR)
TAG_PARSER(SubArtist, 9, MetaString, &TagsWriter::setSubArtist, trimR)
TAG_PARSER(SubTitle, 8, MetaString, &TagsWriter::setSubTitle, trimR)
TAG_PARSER(Bpm, 3, Floating, &TagsWriter::setBpm, std::stod)
TAG_PARSER(Genre, 5, MetaString, &TagsWriter::setGenre, trimR)
TAG_PARSER(Random,
           6,
           pegtl::plus<pegtl::digit>,
           &TagsWriter::enterRandom,
           ([](auto&& value) {
               return RandomRange{ 0, std::stol(trimR(value)) };
           }))
TAG_PARSER(If,
           2,
           pegtl::plus<pegtl::digit>,
           &TagsWriter::enterIf,
           ([](auto&& value) { return std::stol(trimR(value)); }))

struct MeasureNumber : pegtl::seq<pegtl::rep<3, pegtl::digit>>
{
};

struct NoteValue : pegtl::seq<pegtl::rep<2, pegtl::alnum>>
{
};

struct Column : pegtl::range<'0', '9'>
{
};

struct P1VisibleNoteValue : NoteValue
{
};

struct P1VisibleNotes
  : pegtl::seq<pegtl::one<'#'>,
               MeasureNumber,
               pegtl::seq<pegtl::one<'1'>, Column, pegtl::one<':'>>,
               pegtl::plus<P1VisibleNoteValue>>
{
};

struct BgmNoteValue : NoteValue
{
};

struct BgmMeasureNumber : MeasureNumber
{
};

struct BgmNotes
  : pegtl::seq<pegtl::one<'#'>,
               BgmMeasureNumber,
               pegtl::seq<pegtl::string<'0', '1'>, pegtl::one<':'>>,
               pegtl::plus<BgmNoteValue>>
{
};

template<>
struct Action<MeasureNumber>
{
    template<typename ActionInput>
    static auto apply(const ActionInput& input, TagsWriter& chart) -> void
    {
        chart.setMeasure(static_cast<unsigned int>(std::stoul(input.string())));
    }
};

template<>
struct Action<BgmMeasureNumber>
{
    template<typename ActionInput>
    static auto apply(const ActionInput& /*input*/, TagsWriter& chart) -> void
    {
        chart.addBgmVector();
    }
};

template<>
struct Action<BgmNoteValue>
{
    template<typename ActionInput>
    static auto apply(const ActionInput& input, TagsWriter& chart) -> void
    {
        chart.addBgmNoteValue(input.string());
    }
};

template<>
struct Action<P1VisibleNoteValue>
{
    template<typename ActionInput>
    static auto apply(const ActionInput& input, TagsWriter& chart) -> void
    {
        chart.addP1VisibleNoteValue(input.string());
    }
};

template<>
struct Action<Column>
{
    template<typename ActionInput>
    static auto apply(const ActionInput& input, TagsWriter& chart) -> void
    {
        chart.setColumn(static_cast<unsigned int>(std::stoul(input.string())));
    }
};

struct CommonTag
  : pegtl::sor<Title,
               SubTitle,
               Artist,
               SubArtist,
               Bpm,
               Genre,
               P1VisibleNotes,
               BgmNotes>
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
  : pegtl::seq<pegtl::list<pegtl::sor<RandomBlock, CommonTag>,
                           pegtl::plus<pegtl::space>>,
               pegtl::star<pegtl::space>,
               pegtl::eof>
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

auto
BmsChartReader::readBmsChart(const std::string& chart) const
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

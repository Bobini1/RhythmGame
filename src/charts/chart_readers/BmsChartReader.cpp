//
// Created by bobini on 07.07.2022.
//

#include <utility>
#include <lexy/action/parse.hpp>
#include <lexy/dsl.hpp>
#include <lexy/callback.hpp>
#include <functional>
#include <lexy/input/string_input.hpp>
#include <spdlog/spdlog.h>
#include <boost/serialization/strong_typedef.hpp>
#include "BmsChartReader.h"

#include <lexy_ext/report_error.hpp>

namespace charts::chart_readers {
namespace dsl = lexy::dsl;

namespace {
[[nodiscard]] auto
trimR(auto&& str) -> std::string
{
    return std::string(
      std::string_view{ str }.substr(0, str.find_last_not_of(' ') + 1));
};
struct TextTag
{
    static constexpr auto value =
      lexy::as_string<std::string> >>
      lexy::callback<std::string>(trimR<std::string_view>);
    static constexpr auto rule = [] {
        auto limits = dsl::delimited(LEXY_LIT(""), dsl::eol);
        return limits(dsl::code_point);
    }();
};

struct Identifier
{
    static constexpr auto value = lexy::as_string<std::string>;
    static constexpr auto rule =
      dsl::capture(dsl::token(dsl::twice(dsl::ascii::alnum)));
};

struct IdentifierChain
{
    static constexpr auto value = lexy::as_list<std::vector<std::string>>;
    static constexpr auto rule = list(dsl::p<Identifier>);
};

struct FloatingPoint
{
    static constexpr auto rule = [] {
        auto integerPart = dsl::sign + dsl::digits<>;
        auto fraction = dsl::period >> dsl::if_(dsl::digits<>);
        auto exponent =
          dsl::lit_c<'e'> / dsl::lit_c<'E'> >> (dsl::sign + dsl::digits<>);
        auto suffix =
          dsl::lit_c<'f'> / dsl::lit_c<'F'> / dsl::lit_c<'d'> / dsl::lit_c<'D'>;

        auto realNumber = dsl::token(integerPart + dsl::if_(fraction) +
                                     dsl::if_(exponent) + dsl::if_(suffix));
        return dsl::capture(realNumber);
    }();

    static constexpr auto value =
      lexy::as_string<std::string> |
      lexy::callback<double>([](std::string&& str) { return std::stod(str); });
};

struct Channel
{
    static constexpr auto rule =
      dsl::capture(dsl::token(dsl::times<2>(dsl::ascii::alnum)));
    static constexpr auto value =
      lexy::as_string<std::string> | lexy::callback<int>([](std::string&& str) {
          constexpr auto base = 36;
          return std::stoi(str, nullptr, base);
      });
};

struct Measure
{
    static constexpr auto rule =
      dsl::peek(dsl::hash_sign >> dsl::digit<>) >>
      (dsl::hash_sign + dsl::capture(dsl::token(dsl::times<3>(dsl::digit<>))));
    static constexpr auto value =
      lexy::as_string<std::string> | lexy::callback<int>([](std::string&& str) {
          constexpr auto base = 10;
          return std::stoi(str, nullptr, base);
      });
};

struct MeasureBasedTag
{
    static constexpr auto rule = dsl::p<Measure> >>
                                 (dsl::p<Channel> + dsl::colon +
                                  dsl::p<IdentifierChain>);
    static constexpr auto value =
      lexy::construct<std::tuple<uint64_t, int, std::vector<std::string>>>;
};

BOOST_STRONG_TYPEDEF(std::string, Title);
BOOST_STRONG_TYPEDEF(std::string, Artist)
BOOST_STRONG_TYPEDEF(std::string, Genre);
BOOST_STRONG_TYPEDEF(std::string, Subtitle);
BOOST_STRONG_TYPEDEF(std::string, Subartist);
BOOST_STRONG_TYPEDEF(double, Bpm);
using wav_t = std::pair<std::string, std::string>;
BOOST_STRONG_TYPEDEF(wav_t, Wav);
using pair_t = std::pair<std::string, double>;
BOOST_STRONG_TYPEDEF(pair_t, ExBpm);
using meter_t = std::pair<uint64_t, double>;
BOOST_STRONG_TYPEDEF(meter_t, Meter);

struct TitleTag
{
    static constexpr auto rule = [] {
        auto titleTag = dsl::ascii::case_folding(LEXY_LIT("#title"));
        return titleTag >> dsl::p<TextTag>;
    }();
    static constexpr auto value =
      lexy::as_string<std::string> |
      lexy::callback<Title>([](std::string&& str) { return Title{ str }; });
};

struct ArtistTag
{
    static constexpr auto rule = [] {
        auto artistTag = dsl::ascii::case_folding(LEXY_LIT("#artist"));
        return artistTag >> dsl::p<TextTag>;
    }();
    static constexpr auto value =
      lexy::as_string<std::string> |
      lexy::callback<Artist>([](std::string&& str) { return Artist{ str }; });
};

struct GenreTag
{
    static constexpr auto rule = [] {
        auto genreTag = dsl::ascii::case_folding(LEXY_LIT("#genre"));
        return genreTag >> dsl::p<TextTag>;
    }();
    static constexpr auto value =
      lexy::as_string<std::string> |
      lexy::callback<Genre>([](std::string&& str) { return Genre{ str }; });
};

struct SubtitleTag
{
    static constexpr auto rule = [] {
        auto subtitleTag = dsl::ascii::case_folding(LEXY_LIT("#subtitle"));
        return subtitleTag >> dsl::p<TextTag>;
    }();
    static constexpr auto value =
      lexy::as_string<std::string> |
      lexy::callback<Subtitle>(
        [](std::string&& str) { return Subtitle{ str }; });
};

struct SubartistTag
{
    static constexpr auto rule = [] {
        auto subartistTag = dsl::ascii::case_folding(LEXY_LIT("#subartist"));
        return subartistTag >> dsl::p<TextTag>;
    }();
    static constexpr auto value =
      lexy::as_string<std::string> |
      lexy::callback<Subartist>(
        [](std::string&& str) { return Subartist{ str }; });
};

struct BpmTag
{
    static constexpr auto rule = [] {
        auto bpmTag = dsl::ascii::case_folding(LEXY_LIT("#bpm"));
        return bpmTag >> dsl::p<FloatingPoint>;
    }();
    static constexpr auto value =
      lexy::callback<Bpm>([](double num) { return Bpm{ num }; });
};

struct WavTag
{
    static constexpr auto rule = [] {
        auto wavTag = dsl::ascii::case_folding(LEXY_LIT("#wav"));
        return wavTag >> (dsl::p<Identifier> + dsl::p<TextTag>);
    }();
    static constexpr auto value =
      lexy::callback<Wav>([](std::string&& identifier, std::string&& filename) {
          return Wav{ { std::move(identifier), std::move(filename) } };
      });
};

struct ExBpmTag
{
    static constexpr auto rule = [] {
        auto exBpmTag =
          dsl::hash_sign + dsl::if_(dsl::ascii::case_folding(LEXY_LIT("ex"))) +
          dsl::ascii::case_folding(LEXY_LIT("bpm")) + dsl::p<Identifier>;
        return dsl::peek(exBpmTag) >> (exBpmTag + dsl::p<FloatingPoint>);
    }();
    static constexpr auto value =
      lexy::callback<ExBpm>([](std::string identifier, double num) {
          return ExBpm{ { std::move(identifier), num } };
      });
};

struct MeterTag
{
    static constexpr auto rule = [] {
        auto start = (dsl::p<Measure> + LEXY_LIT("02"));
        return dsl::peek(start) >> start >> dsl::colon >> dsl::p<FloatingPoint>;
    }();
    static constexpr auto value =
      lexy::callback<Meter>([](uint64_t measure, double num) {
          return Meter{ { measure, num } };
      });
};

struct TagsSink
{
    struct SinkCallback
    {
        using return_type = // NOLINT(readability-identifier-naming)
          models::BmsChart::Tags;
        models::BmsChart::Tags state;
        auto finish() && -> return_type { return std::move(state); }
        auto operator()(Title&& title) -> void
        {
            state.title = std::move(static_cast<std::string&>(title));
        }
        auto operator()(Artist&& artist) -> void
        {
            state.artist = std::move(static_cast<std::string&>(artist));
        }
        auto operator()(Genre&& genre) -> void
        {
            state.genre = std::move(static_cast<std::string&>(genre));
        }
        auto operator()(Subtitle&& subtitle) -> void
        {
            state.subTitle = std::move(static_cast<std::string&>(subtitle));
        }
        auto operator()(Subartist&& subartist) -> void
        {
            state.subArtist = std::move(static_cast<std::string&>(subartist));
        }
        auto operator()(Bpm&& bpm) -> void
        {
            state.bpm = static_cast<double>(bpm);
        }
        auto operator()(ExBpm&& bpm) -> void
        {
            auto& [identifier, value] =
              static_cast<std::pair<std::string, double>&>(bpm);
            state.exBpms[identifier] = value;
        }
        auto operator()(
          std::pair<models::BmsChart::RandomRange,
                    std::vector<std::pair<models::BmsChart::IfTag,
                                          models::BmsChart::Tags>>>&&
            randomBlock)
        {
            state.randomBlocks.emplace_back(std::move(randomBlock));
        }

        auto operator()(Meter&& meter) -> void
        {
            auto [measure, value] =
              static_cast<std::pair<uint64_t, double>>(meter);
            state.measures[measure].meter = value;
        }

        auto operator()(Wav&& wav) -> void
        {
            auto& [identifier, filename] =
              static_cast<std::pair<std::string, std::string>&>(wav);
            state.wavs[identifier] = std::move(filename);
        }

        auto operator()(
          std::tuple<uint64_t, int, std::vector<std::string>>&& measureBasedTag)
          -> void
        {
            auto [measure, channel, identifiers] = std::move(measureBasedTag);

            enum ChannelCategories
            {
                General = 0,
                P1Visible = 1,
                P2Visible = 2,
                P1Invisible = 3,
                P2Invisible = 4,
                P1LongNote = 5,
                P2LongNote = 6,
            };
            enum GeneralSubcategories
            {
                Bgm = 1,
                /* Meter = 2, // handled elsewhere */
                Bpm = 3,
                /* BgaBase = 4, // unimplemented
                ExtendedObject = 5,
                SeekObject = 6,
                BgaLayer = 7, */
                ExBpm = 8,
                /* Stop = 9, // unimplemented */
            };
            constexpr auto base = 36;
            auto channelCategory =
              static_cast<ChannelCategories>(channel / base);
            auto channelSubcategory = static_cast<unsigned>(channel % base);
            switch (channelCategory) {
                case General:
                    switch (channelSubcategory) {
                        case Bgm:
                            state.measures[measure].bgmNotes.push_back(
                              std::move(identifiers));
                            break;
                        case Bpm:
                            state.measures[measure].bpmChanges =
                              std::move(identifiers);
                            break;
                        case ExBpm:
                            state.measures[measure].exBpmChanges =
                              std::move(identifiers);
                            break;
                        default:
                            spdlog::error("Unknown channel: {}", channel);
                            break;
                    }
                    break;
                case P1Visible:
                    state.measures[measure]
                      .p1VisibleNotes[channelSubcategory - 1] =
                      std::move(identifiers);
                    break;
                case P2Visible:
                    state.measures[measure]
                      .p2VisibleNotes[channelSubcategory - 1] =
                      std::move(identifiers);
                    break;
                case P1Invisible:
                    state.measures[measure]
                      .p1InvisibleNotes[channelSubcategory - 1] =
                      std::move(identifiers);
                    break;
                case P2Invisible:
                    state.measures[measure]
                      .p2InvisibleNotes[channelSubcategory - 1] =
                      std::move(identifiers);
                    break;
                case P1LongNote:
                    state.measures[measure]
                      .p1LongNotes[channelSubcategory - 1] =
                      std::move(identifiers);
                    break;
                case P2LongNote:
                    state.measures[measure]
                      .p2LongNotes[channelSubcategory - 1] =
                      std::move(identifiers);
                    break;
                default:
                    spdlog::error("Unknown channel: {}", channel);
                    break;
            }
        }
    };
    [[nodiscard]] auto sink() const -> SinkCallback { return SinkCallback{}; }
};

struct RandomBlock;

struct MainTags
{
    static constexpr auto whitespace = dsl::whitespace(dsl::unicode::space);
    static constexpr auto rule = [] {
        auto term = dsl::terminator(
          dsl::eof | dsl::peek(dsl::ascii::case_folding(LEXY_LIT("#endif"))));
        return term.list(dsl::try_(
          dsl::p<TitleTag> | dsl::p<ArtistTag> | dsl::p<GenreTag> |
            dsl::p<SubtitleTag> | dsl::p<SubartistTag> | dsl::p<ExBpmTag> |
            dsl::p<BpmTag> | dsl::p<MeterTag> | dsl::p<WavTag> |
            dsl::p<MeasureBasedTag> | dsl::recurse_branch<RandomBlock>,
          dsl::until(dsl::unicode::newline).or_eof()));
    }();
    static constexpr auto value = TagsSink{};
};

struct IfBlock
{
    static constexpr auto rule =
      dsl::ascii::case_folding(LEXY_LIT("#if")) >>
      (dsl::integer<models::BmsChart::IfTag>(dsl::digits<>) + dsl::p<MainTags> +
       dsl::ascii::case_folding(LEXY_LIT("#endif")));
    static constexpr auto value = lexy::construct<
      std::pair<models::BmsChart::IfTag, models::BmsChart::Tags>>;
};

struct IfList
{
    static constexpr auto rule = [] {
        auto delims = dsl::terminator(
          dsl::eof |
          dsl::peek(dsl::ascii::case_folding(LEXY_LIT("#endrandom"))));
        return delims.list(dsl::p<IfBlock>);
    }();
    static constexpr auto value = lexy::as_list<
      std::vector<std::pair<models::BmsChart::IfTag, models::BmsChart::Tags>>>;
};

struct RandomBlock
{
    static constexpr auto rule = [] {
        return dsl::ascii::case_folding(LEXY_LIT("#random")) >>
               (dsl::integer<models::BmsChart::RandomRange>(dsl::digits<>) +
                dsl::p<IfList> +
                (dsl::ascii::case_folding(LEXY_LIT("#endrandom")) | dsl::eof));
    }();
    static constexpr auto value = lexy::construct<std::pair<
      models::BmsChart::RandomRange,
      std::vector<std::pair<models::BmsChart::IfTag, models::BmsChart::Tags>>>>;
};
} // namespace

auto
BmsChartReader::readBmsChart(const std::string& chart) const
  -> models::BmsChart::Tags
{
    auto result =
      lexy::parse<MainTags>(lexy::string_input<lexy::utf8_char_encoding>(chart),
                            lexy_ext::report_error);
    return std::move(result).value();
}
} // namespace charts::chart_readers

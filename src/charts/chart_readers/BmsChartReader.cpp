//
// Created by bobini on 07.07.2022.
//

#include <utility>
#include "charts/chart_readers/ToChars.h"
#include <lexy/action/parse.hpp>
#include <lexy/dsl.hpp>
#include <lexy/callback.hpp>
#include <functional>
#include <lexy/input/string_input.hpp>
#include <spdlog/spdlog.h>
#include <type_safe/strong_typedef.hpp>
#include "BmsChartReader.h"

#include <lexy_ext/report_error.hpp>

namespace charts::chart_readers {
namespace dsl = lexy::dsl;

namespace {
constexpr auto trimR = [](auto&& str) {
    return std::string(
      std::string_view{ str }.substr(0, str.find_last_not_of(' ') + 1));
};
} // namespace
struct TextTag
{
    static constexpr auto value = lexy::as_string<std::string> >>
                                  lexy::callback<std::string>(trimR);
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
          dsl::lit_c<'e'> / dsl::lit_c<'E'> >> dsl::sign + dsl::digits<>;
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
      dsl::hash_sign + dsl::capture(dsl::token(dsl::times<3>(dsl::digit<>)));
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

struct Title : type_safe::strong_typedef<Title, std::string>
{
    using strong_typedef::strong_typedef;
};

struct Artist : type_safe::strong_typedef<Artist, std::string>
{
    using strong_typedef::strong_typedef;
};

struct Genre : type_safe::strong_typedef<Genre, std::string>
{
    using strong_typedef::strong_typedef;
};

struct Subtitle : type_safe::strong_typedef<Subtitle, std::string>
{
    using strong_typedef::strong_typedef;
};

struct Subartist : type_safe::strong_typedef<Subartist, std::string>
{
    using strong_typedef::strong_typedef;
};

struct Bpm : type_safe::strong_typedef<Bpm, double>
{
    using strong_typedef::strong_typedef;
};

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

struct TagsSink
{
    struct SinkCallback
    {
        using return_type = models::BmsChart::Tags;
        models::BmsChart::Tags state;
        auto finish() && -> return_type { return std::move(state); }
        auto operator()(Title&& title) -> void
        {
            state.title = static_cast<std::string>(title);
        }
        auto operator()(Artist&& artist) -> void
        {
            state.artist = static_cast<std::string>(artist);
        }
        auto operator()(Genre&& genre) -> void
        {
            state.genre = static_cast<std::string>(genre);
        }
        auto operator()(Subtitle&& subtitle) -> void
        {
            state.subTitle = static_cast<std::string>(subtitle);
        }
        auto operator()(Subartist&& subartist) -> void
        {
            state.subArtist = static_cast<std::string>(subartist);
        }
        auto operator()(Bpm&& bpm) -> void
        {
            state.bpm = static_cast<double>(bpm);
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
                Bgm = 1
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
    auto sink() const -> SinkCallback { return SinkCallback{}; }
};

struct Production
{
    static constexpr auto whitespace = dsl::whitespace(dsl::unicode::space);
    static constexpr auto rule = [] {
        return dsl::list(dsl::p<TitleTag> | dsl::p<ArtistTag> |
                         dsl::p<GenreTag> | dsl::p<SubtitleTag> |
                         dsl::p<SubartistTag> | dsl::p<BpmTag> |
                         dsl::p<MeasureBasedTag>) +
               dsl::eof;
    }();
    static constexpr auto value = TagsSink{};
};

auto
BmsChartReader::readBmsChart(const std::string& chart) const
  -> std::optional<models::BmsChart::Tags>
{
    auto result = lexy::parse<Production>(
      lexy::string_input<lexy::utf8_char_encoding>(chart),
      lexy_ext::report_error);
    if (result.has_value()) {
        return std::move(result).value();
    }
    return std::nullopt;
}
} // namespace charts::chart_readers

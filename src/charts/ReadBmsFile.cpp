//
// Created by bobini on 07.07.2022.
//

#include <utility>
#include <variant>
#include <lexy/action/parse.hpp>
#include <lexy/dsl.hpp>
#include <lexy/callback.hpp>
#include <functional>
#include <lexy/input/string_input.hpp>
#include <spdlog/spdlog.h>
#include <type_traits>
#include <iconv.h>
#include <algorithm>
#include "ReadBmsFile.h"

#include <lexy_ext/report_error.hpp>

namespace charts {
namespace dsl = lexy::dsl;

namespace {
auto
trimR(std::string_view str) -> size_t
{
    auto pos = str.find_last_not_of(" \t\r\n");
    if (pos != std::string::npos) {
        return pos + 1;
    }
    return 0;
}

auto
convertToUtf8(const char* encoding,
              const std::string_view& str,
              const bool ignoreErrors = false) -> std::optional<std::string>
{
    const auto* targetEncoding = "UTF-8";
    if (ignoreErrors) {
        targetEncoding = "UTF-8//IGNORE";
    }

    iconv_t cd = iconv_open(targetEncoding, encoding);
    if (cd == (iconv_t)-1) {
        return std::nullopt;
    }

    auto* srcPtr = const_cast<char*>(str.data());
    auto srcLeft = str.size();

    const auto dstSize = str.size() * 4;
    std::vector<char> dstBuf(dstSize + 1);
    auto* dstPtr = dstBuf.data();
    auto dstLeft = dstSize;

    auto result = iconv(cd, &srcPtr, &srcLeft, &dstPtr, &dstLeft);
    iconv_close(cd);

    if (result == static_cast<size_t>(-1) && !ignoreErrors) {
        return std::nullopt;
    }

    *dstPtr = '\0';
    const auto result_size = dstSize - dstLeft;
    const auto size = trimR({ dstBuf.data(), result_size });

    return std::string(dstBuf.data(), size);
}

struct ParseState
{
    std::function<ParsedBmsChart::RandomRange(ParsedBmsChart::RandomRange)>
      randomGenerator;
    bool utf8;
};

constexpr auto decode = [](auto&& str,
                           const ParseState& parseState) -> std::string {
    const auto view = std::string_view{ str.data(), str.size() };
    if (parseState.utf8) {
        auto ret = convertToUtf8("UTF-8", view);
        if (ret.has_value()) {
            return ret.value();
        }
        ret = convertToUtf8("UTF-8", view, true);
        spdlog::warn("Text tag contains invalid UTF-8 characters: {}",
                     ret.value_or(std::string{}));
        return ret.value_or(std::string{});
    }
    auto ret = convertToUtf8("CP932", view);
    if (ret.has_value()) {
        return ret.value();
    }
    ret = convertToUtf8("CP949", view);
    if (ret.has_value()) {
        return ret.value();
    }
    ret = convertToUtf8("CP932", view, true);

    spdlog::warn("Text tag contains invalid CP932 characters: {}",
                 ret.value_or(std::string{}));
    return ret.value_or(std::string{});
};

} // namespace

struct TextTag
{
    static constexpr auto value =
      lexy::bind(lexy::callback<std::string>(decode),
                 lexy::values,
                 lexy::parse_state);
    static constexpr auto rule = capture(until(dsl::unicode::newline).or_eof());
};

struct Identifier
{
    static constexpr auto value = lexy::callback<uint16_t>([](auto&& str) {
        auto res = uint16_t{};
        auto [ptr, ec] = std::from_chars(str.begin(), str.end(), res, 36);
        if (ec == std::errc{} && ptr == str.end()) {
            return res;
        }
        return uint16_t{ 0 };
    });
    static constexpr auto rule = capture(token(twice(dsl::ascii::alnum)));
};

struct IdentifierChain
{
    static constexpr auto value = lexy::as_list<std::vector<uint16_t>>;
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

        auto realNumber =
          token(integerPart + if_(fraction) + if_(exponent) + if_(suffix));
        return capture(realNumber);
    }();

    static constexpr auto value =
      lexy::as_string<std::string> |
      lexy::callback<double>([](std::string&& str) {
          auto val = ParsedBmsChart::Measure::defaultMeter;
          auto err =
            std::from_chars(str.c_str(), str.c_str() + str.size(), val);
          if (err.ec != std::errc{}) {
              spdlog::error("Failed to parse meter: {}", str);
          }
          return val;
      });
};

struct Channel
{
    static constexpr auto rule =
      capture(token(dsl::times<2>(dsl::ascii::alnum)));
    static constexpr auto value = lexy::callback<int>([](auto&& str) {
        constexpr auto base = 36;
        auto result = 0;
        std::from_chars(str.begin(), str.end(), result, base);
        return result;
    });
};

struct Measure
{
    static constexpr auto rule = peek(dsl::hash_sign >> dsl::digit<>) >>
                                 (dsl::hash_sign +
                                  capture(token(dsl::times<3>(dsl::digit<>))));
    static constexpr auto value = lexy::callback<int>([](auto&& str) {
        constexpr auto base = 10;
        auto result = 0;
        std::from_chars(str.begin(), str.end(), result, base);
        return result;
    });
};

struct MeasureBasedTag
{
    static constexpr auto rule = dsl::p<Measure> >>
                                 (dsl::p<Channel> + dsl::colon +
                                  dsl::p<IdentifierChain>);
    static constexpr auto value =
      lexy::construct<std::tuple<int64_t, int, std::vector<uint16_t>>>;
};

#define RG_STRONG_TYPEDEF(T, D)                                                \
    struct D                                                                   \
    {                                                                          \
        T t;                                                                   \
        explicit D(T t_) noexcept(                                             \
          std::is_nothrow_move_constructible<T>::value)                        \
          : t(std::move(t_))                                                   \
        {                                                                      \
        }                                                                      \
        D() noexcept(std::is_nothrow_constructible<T>::value)                  \
          : t()                                                                \
        {                                                                      \
        }                                                                      \
        D(const D& t_)                                                         \
          : t(t_.t)                                                            \
        {                                                                      \
        }                                                                      \
        D(D&& t_) noexcept(std::is_nothrow_move_constructible<T>::value)       \
          : t(std::move(t_.t))                                                 \
        {                                                                      \
        }                                                                      \
        D& operator=(const D& rhs) noexcept(                                   \
          std::is_nothrow_copy_assignable<T>::value)                           \
        {                                                                      \
            t = rhs.t;                                                         \
            return *this;                                                      \
        }                                                                      \
        D& operator=(const T& rhs) noexcept(                                   \
          std::is_nothrow_copy_assignable<T>::value)                           \
        {                                                                      \
            t = rhs;                                                           \
            return *this;                                                      \
        }                                                                      \
        D& operator=(D&& rhs) noexcept(                                        \
          std::is_nothrow_move_assignable<T>::value)                           \
        {                                                                      \
            t = std::move(rhs.t);                                              \
            return *this;                                                      \
        }                                                                      \
        D& operator=(T&& rhs) noexcept(                                        \
          std::is_nothrow_move_assignable<T>::value)                           \
        {                                                                      \
            t = std::move(rhs);                                                \
            return *this;                                                      \
        }                                                                      \
        operator const T&() const                                              \
        {                                                                      \
            return t;                                                          \
        }                                                                      \
        operator T&() &                                                        \
        {                                                                      \
            return t;                                                          \
        }                                                                      \
        operator T&&() &&                                                      \
        {                                                                      \
            return std::move(t);                                               \
        }                                                                      \
        bool operator==(const D& rhs) const                                    \
        {                                                                      \
            return t == rhs.t;                                                 \
        }                                                                      \
        bool operator<(const D& rhs) const                                     \
        {                                                                      \
            return t < rhs.t;                                                  \
        }                                                                      \
    };

RG_STRONG_TYPEDEF(std::string, Title)
RG_STRONG_TYPEDEF(std::string, Artist)
RG_STRONG_TYPEDEF(std::string, Subtitle)
RG_STRONG_TYPEDEF(std::string, Subartist)
RG_STRONG_TYPEDEF(std::string, Genre)
RG_STRONG_TYPEDEF(std::string, StageFile)
RG_STRONG_TYPEDEF(std::string, Banner)
RG_STRONG_TYPEDEF(std::string, BackBmp)
RG_STRONG_TYPEDEF(double, Total)
RG_STRONG_TYPEDEF(int, Rank)
RG_STRONG_TYPEDEF(double, Bpm)
RG_STRONG_TYPEDEF(int, PlayLevel)
RG_STRONG_TYPEDEF(int, Difficulty)
using wav_t = std::pair<uint16_t, std::string>;
RG_STRONG_TYPEDEF(wav_t, Wav)
using bmp_t = std::pair<uint16_t, std::string>;
RG_STRONG_TYPEDEF(bmp_t, Bmp)
using lnobj_t = uint16_t;
RG_STRONG_TYPEDEF(lnobj_t, LnObj)
RG_STRONG_TYPEDEF(int, LnType)
using pair_t = std::pair<uint16_t, double>;
RG_STRONG_TYPEDEF(pair_t, ExBpm)
using stop_t = std::pair<uint16_t, double>;
RG_STRONG_TYPEDEF(stop_t, Stop)
using meter_t = std::pair<int64_t, double>;
RG_STRONG_TYPEDEF(meter_t, Meter)

struct TitleTag
{
    static constexpr auto rule = [] {
        auto titleTag = dsl::ascii::case_folding(LEXY_LIT("#title"));
        return titleTag >> dsl::p<TextTag>;
    }();
    static constexpr auto value = lexy::as_string<std::string> |
                                  lexy::callback<Title>([](std::string&& str) {
                                      return Title{ std::move(str) };
                                  });
};

struct ArtistTag
{
    static constexpr auto rule = [] {
        auto artistTag = dsl::ascii::case_folding(LEXY_LIT("#artist"));
        return artistTag >> dsl::p<TextTag>;
    }();
    static constexpr auto value = lexy::as_string<std::string> |
                                  lexy::callback<Artist>([](std::string&& str) {
                                      return Artist{ std::move(str) };
                                  });
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
        [](std::string&& str) { return Subtitle{ std::move(str) }; });
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
        [](std::string&& str) { return Subartist{ std::move(str) }; });
};

struct GenreTag
{
    static constexpr auto rule = [] {
        auto genreTag = dsl::ascii::case_folding(LEXY_LIT("#genre"));
        return genreTag >> dsl::p<TextTag>;
    }();
    static constexpr auto value = lexy::as_string<std::string> |
                                  lexy::callback<Genre>([](std::string&& str) {
                                      return Genre{ std::move(str) };
                                  });
};

struct StageFileTag
{
    static constexpr auto rule = [] {
        auto stageFileTag = dsl::ascii::case_folding(LEXY_LIT("#stagefile"));
        return stageFileTag >> dsl::p<TextTag>;
    }();
    static constexpr auto value =
      lexy::as_string<std::string> |
      lexy::callback<StageFile>(
        [](std::string&& str) { return StageFile{ std::move(str) }; });
};

struct BannerTag
{
    static constexpr auto rule = [] {
        auto bannerTag = dsl::ascii::case_folding(LEXY_LIT("#banner"));
        return bannerTag >> dsl::p<TextTag>;
    }();
    static constexpr auto value = lexy::as_string<std::string> |
                                  lexy::callback<Banner>([](std::string&& str) {
                                      return Banner{ std::move(str) };
                                  });
};

struct BackBmpTag
{
    static constexpr auto rule = [] {
        auto backBmpTag = dsl::ascii::case_folding(LEXY_LIT("#backbmp"));
        return backBmpTag >> dsl::p<TextTag>;
    }();
    static constexpr auto value =
      lexy::as_string<std::string> |
      lexy::callback<BackBmp>(
        [](std::string&& str) { return BackBmp{ std::move(str) }; });
};

struct TotalTag
{
    static constexpr auto rule = [] {
        auto totalTag = dsl::ascii::case_folding(LEXY_LIT("#total"));
        return totalTag >> dsl::p<FloatingPoint>;
    }();
    static constexpr auto value =
      lexy::callback<Total>([](double num) { return Total{ num }; });
};

struct RankTag
{
    static constexpr auto rule = [] {
        auto rankTag = dsl::ascii::case_folding(LEXY_LIT("#rank"));
        return rankTag >> dsl::integer<int>(dsl::digits<>);
    }();
    static constexpr auto value =
      lexy::callback<Rank>([](int num) { return Rank{ num }; });
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

struct PlayLevelTag
{
    static constexpr auto rule = [] {
        auto playLevelTag = dsl::ascii::case_folding(LEXY_LIT("#playlevel"));
        return playLevelTag >> dsl::integer<int>(dsl::digits<>);
    }();
    static constexpr auto value =
      lexy::callback<PlayLevel>([](int num) { return PlayLevel{ num }; });
};

struct DifficultyTag
{
    static constexpr auto rule = [] {
        auto difficultyTag = dsl::ascii::case_folding(LEXY_LIT("#difficulty"));
        return difficultyTag >> dsl::integer<int>(dsl::digits<>);
    }();
    static constexpr auto value =
      lexy::callback<Difficulty>([](int num) { return Difficulty{ num }; });
};

struct WavTag
{
    static constexpr auto rule = [] {
        auto wavTag = dsl::ascii::case_folding(LEXY_LIT("#wav"));
        return wavTag >> (dsl::p<Identifier> + dsl::p<TextTag>);
    }();
    static constexpr auto value =
      lexy::callback<Wav>([](uint16_t&& identifier, std::string&& filename) {
          return Wav{ { std::move(identifier), std::move(filename) } };
      });
};

struct BmpTag
{
    static constexpr auto rule = [] {
        auto bmpTag = dsl::ascii::case_folding(LEXY_LIT("#bmp"));
        return bmpTag >> (dsl::p<Identifier> + dsl::p<TextTag>);
    }();
    static constexpr auto value =
      lexy::callback<Bmp>([](uint16_t&& identifier, std::string&& filename) {
          return Bmp{ { std::move(identifier), std::move(filename) } };
      });
};

struct LnObjTag
{
    static constexpr auto rule = [] {
        auto lnObjTag = dsl::ascii::case_folding(LEXY_LIT("#lnobj"));
        return lnObjTag >> dsl::p<Identifier>;
    }();
    static constexpr auto value = lexy::callback<LnObj>(
      [](uint16_t&& identifier) { return LnObj{ { std::move(identifier) } }; });
};

struct LnTypeTag
{
    static constexpr auto rule = [] {
        auto lnTypeTag = dsl::ascii::case_folding(LEXY_LIT("#lntype"));
        return lnTypeTag >> capture(dsl::lit_c<'1'> / dsl::lit_c<'2'>);
    }();
    static constexpr auto value = lexy::callback<LnType>(
      [](auto&& num) { return LnType{ static_cast<int>(num[0] - '0') }; });
};

struct ExBpmTag
{
    static constexpr auto rule = [] {
        auto exBpmTag =
          dsl::hash_sign + dsl::if_(dsl::ascii::case_folding(LEXY_LIT("ex"))) +
          dsl::ascii::case_folding(LEXY_LIT("bpm")) + dsl::p<Identifier>;
        return peek(exBpmTag) >> (exBpmTag + dsl::p<FloatingPoint>);
    }();
    static constexpr auto value =
      lexy::callback<ExBpm>([](uint16_t identifier, double num) {
          return ExBpm{ { std::move(identifier), num } };
      });
};

struct StopTag
{
    static constexpr auto rule = [] {
        auto stopTag =
          dsl::ascii::case_folding(LEXY_LIT("#stop")) + dsl::p<Identifier>;
        return peek(stopTag) >> (stopTag + dsl::p<FloatingPoint>);
    }();
    static constexpr auto value =
      lexy::callback<Stop>([](uint16_t identifier, double num) {
          return Stop{ { std::move(identifier), num } };
      });
};

struct MeterTag
{
    static constexpr auto rule = [] {
        auto start = (dsl::p<Measure> + LEXY_LIT("02"));
        return peek(start) >> start >> dsl::colon >> dsl::p<FloatingPoint>;
    }();
    static constexpr auto value = lexy::callback<Meter>(
      [](int64_t measure, double num) { return Meter{ { measure, num } }; });
};

struct TagsSink
{
    struct SinkCallback
    {
        using return_type = // NOLINT(readability-identifier-naming)
          ParsedBmsChart::Tags;
        ParsedBmsChart::Tags state;

        auto finish() && -> return_type { return std::move(state); }
        auto operator()(Title&& title) -> void
        {
            state.title = static_cast<std::string&&>(std::move(title));
        }
        auto operator()(Artist&& artist) -> void
        {
            state.artist = static_cast<std::string&&>(std::move(artist));
        }
        auto operator()(Subtitle&& subtitle) -> void
        {
            state.subTitle = static_cast<std::string&&>(std::move(subtitle));
        }
        auto operator()(Subartist&& subartist) -> void
        {
            state.subArtist = static_cast<std::string&&>(std::move(subartist));
        }
        auto operator()(Genre&& genre) -> void
        {
            state.genre = static_cast<std::string&&>(std::move(genre));
        }
        auto operator()(StageFile&& stageFile) -> void
        {
            state.stageFile = static_cast<std::string&&>(std::move(stageFile));
        }
        auto operator()(Banner&& banner) -> void
        {
            state.banner = static_cast<std::string&&>(std::move(banner));
        }
        auto operator()(BackBmp&& backBmp) -> void
        {
            state.backBmp = static_cast<std::string&&>(std::move(backBmp));
        }
        auto operator()(Total&& total) -> void
        {
            state.total = static_cast<double>(total);
        }
        auto operator()(Rank&& rank) -> void
        {
            state.rank = static_cast<int>(rank);
        }
        auto operator()(Bpm&& bpm) -> void
        {
            state.bpm = static_cast<double>(bpm);
        }
        auto operator()(PlayLevel&& playLevel) -> void
        {
            state.playLevel = static_cast<int>(playLevel);
        }
        auto operator()(Difficulty&& difficulty) -> void
        {
            state.difficulty = static_cast<int>(difficulty);
        }
        auto operator()(LnObj&& lnObj) -> void
        {
            state.lnObj = std::move(static_cast<uint16_t&>(lnObj));
        }
        auto operator()(LnType&& lnType) -> void
        {
            state.lnType = static_cast<int>(lnType);
        }
        auto operator()(ExBpm&& bpm) -> void
        {
            auto& [identifier, value] =
              static_cast<std::pair<uint16_t, double>&>(bpm);
            if (value != 0.0) {
                state.exBpms[identifier] = value;
            }
        }
        auto operator()(Stop&& stop) -> void
        {
            auto& [identifier, value] =
              static_cast<std::pair<uint16_t, double>&>(stop);
            if (value != 0.0) {
                state.stops[identifier] = value;
            }
        }
        auto operator()(ParsedBmsChart::Tags&& randomBlock)
        {
            state.isRandom = true;
            ParsedBmsChart::mergeTags(state, std::move(randomBlock));
        }

        auto operator()(Meter&& meter) -> void
        {
            auto [measure, value] =
              static_cast<std::pair<int64_t, double>>(meter);
            state.measures[measure].meter = value;
        }

        auto operator()(Wav&& wav) -> void
        {
            auto& [identifier, filename] =
              static_cast<std::pair<uint16_t, std::string>&>(wav);
            state.wavs[identifier] = std::move(filename);
        }

        auto operator()(Bmp&& bmp) -> void
        {
            auto& [identifier, filename] =
              static_cast<std::pair<uint16_t, std::string>&>(bmp);
            state.bmps[identifier] = std::move(filename);
        }

        auto operator()(
          std::tuple<int64_t, int, std::vector<uint16_t>>&& measureBasedTag)
          -> void
        {
            auto [measure, channel, identifiers] = std::move(measureBasedTag);

            enum ChannelCategory
            {
                General = 0,
                P1Visible = 1,
                P2Visible = 2,
                P1Invisible = 3,
                P2Invisible = 4,
                P1LongNote = 5,
                P2LongNote = 6,
                P1Landmine = 0xD,
                P2Landmine = 0xE
            };
            enum GeneralSubcategory
            {
                Bgm = 1,
                /* Meter = 2, // handled elsewhere */
                Bpm = 3,
                BgaBase = 4, // unimplemented
                ExtendedObject = 5,
                BgaPoor = 6,
                BgaLayer = 7,
                ExBpm = 8,
                Stop = 9,
                BgaLayer2 = 0xA
            };
            constexpr auto base = 36;
            auto channelCategory = static_cast<ChannelCategory>(channel / base);
            auto channelSubcategory = static_cast<unsigned>(channel % base);

            auto addNotes = [](auto& noteArray,
                               unsigned column,
                               std::vector<uint16_t> identifiers) {
                if (column < 1 || column > noteArray.size()) [[unlikely]] {
                    return;
                }
                noteArray[column - 1].push_back(std::move(identifiers));
            };

            switch (channelCategory) {
                case General:
                    switch (channelSubcategory) {
                        case Bgm:
                            state.measures[measure].bgmNotes.push_back(
                              std::move(identifiers));
                            break;
                        case Bpm:
                            std::ranges::transform(
                              identifiers,
                              identifiers.begin(),
                              [](auto identifier) -> uint16_t {
                                  // convert from base-36 identifier to base-16
                                  auto bpmValueHigh = identifier / 36;
                                  if (bpmValueHigh >= 16) [[unlikely]] {
                                      return 0;
                                  }
                                  bpmValueHigh *= 16;
                                  auto bpmValueLow = identifier % 36;
                                  if (bpmValueLow >= 16) [[unlikely]] {
                                      return 0;
                                  }
                                  return bpmValueHigh + bpmValueLow;
                              });
                            state.measures[measure].bpmChanges =
                              std::move(identifiers);
                            break;
                        case ExBpm:
                            state.measures[measure].exBpmChanges =
                              std::move(identifiers);
                            break;
                        case Stop:
                            state.measures[measure].stops =
                              std::move(identifiers);
                            break;
                        case BgaBase:
                            state.measures[measure].bgaBase.push_back(
                              std::move(identifiers));
                            break;
                        case BgaPoor:
                            state.measures[measure].bgaPoor.push_back(
                              std::move(identifiers));
                            break;
                        case BgaLayer:
                            state.measures[measure].bgaLayer.push_back(
                              std::move(identifiers));
                            break;
                        case BgaLayer2:
                            state.measures[measure].bgaLayer2.push_back(
                              std::move(identifiers));
                            break;
                        default:
                            spdlog::debug("Unknown channel: {:02d}", channel);
                            break;
                    }
                    break;
                case P1Visible:
                    addNotes(state.measures[measure].p1VisibleNotes,
                             channelSubcategory,
                             std::move(identifiers));
                    break;
                case P2Visible:
                    addNotes(state.measures[measure].p2VisibleNotes,
                             channelSubcategory,
                             std::move(identifiers));
                    break;
                case P1Invisible:
                    addNotes(state.measures[measure].p1InvisibleNotes,
                             channelSubcategory,
                             std::move(identifiers));
                    break;
                case P2Invisible:
                    addNotes(state.measures[measure].p2InvisibleNotes,
                             channelSubcategory,
                             std::move(identifiers));
                    break;
                case P1LongNote:
                    addNotes(state.measures[measure].p1LongNotes,
                             channelSubcategory,
                             std::move(identifiers));
                    break;
                case P2LongNote:
                    addNotes(state.measures[measure].p2LongNotes,
                             channelSubcategory,
                             std::move(identifiers));
                    break;
                case P1Landmine:
                    addNotes(state.measures[measure].p1Landmines,
                             channelSubcategory,
                             std::move(identifiers));
                    break;
                case P2Landmine:
                    addNotes(state.measures[measure].p2Landmines,
                             channelSubcategory,
                             std::move(identifiers));
                    break;
                default:
                    spdlog::debug("Unknown channel: {:02d}", channel);
                    break;
            }
        }
    };
    [[nodiscard]] auto sink() const -> SinkCallback { return SinkCallback{}; }
};

struct RandomBlock;

struct MainTags
{
    static constexpr auto whitespace =
      dsl::whitespace(dsl::unicode::space - dsl::unicode::newline);
    static constexpr auto rule = [] {
        auto term = terminator(
          dsl::eof | peek(dsl::ascii::case_folding(LEXY_LIT("#endif"))));
        return term.list(try_(
          dsl::unicode::newline | dsl::p<MeterTag> | dsl::p<MeasureBasedTag> |
            dsl::p<WavTag> | dsl::p<BmpTag> | dsl::p<ExBpmTag> |
            dsl::p<StopTag> | dsl::p<TitleTag> | dsl::p<ArtistTag> |
            dsl::p<GenreTag> | dsl::p<StageFileTag> | dsl::p<BannerTag> |
            dsl::p<BackBmpTag> | dsl::p<SubtitleTag> | dsl::p<SubartistTag> |
            dsl::p<TotalTag> | dsl::p<RankTag> | dsl::p<PlayLevelTag> |
            dsl::p<DifficultyTag> | dsl::p<BpmTag> | dsl::p<LnObjTag> |
            dsl::p<LnTypeTag> | dsl::recurse_branch<RandomBlock>,
          until(dsl::unicode::newline).or_eof()));
    }();
    static constexpr auto value = TagsSink{};
};

struct OrphanizedRandomCommonPart
{
    static constexpr auto rule = [] {
        auto term = dsl::terminator(
          dsl::eof | peek(dsl::ascii::case_folding(LEXY_LIT("#endrandom"))) |
          peek(dsl::ascii::case_folding(LEXY_LIT("#random"))) |
          peek(dsl::ascii::case_folding(LEXY_LIT("#if"))));
        return dsl::peek_not(dsl::ascii::case_folding(LEXY_LIT("#if"))) >>
               term.list(try_(
                 dsl::unicode::newline | dsl::p<MeterTag> |
                   dsl::p<MeasureBasedTag> | dsl::p<WavTag> | dsl::p<BmpTag> |
                   dsl::p<ExBpmTag> | dsl::p<StopTag> | dsl::p<TitleTag> |
                   dsl::p<ArtistTag> | dsl::p<GenreTag> | dsl::p<StageFileTag> |
                   dsl::p<BannerTag> | dsl::p<BackBmpTag> |
                   dsl::p<SubtitleTag> | dsl::p<SubartistTag> |
                   dsl::p<TotalTag> | dsl::p<RankTag> | dsl::p<PlayLevelTag> |
                   dsl::p<DifficultyTag> | dsl::p<BpmTag> | dsl::p<LnObjTag> |
                   dsl::p<LnTypeTag>,
                 until(dsl::unicode::newline).or_eof()));
    }();
    static constexpr auto value = TagsSink{};
};

struct IfData
{
    ParsedBmsChart::RandomRange number;
    ParsedBmsChart::Tags tags;
};

struct IfBlock
{
    static constexpr auto rule =
      dsl::ascii::case_folding(LEXY_LIT("#if")) >>
      (dsl::integer<ParsedBmsChart::RandomRange>(dsl::digits<>) +
       dsl::p<MainTags> + dsl::ascii::case_folding(LEXY_LIT("#endif")));
    static constexpr auto value = lexy::construct<IfData>;
};

struct RandomSink
{
    struct RandomSinkCallback
    {
        using return_type = // NOLINT(readability-identifier-naming)
          std::vector<std::variant<IfData, ParsedBmsChart::Tags>>;
        return_type state;

        auto finish() && -> return_type { return std::move(state); }
        auto operator()(IfData&& ifBlock) -> void
        {
            state.push_back(std::move(ifBlock));
        }
        auto operator()(ParsedBmsChart::Tags&& orphanTags) -> void
        {
            state.push_back(std::move(orphanTags));
        }
    };
    [[nodiscard]] auto sink() const -> RandomSinkCallback
    {
        return RandomSinkCallback{};
    }
};

struct IfList
{
    static constexpr auto rule = [] {
        auto delims = dsl::terminator(
          dsl::eof | dsl::peek(dsl::ascii::case_folding(LEXY_LIT("#random"))) |
          dsl::peek(dsl::ascii::case_folding(LEXY_LIT("#endrandom"))));
        return delims.list(dsl::p<IfBlock> |
                           dsl::p<OrphanizedRandomCommonPart>);
    }();
    static constexpr auto value = RandomSink{};
};

auto
resolveIfs(
  ParsedBmsChart::RandomRange randomRange,
  std::vector<std::variant<IfData, ParsedBmsChart::Tags>> randomContents,
  const ParseState& parseState) -> ParsedBmsChart::Tags
{
    auto tags = ParsedBmsChart::Tags{};
    auto randomNumber = parseState.randomGenerator(randomRange);
    for (auto& randomContent : randomContents) {
        if (std::holds_alternative<IfData>(randomContent)) {
            auto& ifData = std::get<IfData>(randomContent);
            if (ifData.number == randomNumber) {
                ParsedBmsChart::mergeTags(tags, std::move(ifData.tags));
            }
        } else {
            ParsedBmsChart::mergeTags(
              tags, std::move(std::get<ParsedBmsChart::Tags>(randomContent)));
        }
    }
    return tags;
}

struct RandomBlock
{
    static constexpr auto rule =
      dsl::ascii::case_folding(LEXY_LIT("#random")) >>
      (dsl::integer<ParsedBmsChart::RandomRange>(dsl::digits<>) +
       dsl::p<IfList> +
       (dsl::peek(dsl::ascii::case_folding(LEXY_LIT("#random"))) |
        dsl::ascii::case_folding(LEXY_LIT("#endrandom")) | dsl::eof));
    static constexpr auto value =
      lexy::bind(lexy::callback<ParsedBmsChart::Tags>(resolveIfs),
                 lexy::values,
                 lexy::parse_state);
};

struct ReportError
{
    using return_type = void;
    template<typename... Args>
    void operator()(Args&&...) const
    {
    }
};

auto
getEncodingFromBom(std::string_view chart) -> const char*
{
    if (chart.size() >= 3 && chart[0] == '\xEF' && chart[1] == '\xBB' &&
        chart[2] == '\xBF') {
        chart.remove_prefix(3); // remove BOM
        return "UTF-8";
    }
    if (chart.size() >= 2 && chart[0] == '\xFF' && chart[1] == '\xFE') {
        chart.remove_prefix(2); // remove UTF-16 LE BOM
        return "UTF-16LE";
    }
    if (chart.size() >= 2 && chart[0] == '\xFE' && chart[1] == '\xFF') {
        chart.remove_prefix(2); // remove UTF-16 BE BOM
        return "UTF-16BE";
    }
    if (chart.size() >= 1 && chart[0] == '\xEF') {
        // UTF-8 BOM is optional, so we don't throw an error here
        return "UTF-8";
    }
    if (chart.size() >= 4 && chart[0] == '\x00' && chart[1] == '\x00' &&
        chart[2] == '\xFE' && chart[3] == '\xFF') {
        chart.remove_prefix(4); // remove UTF-32 BE BOM
        return "UTF-32BE";
    }
    if (chart.size() >= 4 && chart[0] == '\xFF' && chart[1] == '\xFE' &&
        chart[2] == '\x00' && chart[3] == '\x00') {
        chart.remove_prefix(4); // remove UTF-32 LE BOM
        return "UTF-32LE";
    }
    return nullptr;
}
auto
readBmsChart(
  const std::string_view chart,
  const std::function<ParsedBmsChart::RandomRange(ParsedBmsChart::RandomRange)>&
    randomGenerator) -> ParsedBmsChart
{

    const char* encoding = getEncodingFromBom(chart);
    using namespace std::string_literals;
    if (encoding != nullptr && encoding != "UTF-8"s) {
        throw std::runtime_error(
          fmt::format("Encoding {} is unsupported", encoding));
    }
    auto result = lexy::parse<MainTags>(
      lexy::string_input<lexy::utf8_char_encoding>(chart),
      ParseState{ randomGenerator, encoding && encoding == "UTF-8"s },
      ReportError{});
    return ParsedBmsChart{ std::move(result).value() };
}
} // namespace charts

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
#include "BmsChartReader.h"

#include <lexy_ext/report_error.hpp>

#define TO_CHARS(STRLEN, STR) RHYTHMGAME_TO_CHARS(STRLEN, STR)

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
        auto value = limits(dsl::code_point);
        return value;
    }();
};

struct FloatingPoint
{
    static constexpr auto rule = [] {
        auto integer_part = dsl::sign + dsl::digits<>;
        auto fraction = dsl::period >> dsl::if_(dsl::digits<>);
        auto exponent =
          dsl::lit_c<'e'> / dsl::lit_c<'E'> >> dsl::sign + dsl::digits<>;
        auto suffix =
          dsl::lit_c<'f'> / dsl::lit_c<'F'> / dsl::lit_c<'d'> / dsl::lit_c<'D'>;

        auto real_number = dsl::token(integer_part + dsl::if_(fraction) +
                                      dsl::if_(exponent) + dsl::if_(suffix));
        return dsl::capture(real_number);
    }();

    static constexpr auto value =
      lexy::as_string<std::string> |
      lexy::callback<double>([](std::string&& str) { return std::stod(str); });
};

struct Production
{
    static constexpr auto whitespace =
      dsl::whitespace(dsl::lit_c<' '> | dsl::lit_c<'\t'> | dsl::lit_c<'\r'> |
                      dsl::lit_c<'\n'> | LEXY_LIT("\r\n"));
    static constexpr auto rule = [] {
        auto titleTag = dsl::ascii::case_folding(LEXY_LIT("#title"));
        auto artistTag = dsl::ascii::case_folding(LEXY_LIT("#artist"));
        auto bpmTag = dsl::ascii::case_folding(LEXY_LIT("#bpm"));
        return dsl::list(
                 titleTag >> (dsl::member<&models::BmsChart::Tags::title> =
                                dsl::p<TextTag>) |
                 artistTag >> (dsl::member<&models::BmsChart::Tags::artist> =
                                 dsl::p<TextTag>) |
                 bpmTag >> (dsl::member<&models::BmsChart::Tags::bpm> =
                              dsl::p<FloatingPoint>)) +
               dsl::eof;
    }();
    static constexpr auto value = lexy::as_aggregate<models::BmsChart::Tags>;
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

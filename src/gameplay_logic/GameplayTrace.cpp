#include "GameplayTrace.h"

#include <QJsonArray>
#include <QJsonDocument>

#include <cmath>
#include <limits>
#include <stdexcept>

namespace gameplay_logic {
namespace {

auto
jsonString(const QString& value) -> QByteArray
{
    auto encoded = QJsonDocument{ QJsonArray{ value } }.toJson(
      QJsonDocument::Compact);
    return encoded.mid(1, encoded.size() - 2);
}

auto
number(const double value) -> QByteArray
{
    if (!std::isfinite(value)) {
        throw std::invalid_argument(
          "Gameplay trace cannot contain non-finite numbers");
    }
    if (value == 0.0) {
        return QByteArrayLiteral("0");
    }
    return QByteArray::number(value, 'g', 17);
}

template<typename Range, typename Function>
void
appendArray(QByteArray& json, const Range& values, Function appendValue)
{
    json += '[';
    auto first = true;
    for (const auto& value : values) {
        if (!first) {
            json += ',';
        }
        first = false;
        appendValue(json, value);
    }
    json += ']';
}

auto
actionName(const HitEvent::Action action) -> QByteArray
{
    switch (action) {
        case HitEvent::Action::None:
            return QByteArrayLiteral("\"none\"");
        case HitEvent::Action::Press:
            return QByteArrayLiteral("\"press\"");
        case HitEvent::Action::Release:
            return QByteArrayLiteral("\"release\"");
    }
    throw std::invalid_argument("Invalid gameplay trace action");
}

void
appendOptionalInteger(QByteArray& json,
                      const std::optional<std::int64_t> value)
{
    json += value ? QByteArray::number(*value) : QByteArrayLiteral("null");
}

void
appendOptionalDouble(QByteArray& json, const std::optional<double> value)
{
    json += value ? number(*value) : QByteArrayLiteral("null");
}

} // namespace

auto
GameplayTrace::toCanonicalJson() const -> QByteArray
{
    auto json = QByteArray{};
    json.reserve(2048);
    json += "{\"schemaVersion\":1,\"chart\":{\"sha256\":";
    json += jsonString(sha256);
    json += ",\"md5\":";
    json += jsonString(md5);
    json += "},\"play\":{\"randomSequence\":";
    appendArray(json, randomSequence, [](auto& out, const auto value) {
        out += QByteArray::number(value);
    });
    json += ",\"permutation\":";
    appendArray(json, permutation, [](auto& out, const auto value) {
        out += QByteArray::number(value);
    });
    json += ",\"laneSeed\":";
    json += QByteArray::number(laneSeed);
    json += ",\"noteOrderP1\":";
    json += QByteArray::number(static_cast<int>(noteOrderP1));
    json += ",\"noteOrderP2\":";
    json += QByteArray::number(static_cast<int>(noteOrderP2));
    json += ",\"dpMode\":";
    json += QByteArray::number(static_cast<int>(dpMode));
    json += "},\"inputs\":";
    appendArray(json, inputs, [](auto& out, const auto& input) {
        out += "{\"timeNs\":";
        out += QByteArray::number(input.chartTimeNs);
        out += ",\"key\":";
        out += QByteArray::number(input.key);
        out += ",\"action\":";
        out += actionName(input.action);
        out += '}';
    });
    json += ",\"judgements\":";
    appendArray(json, judgements, [](auto& out, const auto& judgement) {
        out += "{\"timeNs\":";
        out += QByteArray::number(judgement.chartTimeNs);
        out += ",\"hitOffsetNs\":";
        out += QByteArray::number(judgement.hitOffsetNs);
        out += ",\"column\":";
        out += QByteArray::number(judgement.column);
        out += ",\"key\":";
        out += QByteArray::number(judgement.key);
        out += ",\"noteIndex\":";
        out += QByteArray::number(judgement.noteIndex);
        out += ",\"action\":";
        out += actionName(judgement.action);
        out += ",\"noteRemoved\":";
        out += judgement.noteRemoved ? "true" : "false";
        out += ",\"judgement\":";
        if (judgement.judgement) {
            out += QByteArray::number(static_cast<int>(*judgement.judgement));
        } else {
            out += "null";
        }
        out += ",\"deviationNs\":";
        appendOptionalInteger(out, judgement.deviationNs);
        out += ",\"value\":";
        appendOptionalDouble(out, judgement.value);
        out += '}';
    });
    json += ",\"gaugeSamples\":";
    appendArray(json, gaugeSamples, [](auto& out, const auto& sample) {
        out += "{\"timeNs\":";
        out += QByteArray::number(sample.chartTimeNs);
        out += ",\"value\":";
        out += number(sample.value);
        out += '}';
    });
    json += ",\"result\":{\"points\":";
    json += number(result.points);
    json += ",\"maxPoints\":";
    json += number(result.maxPoints);
    json += ",\"maxPointsNow\":";
    json += number(result.maxPointsNow);
    json += ",\"gauge\":";
    json += number(result.gauge);
    json += ",\"combo\":";
    json += QByteArray::number(result.combo);
    json += ",\"maxCombo\":";
    json += QByteArray::number(result.maxCombo);
    json += ",\"mineHits\":";
    json += QByteArray::number(result.mineHits);
    json += ",\"clearType\":";
    json += jsonString(result.clearType);
    json += ",\"judgementCounts\":";
    appendArray(json, result.judgementCounts, [](auto& out, const auto value) {
        out += QByteArray::number(value);
    });
    json += ",\"savedTimestampSeconds\":";
    json += QByteArray::number(result.savedTimestampSeconds);
    json += ",\"scoreGuid\":";
    json += jsonString(result.scoreGuid);
    json += ",\"chartLengthNs\":";
    json += QByteArray::number(result.chartLengthNs);
    json += ",\"keymode\":";
    json += QByteArray::number(result.keymode);
    json += ",\"dpMode\":";
    json += QByteArray::number(result.dpMode);
    json += "}}";
    return json;
}

} // namespace gameplay_logic

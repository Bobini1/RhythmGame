#include "QtArenaGameplaySource.h"

#include "gameplay_logic/BmsGaugeHistory.h"
#include "gameplay_logic/BmsLiveScore.h"
#include "gameplay_logic/BmsResult.h"
#include "gameplay_logic/BmsScore.h"
#include "gameplay_logic/ChartRunner.h"
#include "gameplay_logic/Judgement.h"
#include "gameplay_logic/rules/BmsGauge.h"

#include <QThread>

#include <algorithm>
#include <cmath>
#include <optional>
#include <tuple>

namespace arena {
namespace {

constexpr qint64 MaxGaugeMilli = 100'000;

auto
boundedInteger(const double value) -> std::optional<qint64>
{
    if (!std::isfinite(value) || value < 0.0 || value > MaxScoreCounter ||
        std::trunc(value) != value) {
        return std::nullopt;
    }
    return static_cast<qint64>(value);
}

auto
boundedCounter(const qint64 value) -> bool
{
    return value >= 0 && value <= MaxScoreCounter;
}

auto
gaugeType(const QStringView name) -> std::optional<GaugeType>
{
    if (name == u"FC") {
        return GaugeType::Fc;
    }
    if (name == u"EXHARD") {
        return GaugeType::ExHard;
    }
    if (name == u"HARD") {
        return GaugeType::Hard;
    }
    if (name == u"NORMAL") {
        return GaugeType::Normal;
    }
    if (name == u"EASY") {
        return GaugeType::Easy;
    }
    if (name == u"AEASY") {
        return GaugeType::AssistEasy;
    }
    return std::nullopt;
}

auto
clearType(const QStringView name) -> std::optional<ClearType>
{
    if (name == u"MAX") {
        return ClearType::Max;
    }
    if (name == u"PERFECT") {
        return ClearType::Perfect;
    }
    if (name == u"FC") {
        return ClearType::FullCombo;
    }
    if (name == u"EXHARD") {
        return ClearType::ExHard;
    }
    if (name == u"HARD") {
        return ClearType::Hard;
    }
    if (name == u"NORMAL") {
        return ClearType::Normal;
    }
    if (name == u"EASY") {
        return ClearType::Easy;
    }
    if (name == u"AEASY") {
        return ClearType::AssistEasy;
    }
    if (name == u"FAILED") {
        return ClearType::Failed;
    }
    return std::nullopt;
}

auto
judgementsFrom(const QList<int>& counts)
  -> std::expected<ArenaJudgements, ArenaGameplayCaptureFailure>
{
    const auto count =
      [&counts](
        const gameplay_logic::Judgement judgement) -> std::optional<qint64> {
        const auto index = static_cast<qsizetype>(judgement);
        if (index < 0 || index >= counts.size()) {
            return std::nullopt;
        }
        const auto value = static_cast<qint64>(counts[index]);
        return boundedCounter(value) ? std::optional{ value } : std::nullopt;
    };
    const auto perfect = count(gameplay_logic::Judgement::Perfect);
    const auto great = count(gameplay_logic::Judgement::Great);
    const auto good = count(gameplay_logic::Judgement::Good);
    const auto bad = count(gameplay_logic::Judgement::Bad);
    const auto poor = count(gameplay_logic::Judgement::Poor);
    const auto emptyPoor = count(gameplay_logic::Judgement::EmptyPoor);
    if (!perfect || !great || !good || !bad || !poor || !emptyPoor) {
        return std::unexpected(ArenaGameplayCaptureFailure::InvalidNumber);
    }
    return ArenaJudgements{ .perfect = *perfect,
                            .great = *great,
                            .good = *good,
                            .bad = *bad,
                            .poor = *poor,
                            .emptyPoor = *emptyPoor };
}

auto
validatedCounters(const double points,
                  const int maxCombo,
                  const QList<int>& counts)
  -> std::expected<std::tuple<qint64, qint64, qint64, ArenaJudgements>,
                   ArenaGameplayCaptureFailure>
{
    const auto exScore = boundedInteger(points);
    const auto combo = static_cast<qint64>(maxCombo);
    const auto judgements = judgementsFrom(counts);
    if (!exScore || !boundedCounter(combo) || !judgements) {
        return std::unexpected(ArenaGameplayCaptureFailure::InvalidNumber);
    }
    const auto expectedEx = 2 * judgements->perfect + judgements->great;
    const auto badPoor =
      judgements->bad + judgements->poor + judgements->emptyPoor;
    if (*exScore != expectedEx || !boundedCounter(expectedEx) ||
        !boundedCounter(badPoor)) {
        return std::unexpected(ArenaGameplayCaptureFailure::InvalidNumber);
    }
    return std::tuple{ *exScore, combo, badPoor, *judgements };
}

auto
normalizedGauge(const QStringView name,
                const double value,
                const double maximum)
  -> std::expected<GaugeSnapshot, ArenaGameplayCaptureFailure>
{
    const auto type = gaugeType(name);
    if (!type) {
        return std::unexpected(ArenaGameplayCaptureFailure::UnsupportedGauge);
    }
    if (!std::isfinite(value) || !std::isfinite(maximum) || maximum <= 0.0) {
        return std::unexpected(ArenaGameplayCaptureFailure::InvalidNumber);
    }
    const auto ratio = std::clamp(value / maximum, 0.0, 1.0);
    const auto milli = static_cast<qint64>(std::llround(ratio * MaxGaugeMilli));
    return GaugeSnapshot{ .type = *type, .valueMilli = milli };
}

auto
liveGauge(const QList<gameplay_logic::rules::BmsGauge*>& gauges)
  -> std::expected<GaugeSnapshot, ArenaGameplayCaptureFailure>
{
    if (gauges.isEmpty()) {
        return std::unexpected(ArenaGameplayCaptureFailure::UnsupportedGauge);
    }
    gameplay_logic::rules::BmsGauge* selected = nullptr;
    for (auto* gauge : gauges) {
        if (gauge == nullptr || !std::isfinite(gauge->getGauge()) ||
            !std::isfinite(gauge->getThreshold())) {
            return std::unexpected(ArenaGameplayCaptureFailure::InvalidNumber);
        }
        selected = gauge;
        if (gauge->getGauge() > gauge->getThreshold()) {
            break;
        }
    }
    return normalizedGauge(
      selected->getName(), selected->getGauge(), selected->getGaugeMax());
}

auto
finalGauge(const QList<gameplay_logic::BmsGaugeInfo>& gauges)
  -> std::expected<GaugeSnapshot, ArenaGameplayCaptureFailure>
{
    if (gauges.isEmpty()) {
        return std::unexpected(ArenaGameplayCaptureFailure::InvalidResult);
    }
    const gameplay_logic::BmsGaugeInfo* selected = nullptr;
    double selectedValue{};
    for (const auto& gauge : gauges) {
        if (gauge.gaugeHistory.isEmpty()) {
            return std::unexpected(ArenaGameplayCaptureFailure::InvalidResult);
        }
        const auto value = gauge.gaugeHistory.back().getGauge();
        if (!std::isfinite(value) || !std::isfinite(gauge.threshold)) {
            return std::unexpected(ArenaGameplayCaptureFailure::InvalidNumber);
        }
        selected = &gauge;
        selectedValue = value;
        if (value > gauge.threshold) {
            break;
        }
    }
    return normalizedGauge(selected->name, selectedValue, selected->maxGauge);
}

auto
onObjectThread(const QObject* object) -> bool
{
    return object != nullptr && object->thread() == QThread::currentThread();
}

} // namespace

auto
QtArenaGameplaySource::attach(gameplay_logic::ChartRunner* runner)
  -> std::expected<QString, ArenaGameplayCaptureFailure>
{
    detach();
    if (!onObjectThread(this) || !onObjectThread(runner) ||
        runner->getPlayer1() == nullptr ||
        runner->getPlayer1()->getScore() == nullptr) {
        return std::unexpected(ArenaGameplayCaptureFailure::NoRunner);
    }
    const auto guid = runner->getPlayer1()->getScore()->getGuid();
    if (guid.isEmpty()) {
        return std::unexpected(ArenaGameplayCaptureFailure::InvalidResult);
    }
    m_runner = runner;
    m_expectedScoreGuid = guid;
    return guid;
}

void
QtArenaGameplaySource::detach()
{
    m_runner.clear();
    m_expectedScoreGuid.clear();
}

auto
QtArenaGameplaySource::sample(const quint32 sequence) const
  -> std::expected<TelemetrySnapshot, ArenaGameplayCaptureFailure>
{
    if (!onObjectThread(this) || m_runner.isNull() ||
        !onObjectThread(m_runner.data()) || m_runner->getPlayer1() == nullptr ||
        m_runner->getPlayer1()->getScore() == nullptr) {
        return std::unexpected(ArenaGameplayCaptureFailure::NoRunner);
    }
    if (sequence == 0) {
        return std::unexpected(ArenaGameplayCaptureFailure::InvalidNumber);
    }
    const auto* player = m_runner->getPlayer1();
    const auto* score = player->getScore();
    const auto counters =
      validatedCounters(score->getPoints(),
                        score->getMaxCombo(),
                        score->getJudgementCounts()->getJudgementCounts());
    if (!counters) {
        return std::unexpected(counters.error());
    }
    const auto gauge = liveGauge(score->getGauges());
    if (!gauge) {
        return std::unexpected(gauge.error());
    }
    const auto length = player->getChartLength();
    if (length < 0) {
        return std::unexpected(ArenaGameplayCaptureFailure::InvalidNumber);
    }
    qint64 progress{};
    if (length > 0) {
        const auto ratio =
          std::clamp(static_cast<long double>(player->getElapsed()) /
                       static_cast<long double>(length),
                     0.0L,
                     1.0L);
        progress = static_cast<qint64>(std::llround(ratio * 1000.0L));
    }
    const auto& [exScore, maxCombo, badPoor, judgements] = *counters;
    return TelemetrySnapshot{ .sequence = static_cast<qint64>(sequence),
                              .exScore = exScore,
                              .progressPermille = progress,
                              .maxCombo = maxCombo,
                              .badPoorCount = badPoor,
                              .judgements = judgements,
                              .gauge = *gauge };
}

auto
QtArenaGameplaySource::captureFinal(gameplay_logic::BmsScore* score) const
  -> std::expected<FinalResult, ArenaGameplayCaptureFailure>
{
    if (!onObjectThread(this) || m_expectedScoreGuid.isEmpty()) {
        return std::unexpected(ArenaGameplayCaptureFailure::NoRunner);
    }
    if (!onObjectThread(score) || score->getResult() == nullptr ||
        score->getGaugeHistory() == nullptr) {
        return std::unexpected(ArenaGameplayCaptureFailure::InvalidResult);
    }
    const auto* result = score->getResult();
    if (result->getGuid() != m_expectedScoreGuid ||
        score->getGaugeHistory()->getGuid() != m_expectedScoreGuid) {
        return std::unexpected(ArenaGameplayCaptureFailure::WrongScore);
    }
    const auto counters = validatedCounters(
      result->getPoints(), result->getMaxCombo(), result->getJudgementCounts());
    if (!counters) {
        return std::unexpected(counters.error());
    }
    const auto mappedClear = clearType(result->getClearType());
    if (!mappedClear) {
        return std::unexpected(ArenaGameplayCaptureFailure::InvalidResult);
    }
    const auto gauge = finalGauge(score->getGaugeHistory()->getGaugeInfo());
    if (!gauge) {
        return std::unexpected(gauge.error());
    }
    const auto& [exScore, maxCombo, badPoor, judgements] = *counters;
    return FinalResult{ .exScore = exScore,
                        .maxCombo = maxCombo,
                        .badPoorCount = badPoor,
                        .judgements = judgements,
                        .clearType = *mappedClear,
                        .finalGauge = *gauge };
}

} // namespace arena

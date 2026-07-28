#include "SinglePlayerGameplayCore.h"

#include "BmsGameReferee.h"
#include "BmsLiveScore.h"
#include "SinglePlayerChartBuilder.h"
#include "resource_managers/ChartDataFactory.h"
#include "rules/HitRules.h"
#include "rules/Lr2Gauge.h"
#include "rules/Lr2HitValues.h"
#include "rules/Lr2TimingWindows.h"

#include <QVariant>

#include <algorithm>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace gameplay_logic {
namespace {

using namespace std::chrono_literals;

constexpr auto renderBehind = 2.0;
constexpr auto renderAhead = 16.0;

auto
asPlayerBuildOptions(const resource_managers::ChartPlayConfig& config)
  -> PlayerChartBuildOptions
{
    return {
        .noteOrderP1 = config.noteOrderP1,
        .noteOrderP2 = config.noteOrderP2,
        .dpOptions = config.dpMode,
        .randomSeed = config.laneSeed,
    };
}

auto
makeNormalGauge(const ChartData& chartData, const PlayerChartBuildResult& chart)
  -> QList<rules::BmsGauge*>
{
    const auto multiplier =
      chart.effectiveDpOptions == resource_managers::DpOptions::Battle ? 2 : 1;
    const auto hitCount =
      (chartData.getNormalNoteCount() + chartData.getScratchCount() +
       chartData.getLnCount() + chartData.getBssCount()) *
      multiplier;
    if (hitCount <= 0) {
        throw std::invalid_argument(
          "The gameplay core requires a chart with at least one hittable note");
    }
    auto gauges = rules::Lr2Gauge::getGauges(chartData.getTotal(), hitCount);
    const auto normal = std::ranges::find_if(gauges, [](const auto& gauge) {
        return gauge->getName() == QStringLiteral("NORMAL");
    });
    if (normal == gauges.end()) {
        throw std::logic_error("LR2 NORMAL gauge is unavailable");
    }
    auto* normalGauge = normal->release();
    return { normalGauge };
}

auto
makeScore(const ChartData& chartData,
          const PlayerChartBuildResult& chart,
          const GameplayCoreConfig& config,
          QList<rules::BmsGauge*> gauges) -> std::unique_ptr<BmsLiveScore>
{
    const auto multiplier =
      chart.effectiveDpOptions == resource_managers::DpOptions::Battle ? 2 : 1;
    return std::make_unique<BmsLiveScore>(
      chartData.getNormalNoteCount() * multiplier,
      chartData.getScratchCount() * multiplier,
      chartData.getLnCount() * multiplier,
      chartData.getBssCount() * multiplier,
      chartData.getMineCount() * multiplier,
      (chartData.getNormalNoteCount() + chartData.getScratchCount() +
       chartData.getLnCount() + chartData.getBssCount()) *
        multiplier,
      config.maxHitValue,
      std::move(gauges),
      chartData.getRandomSequence(),
      config.play.noteOrderP1,
      isDp(chart.effectiveKeymode)
        ? config.play.noteOrderP2
        : resource_managers::NoteOrderAlgorithm::Normal,
      chart.effectiveDpOptions,
      chart.storedPermutation(),
      chart.storedSeed(),
      chartData.getLength(),
      chartData.getSha256(),
      chartData.getMd5(),
      chart.effectiveKeymode,
      config.savedTimestampSeconds,
      config.scoreGuid);
}

auto
stableId(const std::size_t column, const std::int64_t noteIndex)
  -> std::uint32_t
{
    if (column > std::numeric_limits<std::uint8_t>::max() || noteIndex < 0 ||
        noteIndex > std::numeric_limits<std::uint16_t>::max()) {
        throw std::overflow_error("Visible-note identity is out of range");
    }
    return (static_cast<std::uint32_t>(column) << 16U) |
           static_cast<std::uint32_t>(noteIndex);
}

auto
isLongNote(const charts::BmsNotesData::NoteType type) -> bool
{
    return type == charts::BmsNotesData::NoteType::LongNoteBegin ||
           type == charts::BmsNotesData::NoteType::LongNoteEnd;
}

auto
pairedLongNoteScrollPosition(
  const std::vector<charts::BmsNotesData::Note>& notes,
  const std::size_t noteIndex) -> std::optional<double>
{
    const auto type = notes.at(noteIndex).noteType;
    if (type == charts::BmsNotesData::NoteType::LongNoteBegin) {
        const auto pairedIndex = noteIndex + 1;
        if (pairedIndex >= notes.size() ||
            notes[pairedIndex].noteType !=
              charts::BmsNotesData::NoteType::LongNoteEnd) {
            return std::nullopt;
        }
        return notes[pairedIndex].time.position;
    }
    if (type == charts::BmsNotesData::NoteType::LongNoteEnd) {
        if (noteIndex == 0 || notes[noteIndex - 1].noteType !=
                                charts::BmsNotesData::NoteType::LongNoteBegin) {
            return std::nullopt;
        }
        return notes[noteIndex - 1].time.position;
    }
    return std::nullopt;
}

auto
intersectsRenderWindow(const double scrollPosition,
                       const std::optional<double> pairedScrollPosition,
                       const double renderBottom,
                       const double renderTop) -> bool
{
    const auto segmentBottom =
      pairedScrollPosition ? std::min(scrollPosition, *pairedScrollPosition)
                           : scrollPosition;
    const auto segmentTop = pairedScrollPosition
                              ? std::max(scrollPosition, *pairedScrollPosition)
                              : scrollPosition;
    return !(segmentTop < renderBottom || segmentBottom > renderTop);
}

auto
hasRemovedHit(const QVariant& hitData) -> bool
{
    return !hitData.isNull() && hitData.value<HitEvent>().getNoteRemoved();
}

auto
isPairHolding(const NoteState& noteState,
              const charts::BmsNotesData::NoteType type,
              const bool laneHolding) -> bool
{
    if (!laneHolding || !isLongNote(type)) {
        return false;
    }
    if (type == charts::BmsNotesData::NoteType::LongNoteBegin) {
        return hasRemovedHit(noteState.hitData) &&
               !hasRemovedHit(noteState.otherEndHitData);
    }
    return hasRemovedHit(noteState.otherEndHitData) &&
           !hasRemovedHit(noteState.hitData);
}

} // namespace

class SinglePlayerGameplayCore::Impl
{
  public:
    GameplayCoreConfig config;
    std::unique_ptr<ChartData> chartData;
    charts::BmsNotesData notesData;
    PlayerChartBuildResult chart;
    std::unique_ptr<BmsLiveScore> score;
    std::unique_ptr<BmsGameReferee> referee;
    std::optional<std::chrono::nanoseconds> lastCommandTime;
    std::chrono::nanoseconds currentTime{};
    BmsGameReferee::PositionInfo position{};
    std::optional<Judgement> latestJudgement;
    std::optional<std::int64_t> latestDeviationNs;
    bool finished{};
    GameplayTrace trace;

    Impl(
      GameplayCoreConfig config,
      resource_managers::ChartDataFactory::ChartComponents components,
      PlayerChartBuildResult chart,
      std::unique_ptr<BmsLiveScore> score,
      std::unordered_map<std::uint64_t, std::shared_ptr<sounds::Sound>> sounds)
      : config(std::move(config))
      , chartData(std::move(components.chartData))
      , notesData(std::move(components.notesData))
      , chart(std::move(chart))
      , score(std::move(score))
    {
        trace.sha256 = chartData->getSha256();
        trace.md5 = chartData->getMd5();
        trace.randomSequence = chartData->getRandomSequence();
        trace.permutation = this->chart.storedPermutation();
        trace.laneSeed = this->config.play.laneSeed;
        trace.noteOrderP1 = this->config.play.noteOrderP1;
        trace.noteOrderP2 = this->config.play.noteOrderP2;
        trace.dpMode = this->chart.effectiveDpOptions;
        trace.gaugeSamples.push_back(
          { .chartTimeNs = 0, .value = gaugeValue() });

        const auto columns = this->chart.state->getColumnStates();
        for (qsizetype index = 0; index < columns.size(); ++index) {
            auto* column = columns[index];
            QObject::connect(this->score.get(),
                             &BmsLiveScore::hit,
                             column,
                             [column, index](const HitEvent& event) {
                                 if (index == event.getColumn()) {
                                     column->onHitEvent(event);
                                 }
                             });
        }
        QObject::connect(
          this->score.get(),
          &BmsLiveScore::hit,
          this->score.get(),
          [this](const HitEvent& event) { recordJudgement(event); });

        auto mineHitSound = std::shared_ptr<sounds::Sound>{};
        if (const auto mine = sounds.find(0); mine != sounds.end()) {
            mineHitSound = mine->second;
        }
        referee = std::make_unique<BmsGameReferee>(
          this->chart.rawNotes,
          notesData.bgmNotes,
          notesData.bpmChanges,
          std::move(mineHitSound),
          this->score.get(),
          std::move(sounds),
          rules::HitRules(rules::lr2_timing_windows::judgeNormal(),
                          rules::lr2_hit_values::getLr2HitValue));
        updatePosition();
    }

    [[nodiscard]] auto gaugeValue() const -> double
    {
        const auto gauges = score->getGauges();
        return gauges.isEmpty() ? 0.0 : gauges.front()->getGauge();
    }

    void ensureMonotonic(const std::chrono::nanoseconds time)
    {
        if (lastCommandTime && time < *lastCommandTime) {
            throw std::invalid_argument(
              "Gameplay timestamps must be monotonically nondecreasing");
        }
        lastCommandTime = time;
    }

    void updatePosition()
    {
        const auto bpm =
          referee ? referee->getBpm(currentTime) : notesData.bpmChanges.front();
        position = BmsGameReferee::getPosition(bpm, currentTime);
    }

    void advance(const std::chrono::nanoseconds time)
    {
        ensureMonotonic(time);
        currentTime = time;
        const auto shouldFinish =
          time >= std::chrono::nanoseconds{ chartData->getLength() } + 5s;
        referee->update(time, shouldFinish && !finished);
        finished = finished || shouldFinish;
        updatePosition();
    }

    void recordJudgement(const HitEvent& event)
    {
        const auto points = event.getPointsOptional();
        if (points) {
            latestJudgement = points->getJudgement();
            latestDeviationNs = points->getDeviation();
        }
        trace.judgements.push_back(
          { .chartTimeNs = event.getOffsetFromStart(),
            .hitOffsetNs = event.getHitOffset(),
            .column = event.getColumn(),
            .key = event.getKey(),
            .noteIndex = event.getNoteIndex(),
            .action = event.getAction(),
            .noteRemoved = event.getNoteRemoved(),
            .judgement = points.transform(
              [](const auto& value) { return value.getJudgement(); }),
            .deviationNs = points.transform(
              [](const auto& value) { return value.getDeviation(); }),
            .value = points.transform(
              [](const auto& value) { return value.getValue(); }) });
        if (points) {
            trace.gaugeSamples.push_back(
              { .chartTimeNs = event.getOffsetFromStart(),
                .value = gaugeValue() });
        }
    }

    [[nodiscard]] auto snapshotVisibleNoteCapacity() const noexcept
      -> std::size_t
    {
        auto capacity = std::size_t{};
        for (const auto& column : chart.rawNotes) {
            capacity += column.size();
        }
        return capacity;
    }

    void fillSnapshot(GameplaySnapshot& snapshot) const
    {
        snapshot.chartTimeNs = currentTime.count();
        snapshot.beatPosition = position.beatPosition;
        snapshot.scrollPosition = position.position;
        snapshot.points = score->getPoints();
        snapshot.maxPointsNow = score->getMaxPointsNow();
        snapshot.gauge = gaugeValue();
        snapshot.combo = score->getCombo();
        snapshot.maxCombo = score->getMaxCombo();
        snapshot.mineHits = score->getMineHits();
        snapshot.latestJudgement = latestJudgement;
        snapshot.latestDeviationNs = latestDeviationNs;
        snapshot.pressedColumns.fill(false);
        snapshot.visibleNotes.clear();
        snapshot.finished = finished;

        const auto renderBottom = position.position - renderBehind;
        const auto renderTop = position.position + renderAhead;
        const auto columnStates = chart.state->getColumnStates();
        for (auto columnIndex = qsizetype{}; columnIndex < columnStates.size();
             ++columnIndex) {
            const auto* column = columnStates[columnIndex];
            snapshot.pressedColumns[static_cast<std::size_t>(columnIndex)] =
              column->isPressed();
            for (const auto& noteState : column->getNotes()) {
                const auto rawIndex = static_cast<std::size_t>(noteState.index);
                const auto& rawColumn =
                  chart.rawNotes[static_cast<std::size_t>(columnIndex)];
                const auto& rawNote = rawColumn.at(rawIndex);
                const auto type = rawNote.noteType;
                const auto pairedScrollPosition =
                  pairedLongNoteScrollPosition(rawColumn, rawIndex);
                if (!intersectsRenderWindow(rawNote.time.position,
                                            pairedScrollPosition,
                                            renderBottom,
                                            renderTop)) {
                    continue;
                }
                auto removed = false;
                if (!noteState.hitData.isNull()) {
                    removed =
                      noteState.hitData.value<HitEvent>().getNoteRemoved();
                }
                snapshot.visibleNotes.push_back(
                  { .stableId = stableId(static_cast<std::size_t>(columnIndex),
                                         noteState.index),
                    .column = static_cast<std::uint8_t>(columnIndex),
                    .type = type,
                    .chartTimeNs = rawNote.time.timestamp.count(),
                    .beatPosition = rawNote.time.beatPosition,
                    .scrollPosition = rawNote.time.position,
                    .pairedScrollPosition = pairedScrollPosition,
                    .removed = removed,
                    .holding = isPairHolding(
                      noteState, type, column->isHoldingLongNote()) });
            }
        }
        std::ranges::sort(snapshot.visibleNotes,
                          [](const auto& left, const auto& right) {
                              if (left.chartTimeNs != right.chartTimeNs) {
                                  return left.chartTimeNs < right.chartTimeNs;
                              }
                              return left.stableId < right.stableId;
                          });
    }

    [[nodiscard]] auto makeTrace() const -> QByteArray
    {
        if (trace.judgements.size() !=
            static_cast<std::size_t>(score->getHitEvents().size())) {
            throw std::logic_error(
              "Gameplay trace did not capture every production score event");
        }
        auto completed = trace;
        const auto result = score->getResult();
        completed.result = {
            .points = score->getPoints(),
            .maxPoints = score->getMaxPoints(),
            .maxPointsNow = score->getMaxPointsNow(),
            .gauge = gaugeValue(),
            .combo = score->getCombo(),
            .maxCombo = score->getMaxCombo(),
            .mineHits = result->getMineHits(),
            .clearType = result->getClearType(),
            .judgementCounts = result->getJudgementCounts(),
            .savedTimestampSeconds = result->getUnixTimestamp(),
            .scoreGuid = result->getGuid(),
            .chartLengthNs = result->getLength(),
            .keymode = static_cast<int>(result->getKeymode()),
            .dpMode = static_cast<int>(result->getDpOptions()),
        };
        return completed.toCanonicalJson();
    }
};

SinglePlayerGameplayCore::SinglePlayerGameplayCore(std::unique_ptr<Impl> impl)
  : impl(std::move(impl))
{
}

SinglePlayerGameplayCore::~SinglePlayerGameplayCore() = default;
SinglePlayerGameplayCore::SinglePlayerGameplayCore(
  SinglePlayerGameplayCore&&) noexcept = default;
auto
SinglePlayerGameplayCore::operator=(SinglePlayerGameplayCore&&) noexcept
  -> SinglePlayerGameplayCore& = default;

auto
SinglePlayerGameplayCore::create(
  const std::string_view chartBytes,
  const std::filesystem::path& logicalChartPath,
  GameplayCoreConfig config,
  std::unordered_map<std::uint64_t, std::shared_ptr<sounds::Sound>> sounds)
  -> std::unique_ptr<SinglePlayerGameplayCore>
{
    if (!config.play.isSupported()) {
        throw std::invalid_argument("Unsupported chart randomization version");
    }
    if (config.savedTimestampSeconds == 0 || config.scoreGuid.isEmpty()) {
        throw std::invalid_argument(
          "Deterministic gameplay requires a fixed timestamp and GUID");
    }
    const auto factory = resource_managers::ChartDataFactory{};
    auto components = factory.loadChartDataWithRandomSequence(
      chartBytes, logicalChartPath, config.play.randomSequence);
    if (!components) {
        throw std::invalid_argument(
          "The supplied #RANDOM sequence is incomplete or invalid");
    }
    auto chart = buildPlayerChart(components->notesData,
                                  *components->chartData,
                                  asPlayerBuildOptions(config.play));
    auto gauges = makeNormalGauge(*components->chartData, chart);
    auto score =
      makeScore(*components->chartData, chart, config, std::move(gauges));
    return std::unique_ptr<SinglePlayerGameplayCore>{
        new SinglePlayerGameplayCore{
          std::make_unique<Impl>(std::move(config),
                                 std::move(*components),
                                 std::move(chart),
                                 std::move(score),
                                 std::move(sounds)) }
    };
}

void
SinglePlayerGameplayCore::advanceTo(const std::chrono::nanoseconds chartTime)
{
    impl->advance(chartTime);
}

void
SinglePlayerGameplayCore::passKey(const input::BmsKey key,
                                  const GameplayKeyAction action,
                                  const std::chrono::nanoseconds chartTime)
{
    impl->advance(chartTime);
    impl->trace.inputs.push_back({ .chartTimeNs = chartTime.count(),
                                   .key = static_cast<int>(key),
                                   .action = action == GameplayKeyAction::Press
                                               ? HitEvent::Action::Press
                                               : HitEvent::Action::Release });
    if (action == GameplayKeyAction::Press) {
        impl->referee->passPressed(chartTime, key);
    } else {
        impl->referee->passReleased(chartTime, key);
    }
}

void
SinglePlayerGameplayCore::preScheduleBgm()
{
    impl->referee->preScheduleBgm();
}

auto
SinglePlayerGameplayCore::snapshotVisibleNoteCapacity() const noexcept
  -> std::size_t
{
    return impl->snapshotVisibleNoteCapacity();
}

void
SinglePlayerGameplayCore::reserveSnapshot(GameplaySnapshot& snapshot) const
{
    snapshot.visibleNotes.reserve(snapshotVisibleNoteCapacity());
}

void
SinglePlayerGameplayCore::fillSnapshot(GameplaySnapshot& snapshot) const
{
    impl->fillSnapshot(snapshot);
}

auto
SinglePlayerGameplayCore::snapshot() const -> GameplaySnapshot
{
    auto result = GameplaySnapshot{};
    fillSnapshot(result);
    return result;
}

auto
SinglePlayerGameplayCore::finishTrace() const -> QByteArray
{
    return impl->makeTrace();
}

} // namespace gameplay_logic

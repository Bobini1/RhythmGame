#include "DesktopPlayerChartCoordinator.h"

#include <utility>

namespace resource_managers {
namespace {

auto
selectIdentity(const DesktopPlayerBuildOptions& options) -> DesktopScoreIdentity
{
    if (options.replayIdentity) {
        return *options.replayIdentity;
    }
    if (options.autoPlay) {
        return {};
    }
    return {
        .savedTimestamp = 0,
        .guid = options.generatedGuid,
        .submissionState =
          gameplay_logic::BmsScore::SubmissionState::NotSubmitted,
    };
}

auto
buildDesktopPlayerChart(const charts::BmsNotesData& notesData,
                        const gameplay_logic::ChartData& chartData,
                        DesktopPlayerBuildOptions options,
                        const double maxHitValue) -> DesktopPlayerChart
{
    auto chart =
      gameplay_logic::buildPlayerChart(notesData, chartData, options.chart);
    const auto countMultiplier =
      chart.effectiveDpOptions == DpOptions::Battle ? 2 : 1;
    auto identity = selectIdentity(options);
    auto scoreMetadata = DesktopScoreMetadata{
        .countMultiplier = countMultiplier,
        .noteOrderP1 = options.chart.noteOrderP1,
        .noteOrderP2 = gameplay_logic::isDp(chart.effectiveKeymode)
                         ? options.chart.noteOrderP2
                         : NoteOrderAlgorithm::Normal,
        .dpOptions = chart.effectiveDpOptions,
        .permutation = chart.storedPermutation(),
        .randomSeed = chart.storedSeed(),
        .keymode = chart.effectiveKeymode,
        .identity = identity,
    };
    auto score = std::make_unique<gameplay_logic::BmsLiveScore>(
      chartData.getNormalNoteCount() * countMultiplier,
      chartData.getScratchCount() * countMultiplier,
      chartData.getLnCount() * countMultiplier,
      chartData.getBssCount() * countMultiplier,
      chartData.getMineCount() * countMultiplier,
      (chartData.getLnCount() + chartData.getNormalNoteCount() +
       chartData.getBssCount() + chartData.getScratchCount()) *
        countMultiplier,
      maxHitValue,
      std::move(options.gauges),
      chartData.getRandomSequence(),
      scoreMetadata.noteOrderP1,
      scoreMetadata.noteOrderP2,
      scoreMetadata.dpOptions,
      scoreMetadata.permutation,
      scoreMetadata.randomSeed,
      chartData.getLength(),
      chartData.getSha256(),
      chartData.getMd5(),
      scoreMetadata.keymode,
      identity.savedTimestamp,
      identity.guid,
      identity.submissionState);
    return {
        .chart = std::move(chart),
        .score = std::move(score),
        .scoreMetadata = std::move(scoreMetadata),
    };
}

} // namespace

auto
buildDesktopPlayerChartPair(const charts::BmsNotesData& notesData,
                            const gameplay_logic::ChartData& chartData,
                            DesktopPlayerBuildOptions player1,
                            std::optional<DesktopPlayerBuildOptions> player2,
                            const double maxHitValue) -> DesktopPlayerChartPair
{
    auto builtPlayer1 = buildDesktopPlayerChart(
      notesData, chartData, std::move(player1), maxHitValue);
    auto builtPlayer2 = player2.transform([&](auto options) {
        options.chart.dpOptions = DpOptions::Off;
        return buildDesktopPlayerChart(
          notesData, chartData, std::move(options), maxHitValue);
    });
    return {
        .player1 = std::move(builtPlayer1),
        .player2 = std::move(builtPlayer2),
    };
}

} // namespace resource_managers

#ifndef RHYTHMGAME_DESKTOPPLAYERCHARTCOORDINATOR_H
#define RHYTHMGAME_DESKTOPPLAYERCHARTCOORDINATOR_H

#include "gameplay_logic/BmsLiveScore.h"
#include "gameplay_logic/SinglePlayerChartBuilder.h"

#include <cstdint>
#include <memory>
#include <optional>

namespace resource_managers {

struct DesktopScoreIdentity
{
    std::int64_t savedTimestamp{};
    QString guid;
    gameplay_logic::BmsScore::SubmissionState submissionState{
        gameplay_logic::BmsScore::SubmissionState::NotSubmitted
    };

    bool operator==(const DesktopScoreIdentity&) const = default;
};

struct DesktopPlayerBuildOptions
{
    gameplay_logic::PlayerChartBuildOptions chart;
    QList<gameplay_logic::rules::BmsGauge*> gauges;
    bool autoPlay{};
    std::optional<DesktopScoreIdentity> replayIdentity;
    QString generatedGuid;
};

struct DesktopScoreMetadata
{
    int countMultiplier;
    NoteOrderAlgorithm noteOrderP1;
    NoteOrderAlgorithm noteOrderP2;
    DpOptions dpOptions;
    QList<int> permutation;
    std::uint64_t randomSeed;
    gameplay_logic::ChartData::Keymode keymode;
    DesktopScoreIdentity identity;
};

struct DesktopPlayerChart
{
    gameplay_logic::PlayerChartBuildResult chart;
    std::unique_ptr<gameplay_logic::BmsLiveScore> score;
    DesktopScoreMetadata scoreMetadata;
};

struct DesktopPlayerChartPair
{
    DesktopPlayerChart player1;
    std::optional<DesktopPlayerChart> player2;
};

auto
buildDesktopPlayerChartPair(const charts::BmsNotesData& notesData,
                            const gameplay_logic::ChartData& chartData,
                            DesktopPlayerBuildOptions player1,
                            std::optional<DesktopPlayerBuildOptions> player2,
                            double maxHitValue) -> DesktopPlayerChartPair;

} // namespace resource_managers

#endif // RHYTHMGAME_DESKTOPPLAYERCHARTCOORDINATOR_H

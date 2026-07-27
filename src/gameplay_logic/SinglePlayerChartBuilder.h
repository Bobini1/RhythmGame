#ifndef RHYTHMGAME_SINGLEPLAYERCHARTBUILDER_H
#define RHYTHMGAME_SINGLEPLAYERCHARTBUILDER_H

#include "BmsNotes.h"
#include "ChartData.h"
#include "NoteState.h"
#include "charts/BmsNotesData.h"
#include "resource_managers/ChartPlayOptions.h"
#include "support/GeneratePermutation.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace gameplay_logic {

struct PlayerChartBuildOptions
{
    resource_managers::NoteOrderAlgorithm noteOrderP1{
        resource_managers::NoteOrderAlgorithm::Normal
    };
    resource_managers::NoteOrderAlgorithm noteOrderP2{
        resource_managers::NoteOrderAlgorithm::Normal
    };
    resource_managers::DpOptions dpOptions{ resource_managers::DpOptions::Off };
    std::uint64_t randomSeed{};
    bool usePre130{};
};

struct PlayerChartBuildResult
{
    std::unique_ptr<BmsNotes> notes;
    std::unique_ptr<GameplayState> state;
    std::array<support::ShuffleResult, 2> shuffleResults;
    std::array<std::vector<charts::BmsNotesData::Note>,
               charts::BmsNotesData::columnNumber>
      rawNotes;
    resource_managers::DpOptions effectiveDpOptions;
    ChartData::Keymode effectiveKeymode;

    [[nodiscard]] auto storedPermutation() const -> QList<int>;
    [[nodiscard]] auto storedSeed() const -> std::uint64_t;
};

auto
buildPlayerChart(const charts::BmsNotesData& notesData,
                 const ChartData& chartData,
                 const PlayerChartBuildOptions& options)
  -> PlayerChartBuildResult;

} // namespace gameplay_logic

#endif // RHYTHMGAME_SINGLEPLAYERCHARTBUILDER_H

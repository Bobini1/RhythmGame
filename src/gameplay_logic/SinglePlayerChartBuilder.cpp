#include "SinglePlayerChartBuilder.h"

#include "resource_managers/ChartDataFactory.h"

#include <optional>
#include <ranges>
#include <span>

namespace gameplay_logic {
namespace {

auto
applyBeatorajaOrder(std::span<std::vector<charts::BmsNotesData::Note>>& notes,
                    const resource_managers::NoteOrderAlgorithm algorithm,
                    const std::uint64_t seed,
                    const bool k5) -> support::ShuffleResult
{
    auto originalSpan = notes;
    auto workingNotes = notes;
    if (k5) {
        notes[5].swap(notes[7]);
        workingNotes = notes.subspan(0, 6);
    }

    const auto result = support::generateBeatorajaLanePermutation(
      workingNotes, algorithm, static_cast<std::int64_t>(seed));

    if (k5) {
        notes[5].swap(notes[7]);
        notes = originalSpan;
    }
    return result;
}

auto
applyLr2Order(std::span<std::vector<charts::BmsNotesData::Note>>& notes,
              const resource_managers::NoteOrderAlgorithm algorithm,
              support::Lr2Random& randomGenerator,
              const bool k5) -> support::ShuffleResult
{
    auto originalSpan = notes;
    auto workingNotes = notes;
    if (k5) {
        notes[5].swap(notes[7]);
        workingNotes = notes.subspan(0, 6);
    }

    const auto result = support::generateLr2LanePermutation(
      workingNotes, algorithm, randomGenerator);

    if (k5) {
        notes[5].swap(notes[7]);
        notes = originalSpan;
    }
    return result;
}

auto
applyOrder(std::span<std::vector<charts::BmsNotesData::Note>>& notes,
           const resource_managers::NoteOrderAlgorithm algorithm,
           const std::uint64_t seed,
           const bool k5,
           const bool usePre130,
           support::Lr2Random* lr2RandomGenerator) -> support::ShuffleResult
{
    if (support::isBeatorajaNoteOrderAlgorithm(algorithm)) {
        return applyBeatorajaOrder(notes, algorithm, seed, k5);
    }
    if (support::isLr2NoteOrderAlgorithm(algorithm)) {
        if (lr2RandomGenerator == nullptr) {
            auto randomGenerator =
              support::Lr2Random{ static_cast<std::uint32_t>(seed) };
            return applyLr2Order(notes, algorithm, randomGenerator, k5);
        }
        return applyLr2Order(notes, algorithm, *lr2RandomGenerator, k5);
    }
    return support::generatePermutation(notes, algorithm, seed, k5, usePre130);
}

auto
makeGameplayState(const BmsNotes& notes) -> std::unique_ptr<GameplayState>
{
    auto noteStates = QList<ColumnState*>{};
    for (const auto& column : notes.getNotes()) {
        auto columnNotes = QList<NoteState>{};
        columnNotes.reserve(column.size());
        for (const auto& [index, note] :
             std::ranges::views::enumerate(column)) {
            columnNotes.append({ note, index });
        }
        noteStates.append(new ColumnState(std::move(columnNotes)));
    }

    auto barLineStates = QList<BarLineState>{};
    barLineStates.reserve(notes.getBarLines().size());
    for (const auto& [index, barLine] :
         std::ranges::views::enumerate(notes.getBarLines())) {
        barLineStates.append({ barLine, index });
    }
    auto* barLinesState = new BarLinesState(std::move(barLineStates));
    return std::make_unique<GameplayState>(std::move(noteStates),
                                           barLinesState);
}

} // namespace

auto
PlayerChartBuildResult::storedPermutation() const -> QList<int>
{
    return shuffleResults[0].columns + shuffleResults[1].columns;
}

auto
PlayerChartBuildResult::storedSeed() const -> std::uint64_t
{
    return shuffleResults[0].seed;
}

auto
buildPlayerChart(const charts::BmsNotesData& notesData,
                 const ChartData& chartData,
                 const PlayerChartBuildOptions& options)
  -> PlayerChartBuildResult
{
    using resource_managers::DpOptions;

    auto visibleNotes = notesData.notes;
    auto dpOptions = options.dpOptions;
    const auto isDpFlip =
      dpOptions == DpOptions::Flip || dpOptions == DpOptions::Lr2Flip;
    if ((dpOptions == DpOptions::Battle && isDp(chartData.getKeymode())) ||
        (isDpFlip && !isDp(chartData.getKeymode()))) {
        dpOptions = DpOptions::Off;
    }

    auto keymode = chartData.getKeymode();
    if (dpOptions == DpOptions::Battle) {
        switch (keymode) {
            case ChartData::Keymode::K5:
                keymode = ChartData::Keymode::K10;
                break;
            case ChartData::Keymode::K7:
                keymode = ChartData::Keymode::K14;
                break;
            case ChartData::Keymode::K10:
            case ChartData::Keymode::K14:
                break;
        }
    }

    // We used to treat 5K as 7K. Reproduce that when generating replays.
    const auto randomIs5k =
      !options.usePre130 &&
      (keymode == ChartData::Keymode::K5 || keymode == ChartData::Keymode::K10);

    if (dpOptions == DpOptions::Flip) {
        support::flipBeatorajaDpPlayfields(visibleNotes);
    }
    if (dpOptions == DpOptions::Lr2Flip) {
        support::flipLr2DpPlayfields(visibleNotes);
    }
    if (dpOptions == DpOptions::Battle) {
        for (auto column = 0; column < 7; ++column) {
            visibleNotes[14 - column] = visibleNotes[column];
        }
        visibleNotes[15] = visibleNotes[7];
    }

    auto shuffleResults = [&]() -> std::array<support::ShuffleResult, 2> {
        auto lr2RandomGenerator =
          support::isLr2NoteOrderAlgorithm(options.noteOrderP1) ||
              support::isLr2NoteOrderAlgorithm(options.noteOrderP2)
            ? std::optional<support::Lr2Random>{ static_cast<std::uint32_t>(
                options.randomSeed) }
            : std::nullopt;
        if (lr2RandomGenerator) {
            lr2RandomGenerator->discard(
              static_cast<std::size_t>(chartData.getRandomSequence().size()));
        }
        auto* lr2RandomGeneratorPtr =
          lr2RandomGenerator ? &*lr2RandomGenerator : nullptr;

        if (isDp(keymode)) {
            auto notesP1 =
              std::span{ visibleNotes.data(), visibleNotes.size() / 2 };
            auto resultP1 = applyOrder(notesP1,
                                       options.noteOrderP1,
                                       options.randomSeed,
                                       randomIs5k,
                                       options.usePre130,
                                       lr2RandomGeneratorPtr);
            auto notesP2 =
              std::span{ visibleNotes.data() + visibleNotes.size() / 2,
                         visibleNotes.size() / 2 };
            auto resultP2 = applyOrder(notesP2,
                                       options.noteOrderP2,
                                       resultP1.seed + 1,
                                       randomIs5k,
                                       options.usePre130,
                                       lr2RandomGeneratorPtr);
            return { std::move(resultP1), std::move(resultP2) };
        }

        auto notesP1 =
          std::span{ visibleNotes.data(), visibleNotes.size() / 2 };
        return { applyOrder(notesP1,
                            options.noteOrderP1,
                            options.randomSeed,
                            randomIs5k,
                            options.usePre130,
                            lr2RandomGeneratorPtr),
                 support::ShuffleResult{} };
    }();

    auto notes = resource_managers::ChartDataFactory::makeNotes(
      visibleNotes, notesData.barLines);
    auto state = makeGameplayState(*notes);
    return {
        std::move(notes),        std::move(state), std::move(shuffleResults),
        std::move(visibleNotes), dpOptions,        keymode,
    };
}

} // namespace gameplay_logic

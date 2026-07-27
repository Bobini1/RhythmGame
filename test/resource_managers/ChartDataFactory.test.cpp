#include "resource_managers/ChartDataFactory.h"
#include "resource_managers/ChartPlayConfig.h"
#include "support/QStringToPath.h"

#include <QFile>
#include <QTemporaryDir>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string_view>

namespace {
using ChartComponents = resource_managers::ChartDataFactory::ChartComponents;
using Note = charts::BmsNotesData::Note;

constexpr auto fixture = std::string_view{
    "#TITLE Portable chart\n"
    "#ARTIST RhythmGame\n"
    "#SUBARTIST Web playtest\n"
    "#GENRE Test\n"
    "#BPM 120\n"
    "#BPM01 180\n"
    "#SCROLL01 0.5\n"
    "#PLAYLEVEL 7\n"
    "#DIFFICULTY 3\n"
    "#LNTYPE 1\n"
    "#WAV01 sounds/kick.wav\n"
    "#WAV02 sounds/long.wav\n"
    "#BMP01 images/stage.png\n"
    "#RANDOM 2\n"
    "#IF 2\n"
    "#00111:0100\n"
    "#ELSE\n"
    "#00112:0100\n"
    "#ENDIF\n"
    "#RANDOM 3\n"
    "#IF 1\n"
    "#00113:0001\n"
    "#ENDIF\n"
    "#00101:0100\n"
    "#00108:0001\n"
    "#001SC:0100\n"
    "#00151:0202\n"
    "#001D4:00010000\n"
};

auto fixedRandomGenerator(QList<qint64> values)
  -> resource_managers::ChartDataFactory::RandomGenerator
{
    return [values = std::move(values), cursor = qsizetype{}](
             charts::ParsedBmsChart::RandomRange) mutable {
        return static_cast<charts::ParsedBmsChart::RandomRange>(
          values.at(cursor++));
    };
}

void requireSameNote(const Note& lhs, const Note& rhs)
{
    REQUIRE(lhs.time == rhs.time);
    REQUIRE(lhs.snap.numerator == rhs.snap.numerator);
    REQUIRE(lhs.snap.denominator == rhs.snap.denominator);
    REQUIRE(lhs.noteType == rhs.noteType);
    REQUIRE(lhs.sound == rhs.sound);
}

void requireSameComponents(const ChartComponents& lhs,
                           const ChartComponents& rhs)
{
    REQUIRE(lhs.chartData->getTitle() == rhs.chartData->getTitle());
    REQUIRE(lhs.chartData->getArtist() == rhs.chartData->getArtist());
    REQUIRE(lhs.chartData->getSubtitle() == rhs.chartData->getSubtitle());
    REQUIRE(lhs.chartData->getSubartist() == rhs.chartData->getSubartist());
    REQUIRE(lhs.chartData->getGenre() == rhs.chartData->getGenre());
    REQUIRE(lhs.chartData->getPlayLevel() == rhs.chartData->getPlayLevel());
    REQUIRE(lhs.chartData->getDifficulty() == rhs.chartData->getDifficulty());
    REQUIRE(lhs.chartData->getInitialBpm() == rhs.chartData->getInitialBpm());
    REQUIRE(lhs.chartData->getMinBpm() == rhs.chartData->getMinBpm());
    REQUIRE(lhs.chartData->getMaxBpm() == rhs.chartData->getMaxBpm());
    REQUIRE(lhs.chartData->getRandomSequence() ==
            rhs.chartData->getRandomSequence());
    REQUIRE(lhs.chartData->getSha256() == rhs.chartData->getSha256());
    REQUIRE(lhs.chartData->getMd5() == rhs.chartData->getMd5());
    REQUIRE(lhs.chartData->getHistogramData() ==
            rhs.chartData->getHistogramData());
    REQUIRE(lhs.chartData->getBpmChanges() ==
            rhs.chartData->getBpmChanges());

    REQUIRE(lhs.wavs == rhs.wavs);
    REQUIRE(lhs.bmps == rhs.bmps);
    REQUIRE(lhs.notesData.bgmNotes == rhs.notesData.bgmNotes);
    REQUIRE(lhs.notesData.bpmChanges.size() ==
            rhs.notesData.bpmChanges.size());
    for (auto index = std::size_t{}; index < lhs.notesData.bpmChanges.size();
         ++index) {
        const auto& lhsChange = lhs.notesData.bpmChanges[index];
        const auto& rhsChange = rhs.notesData.bpmChanges[index];
        REQUIRE(lhsChange.bpm == rhsChange.bpm);
        REQUIRE(lhsChange.scroll == rhsChange.scroll);
        REQUIRE(lhsChange.timestamp == rhsChange.timestamp);
    }
    REQUIRE(lhs.notesData.notes.size() == rhs.notesData.notes.size());
    for (auto column = std::size_t{}; column < lhs.notesData.notes.size();
         ++column) {
        REQUIRE(lhs.notesData.notes[column].size() ==
                rhs.notesData.notes[column].size());
        for (auto note = std::size_t{};
             note < lhs.notesData.notes[column].size();
             ++note) {
            requireSameNote(lhs.notesData.notes[column][note],
                            rhs.notesData.notes[column][note]);
        }
    }
}
} // namespace

TEST_CASE("ChartDataFactory: chart byte loading is identical to path loading",
          "[ChartDataFactory]")
{
    auto tempDirectory = QTemporaryDir{};
    REQUIRE(tempDirectory.isValid());
    const auto chartPath =
      support::qStringToPath(tempDirectory.filePath("portable.bms"));
    auto file = QFile{ tempDirectory.filePath("portable.bms") };
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(fixture.data(), static_cast<qint64>(fixture.size())) ==
            fixture.size());
    file.close();

    const auto factory = resource_managers::ChartDataFactory{};
    const auto fromPath =
      factory.loadChartData(chartPath, fixedRandomGenerator({ 2, 1 }));
    const auto fromBytes = factory.loadChartData(
      fixture, chartPath, fixedRandomGenerator({ 2, 1 }));
    const auto deterministic =
      factory.loadChartDataWithRandomSequence(
        chartPath, QList<qint64>{ 2, 1 });

    requireSameComponents(fromPath, fromBytes);
    REQUIRE(deterministic.has_value());
    requireSameComponents(fromPath, *deterministic);
    REQUIRE(fromBytes.chartData->getRandomSequence() == QList<qint64>{ 2, 1 });
    REQUIRE(fromBytes.wavs.at(1) ==
            std::filesystem::path{ "sounds/kick.wav" });
    REQUIRE(fromBytes.bmps.at(1) ==
            std::filesystem::path{ "images/stage.png" });
    REQUIRE_FALSE(fromBytes.notesData.bgmNotes.empty());

    auto hasNormal = false;
    auto hasLongNoteBegin = false;
    auto hasLongNoteEnd = false;
    auto hasLandmine = false;
    for (const auto& column : fromBytes.notesData.notes) {
        for (const auto& note : column) {
            hasNormal |=
              note.noteType == charts::BmsNotesData::NoteType::Normal;
            hasLongNoteBegin |=
              note.noteType == charts::BmsNotesData::NoteType::LongNoteBegin;
            hasLongNoteEnd |=
              note.noteType == charts::BmsNotesData::NoteType::LongNoteEnd;
            hasLandmine |=
              note.noteType == charts::BmsNotesData::NoteType::Landmine;
        }
    }
    REQUIRE(hasNormal);
    REQUIRE(hasLongNoteBegin);
    REQUIRE(hasLongNoteEnd);
    REQUIRE(hasLandmine);

    auto hasBpmChange = false;
    auto hasScrollChange = false;
    for (const auto& change : fromBytes.notesData.bpmChanges) {
        hasBpmChange |= change.bpm == 180.0;
        hasScrollChange |= change.scroll == 0.5;
    }
    REQUIRE(hasBpmChange);
    REQUIRE(hasScrollChange);
}

TEST_CASE("ChartDataFactory: deterministic random loading rejects invalid input",
          "[ChartDataFactory]")
{
    const auto chartPath = std::filesystem::path{ "memory/portable.bms" };
    const auto factory = resource_managers::ChartDataFactory{};

    SECTION("valid")
    {
        const auto components = factory.loadChartDataWithRandomSequence(
          fixture, chartPath, QList<qint64>{ 2, 1 });
        REQUIRE(components.has_value());
        REQUIRE(components->chartData->getRandomSequence() ==
                QList<qint64>{ 2, 1 });
    }

    SECTION("incomplete")
    {
        const auto components = factory.loadChartDataWithRandomSequence(
          fixture, chartPath, QList<qint64>{ 2 });
        REQUIRE_FALSE(components.has_value());
    }

    SECTION("out of range")
    {
        const auto components = factory.loadChartDataWithRandomSequence(
          fixture, chartPath, QList<qint64>{ 3, 1 });
        REQUIRE_FALSE(components.has_value());
    }

    SECTION("bmson rejects a BMS random sequence")
    {
        const auto components = factory.loadChartDataWithRandomSequence(
          std::filesystem::path{ "memory/portable.bmson" },
          QList<qint64>{ 1 });
        REQUIRE_FALSE(components.has_value());
    }
}

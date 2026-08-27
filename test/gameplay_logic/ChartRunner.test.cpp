#include "gameplay_logic/BmsGameReferee.h"
#include "gameplay_logic/BmsLiveScore.h"
#include "gameplay_logic/BmsNotes.h"
#include "gameplay_logic/ChartData.h"
#include "gameplay_logic/ChartRunner.h"
#include "gameplay_logic/NoteState.h"
#include "gameplay_logic/rules/HitRules.h"
#include "qml_components/Bga.h"
#include "resource_managers/ChartPlayConfig.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QPromise>
#include <QThread>

#include <array>
#include <memory>
#include <unordered_map>

namespace {

void
ensureCoreApplication()
{
    if (QCoreApplication::instance() != nullptr) {
        return;
    }
    static int argc = 1;
    static char applicationName[] = "RhythmGameChartRunnerTests";
    static char* argv[]{ applicationName, nullptr };
    static const auto application =
      std::make_unique<QCoreApplication>(argc, argv);
}

template<typename T>
auto
readyFuture(T value) -> QFuture<T>
{
    QPromise<T> promise;
    promise.start();
    auto future = promise.future();
    promise.addResult(std::move(value));
    promise.finish();
    return future;
}

auto
makeChartData() -> std::unique_ptr<gameplay_logic::ChartData>
{
    return std::make_unique<gameplay_logic::ChartData>(
      QStringLiteral("Quick retry"),
      QStringLiteral("Composer"),
      QString{},
      QString{},
      QString{},
      QString{},
      QString{},
      QString{},
      75.0,
      100.0,
      1,
      1,
      false,
      QList<qint64>{ 2, 1 },
      0,
      0,
      0,
      0,
      0,
      1'000,
      120.0,
      120.0,
      120.0,
      120.0,
      120.0,
      0.0,
      0.0,
      0.0,
      QStringLiteral("quick-retry.bms"),
      0,
      QString(64, QChar(u'a')),
      QString(32, QChar(u'b')),
      gameplay_logic::ChartData::Keymode::K7,
      QList<QList<qint64>>{},
      QList<gameplay_logic::BpmChange>{},
      0);
}

struct RunnerFixture
{
    std::unique_ptr<gameplay_logic::ChartRunner> runner;
    gameplay_logic::BmsLiveScore* score;
};

auto
makeReadyRunner() -> RunnerFixture
{
    using namespace std::chrono_literals;
    ensureCoreApplication();
    auto* score = new gameplay_logic::BmsLiveScore{
        0,
        0,
        0,
        0,
        0,
        0,
        0.0,
        {},
        { 2, 1 },
        resource_managers::NoteOrderAlgorithm::Random,
        resource_managers::NoteOrderAlgorithm::Mirror,
        resource_managers::DpOptions::Flip,
        {},
        1234,
        1'000,
        QString(64, QChar(u'a')),
        QString(32, QChar(u'b')),
        gameplay_logic::ChartData::Keymode::K7,
        0,
    };
    std::array<std::vector<charts::BmsNotesData::Note>,
               charts::BmsNotesData::columnNumber>
      rawNotes{};
    auto referee = gameplay_logic::BmsGameReferee{
        std::move(rawNotes),
        {},
        { charts::BmsNotesData::BpmChangeValues{
          .bpm = 120.0,
          .scroll = 1.0,
          .timestamp = {},
        } },
        {},
        score,
        std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>{},
        gameplay_logic::rules::HitRules{
          {},
          [](std::chrono::nanoseconds, gameplay_logic::Judgement) {
              return 0.0;
          } },
    };
    auto* player = new gameplay_logic::Player{
        new gameplay_logic::BmsNotes{},
        score,
        new gameplay_logic::GameplayState{
          {}, new gameplay_logic::BarLinesState{ {} } },
        nullptr,
        readyFuture(std::move(referee)),
        1s,
        120.0,
    };
    auto runner = std::make_unique<gameplay_logic::ChartRunner>(
      makeChartData().release(),
      readyFuture(std::make_unique<qml_components::BgaContainer>(
        QList<qml_components::Bga*>{},
        std::vector<QMediaPlayer*>{},
        std::vector<std::unique_ptr<QVideoFrame>>{})),
      gameplay_logic::ChartData::Keymode::K7,
      player,
      nullptr);
    QElapsedTimer timeout;
    timeout.start();
    while (runner->getStatus() != gameplay_logic::ChartRunner::Ready &&
           timeout.elapsed() < 2'000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(1);
    }
    REQUIRE(runner->getStatus() == gameplay_logic::ChartRunner::Ready);
    return { std::move(runner), score };
}

void
addJudgement(gameplay_logic::BmsLiveScore* score,
             gameplay_logic::Judgement judgement)
{
    score->addHit(
      gameplay_logic::HitEvent{ 0,
                                std::optional<int>{ 0 },
                                0,
                                gameplay_logic::BmsPoints{ 0.0, judgement, 0 },
                                gameplay_logic::HitEvent::Action::Press,
                                true });
}

}

TEST_CASE("quick retry remains available through non-playing judgements")
{
    auto fixture = makeReadyRunner();

    CHECK(fixture.runner->canQuickRetry());
    addJudgement(fixture.score, gameplay_logic::Judgement::Poor);
    addJudgement(fixture.score, gameplay_logic::Judgement::EmptyPoor);
    addJudgement(fixture.score, gameplay_logic::Judgement::MineHit);
    addJudgement(fixture.score, gameplay_logic::Judgement::MineAvoided);
    CHECK(fixture.runner->canQuickRetry());
}

TEST_CASE("quick retry remains available after meaningful play")
{
    const auto judgement = GENERATE(gameplay_logic::Judgement::Bad,
                                    gameplay_logic::Judgement::Good,
                                    gameplay_logic::Judgement::Great,
                                    gameplay_logic::Judgement::Perfect,
                                    gameplay_logic::Judgement::LnEndSkip,
                                    gameplay_logic::Judgement::LnBeginHit);
    CAPTURE(judgement);

    auto fixture = makeReadyRunner();
    fixture.runner->start();
    REQUIRE(fixture.runner->getStatus() ==
            gameplay_logic::ChartRunner::Running);
    addJudgement(fixture.score, judgement);

    CHECK(fixture.runner->canQuickRetry());
}

TEST_CASE("quick retry snapshots the complete chart transformation")
{
    auto fixture = makeReadyRunner();

    const auto config = fixture.runner->getPlayConfig();

    CHECK(config.randomSequence == QList<qint64>{ 2, 1 });
    CHECK(config.noteOrderP1 == resource_managers::NoteOrderAlgorithm::Random);
    CHECK(config.noteOrderP2 == resource_managers::NoteOrderAlgorithm::Mirror);
    CHECK(config.dpMode == resource_managers::DpOptions::Flip);
    CHECK(config.laneSeed == 1234);
}

TEST_CASE("aborting a chart does not finish or score the run")
{
    auto fixture = makeReadyRunner();
    fixture.runner->start();
    REQUIRE(fixture.runner->getStatus() ==
            gameplay_logic::ChartRunner::Running);

    fixture.runner->abort();

    CHECK(fixture.runner->getStatus() == gameplay_logic::ChartRunner::Running);
    CHECK(fixture.runner->inputSuppressed());
    CHECK(fixture.runner->canQuickRetry());
}

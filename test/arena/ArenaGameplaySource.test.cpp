#include "arena/QtArenaGameplaySource.h"
#include "gameplay_logic/BmsGameReferee.h"
#include "gameplay_logic/BmsGaugeHistory.h"
#include "gameplay_logic/BmsLiveScore.h"
#include "gameplay_logic/BmsNotes.h"
#include "gameplay_logic/BmsReplayData.h"
#include "gameplay_logic/BmsScore.h"
#include "gameplay_logic/ChartData.h"
#include "gameplay_logic/ChartRunner.h"
#include "gameplay_logic/NoteState.h"
#include "gameplay_logic/rules/BmsGauge.h"
#include "gameplay_logic/rules/HitRules.h"
#include "qml_components/Bga.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QPromise>
#include <QThread>

#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;

void
ensureCoreApplication()
{
    if (QCoreApplication::instance() != nullptr) {
        return;
    }
    static int argc = 1;
    static char applicationName[] = "ArenaGameplaySourceTests";
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

class FixedGauge final : public gameplay_logic::rules::BmsGauge
{
  public:
    FixedGauge(QString name, double maximum, double value, double threshold)
      : BmsGauge(std::move(name), maximum, value, threshold, false)
    {
    }

    void addHit(std::chrono::nanoseconds,
                std::chrono::nanoseconds,
                gameplay_logic::Judgement) override
    {
    }

    void addMineHit(std::chrono::nanoseconds, double) override {}
};

auto
makeChart() -> std::unique_ptr<gameplay_logic::ChartData>
{
    return std::make_unique<gameplay_logic::ChartData>(
      QStringLiteral("Arena source"),
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
      QList<qint64>{},
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
      QStringLiteral("arena-source.bms"),
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
    gameplay_logic::Player* player{};
    gameplay_logic::BmsLiveScore* score{};
};

auto
makeRunner(QList<gameplay_logic::rules::BmsGauge*> gauges,
           std::chrono::nanoseconds length = 1s,
           QString guid = QStringLiteral("arena-score-guid")) -> RunnerFixture
{
    ensureCoreApplication();
    auto* score = new gameplay_logic::BmsLiveScore{
        0,
        0,
        0,
        0,
        0,
        100,
        2.0,
        std::move(gauges),
        {},
        resource_managers::NoteOrderAlgorithm::Normal,
        resource_managers::NoteOrderAlgorithm::Normal,
        resource_managers::DpOptions::Off,
        {},
        0,
        length.count(),
        QString(64, QChar(u'a')),
        QString(32, QChar(u'b')),
        gameplay_logic::ChartData::Keymode::K7,
        0,
        std::move(guid),
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
        length,
        120.0,
    };
    auto runner = std::make_unique<gameplay_logic::ChartRunner>(
      makeChart().release(),
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
    return { std::move(runner), player, score };
}

void
addJudgement(gameplay_logic::BmsLiveScore* score,
             gameplay_logic::Judgement judgement,
             double points)
{
    score->addHit(gameplay_logic::HitEvent{
      0,
      std::optional<int>{ 0 },
      0,
      gameplay_logic::BmsPoints{ points, judgement, 0 },
      gameplay_logic::HitEvent::Action::Press,
      true });
}

void
addRepresentativeJudgements(gameplay_logic::BmsLiveScore* score)
{
    addJudgement(score, gameplay_logic::Judgement::Perfect, 2.0);
    addJudgement(score, gameplay_logic::Judgement::Perfect, 2.0);
    addJudgement(score, gameplay_logic::Judgement::Great, 1.0);
    addJudgement(score, gameplay_logic::Judgement::Good, 0.0);
    addJudgement(score, gameplay_logic::Judgement::Bad, 0.0);
    addJudgement(score, gameplay_logic::Judgement::Poor, 0.0);
    addJudgement(score, gameplay_logic::Judgement::EmptyPoor, 0.0);
}

auto
gaugeInfo(QString name, double maximum, double threshold, double finalValue)
  -> gameplay_logic::BmsGaugeInfo
{
    return { .maxGauge = maximum,
             .threshold = threshold,
             .name = std::move(name),
             .courseGauge = false,
             .gaugeHistory = {
               gameplay_logic::rules::GaugeHistoryEntry{ 0, finalValue } } };
}

auto
makeFinalScore(QString guid = QStringLiteral("arena-score-guid"),
               QString clearType = QStringLiteral("NORMAL"),
               double points = 5.0,
               QList<int> judgements = { 1, 1, 1, 1, 1, 2 },
               int maxCombo = 4,
               QList<gameplay_logic::BmsGaugeInfo> gauges = {
                 gaugeInfo(QStringLiteral("FC"), 100.0, 100.0, 99.0),
                 gaugeInfo(QStringLiteral("NORMAL"), 100.0, 80.0, 81.0),
               }) -> std::unique_ptr<gameplay_logic::BmsScore>
{
    auto result = std::make_unique<gameplay_logic::BmsResult>(
      200.0,
      100,
      100,
      0,
      0,
      0,
      0,
      std::move(clearType),
      std::move(judgements),
      0,
      points,
      maxCombo,
      0,
      1'000'000'000,
      QList<qint64>{},
      0,
      resource_managers::NoteOrderAlgorithm::Normal,
      resource_managers::NoteOrderAlgorithm::Normal,
      resource_managers::DpOptions::Off,
      gameplay_logic::ChartData::Keymode::K7,
      guid,
      QString(64, QChar(u'a')),
      QString(32, QChar(u'b')));
    return std::make_unique<gameplay_logic::BmsScore>(
      std::move(result),
      std::make_unique<gameplay_logic::BmsReplayData>(
        QList<gameplay_logic::HitEvent>{}, guid),
      std::make_unique<gameplay_logic::BmsGaugeHistory>(std::move(gauges),
                                                        guid));
}

auto
failure(const auto& value) -> arena::ArenaGameplayCaptureFailure
{
    REQUIRE_FALSE(value.has_value());
    return value.error();
}

} // namespace

TEST_CASE("ArenaGameplaySource samples bounded P1 runner state",
          "[arena][ArenaGameplaySource]")
{
    auto fixture = makeRunner({
      new FixedGauge(QStringLiteral("FC"), 100.0, 99.0, 100.0),
      new FixedGauge(QStringLiteral("NORMAL"), 100.0, 81.0, 80.0),
    });
    addRepresentativeJudgements(fixture.score);
    fixture.player->update(500ms, false);
    arena::QtArenaGameplaySource source;

    const auto attached = source.attach(fixture.runner.get());
    REQUIRE(attached.has_value());
    CHECK(*attached == QStringLiteral("arena-score-guid"));
    const auto sampled = source.sample(7);
    REQUIRE(sampled.has_value());
    CHECK(sampled->sequence == 7);
    CHECK(sampled->exScore == 5);
    CHECK(sampled->progressPermille == 500);
    CHECK(sampled->maxCombo == 4);
    CHECK(sampled->badPoorCount == 3);
    CHECK(sampled->judgements.perfect == 2);
    CHECK(sampled->judgements.great == 1);
    CHECK(sampled->judgements.good == 1);
    CHECK(sampled->judgements.bad == 1);
    CHECK(sampled->judgements.poor == 1);
    CHECK(sampled->judgements.emptyPoor == 1);
    CHECK(sampled->gauge.type == arena::GaugeType::Normal);
    CHECK(sampled->gauge.valueMilli == 81'000);
}

TEST_CASE("ArenaGameplaySource maps every public gauge and progress edge",
          "[arena][ArenaGameplaySource]")
{
    const std::array gauges{
        std::pair{ "FC", arena::GaugeType::Fc },
        std::pair{ "EXHARD", arena::GaugeType::ExHard },
        std::pair{ "HARD", arena::GaugeType::Hard },
        std::pair{ "NORMAL", arena::GaugeType::Normal },
        std::pair{ "EASY", arena::GaugeType::Easy },
        std::pair{ "AEASY", arena::GaugeType::AssistEasy },
    };
    for (const auto& [name, expected] : gauges) {
        auto fixture = makeRunner(
          { new FixedGauge(QString::fromLatin1(name), 200.0, 50.0, 40.0) });
        arena::QtArenaGameplaySource source;
        REQUIRE(source.attach(fixture.runner.get()).has_value());
        const auto sampled = source.sample(1);
        REQUIRE(sampled.has_value());
        CHECK(sampled->gauge.type == expected);
        CHECK(sampled->gauge.valueMilli == 25'000);
    }

    for (const auto& [value, expected] :
         std::array{ std::pair{ -10.0, 0LL }, std::pair{ 250.0, 100'000LL } }) {
        auto clamped = makeRunner(
          { new FixedGauge(QStringLiteral("EASY"), 200.0, value, 300.0) });
        arena::QtArenaGameplaySource source;
        REQUIRE(source.attach(clamped.runner.get()).has_value());
        const auto sampled = source.sample(1);
        REQUIRE(sampled.has_value());
        CHECK(sampled->gauge.valueMilli == expected);
    }

    auto fixture = makeRunner(
      { new FixedGauge(QStringLiteral("NORMAL"), 100.0, 50.0, 80.0) });
    arena::QtArenaGameplaySource source;
    REQUIRE(source.attach(fixture.runner.get()).has_value());
    fixture.player->update(-100ms, false);
    REQUIRE(source.sample(1).has_value());
    CHECK(source.sample(1)->progressPermille == 0);
    fixture.player->update(2s, false);
    REQUIRE(source.sample(2).has_value());
    CHECK(source.sample(2)->progressPermille == 1000);

    auto zeroLength = makeRunner(
      { new FixedGauge(QStringLiteral("NORMAL"), 100.0, 50.0, 80.0) }, 0ns);
    arena::QtArenaGameplaySource zeroSource;
    REQUIRE(zeroSource.attach(zeroLength.runner.get()).has_value());
    zeroLength.player->update(1s, false);
    REQUIRE(zeroSource.sample(1).has_value());
    CHECK(zeroSource.sample(1)->progressPermille == 0);
}

TEST_CASE("ArenaGameplaySource rejects invalid live values",
          "[arena][ArenaGameplaySource]")
{
    SECTION("unsupported gauge")
    {
        auto fixture = makeRunner(
          { new FixedGauge(QStringLiteral("DAN"), 100.0, 100.0, 0.0) });
        arena::QtArenaGameplaySource source;
        REQUIRE(source.attach(fixture.runner.get()).has_value());
        CHECK(failure(source.sample(1)) ==
              arena::ArenaGameplayCaptureFailure::UnsupportedGauge);
    }
    SECTION("invalid gauge maximum")
    {
        auto fixture = makeRunner(
          { new FixedGauge(QStringLiteral("NORMAL"), 0.0, 0.0, 0.0) });
        arena::QtArenaGameplaySource source;
        REQUIRE(source.attach(fixture.runner.get()).has_value());
        CHECK(failure(source.sample(1)) ==
              arena::ArenaGameplayCaptureFailure::InvalidNumber);
    }
    SECTION("zero sequence")
    {
        auto fixture = makeRunner(
          { new FixedGauge(QStringLiteral("NORMAL"), 100.0, 100.0, 0.0) });
        arena::QtArenaGameplaySource source;
        REQUIRE(source.attach(fixture.runner.get()).has_value());
        CHECK(failure(source.sample(0)) ==
              arena::ArenaGameplayCaptureFailure::InvalidNumber);
    }
    for (const auto points :
         { std::numeric_limits<double>::quiet_NaN(), 1.5, 100'000'001.0 }) {
        auto fixture = makeRunner(
          { new FixedGauge(QStringLiteral("NORMAL"), 100.0, 100.0, 0.0) });
        addJudgement(fixture.score, gameplay_logic::Judgement::Perfect, points);
        arena::QtArenaGameplaySource source;
        REQUIRE(source.attach(fixture.runner.get()).has_value());
        CHECK(failure(source.sample(1)) ==
              arena::ArenaGameplayCaptureFailure::InvalidNumber);
    }
}

TEST_CASE("ArenaGameplaySource captures finals independently of IR state",
          "[arena][ArenaGameplaySource]")
{
    auto fixture = makeRunner(
      { new FixedGauge(QStringLiteral("NORMAL"), 100.0, 100.0, 0.0) });
    arena::QtArenaGameplaySource source;
    REQUIRE(source.attach(fixture.runner.get()).has_value());

    std::optional<arena::FinalResult> baseline;
    const std::array states{
        gameplay_logic::BmsScore::SubmissionState::Submitting,
        gameplay_logic::BmsScore::SubmissionState::Submitted,
        gameplay_logic::BmsScore::SubmissionState::Failed,
        gameplay_logic::BmsScore::SubmissionState::Duplicate,
        gameplay_logic::BmsScore::SubmissionState::NotSubmitting,
    };
    for (const auto state : states) {
        auto score = makeFinalScore();
        score->setSubmissionState(state);
        const auto captured = source.captureFinal(score.get());
        REQUIRE(captured.has_value());
        CHECK(captured->exScore == 5);
        CHECK(captured->maxCombo == 4);
        CHECK(captured->badPoorCount == 3);
        CHECK(captured->judgements.perfect == 2);
        CHECK(captured->clearType == arena::ClearType::Normal);
        CHECK(captured->finalGauge.type == arena::GaugeType::Normal);
        CHECK(captured->finalGauge.valueMilli == 81'000);
        if (baseline) {
            CHECK(*captured == *baseline);
        } else {
            baseline = *captured;
        }
    }
}

TEST_CASE("ArenaGameplaySource maps every final clear",
          "[arena][ArenaGameplaySource]")
{
    auto fixture = makeRunner(
      { new FixedGauge(QStringLiteral("NORMAL"), 100.0, 100.0, 0.0) });
    arena::QtArenaGameplaySource source;
    REQUIRE(source.attach(fixture.runner.get()).has_value());
    const std::array clears{
        std::pair{ "MAX", arena::ClearType::Max },
        std::pair{ "PERFECT", arena::ClearType::Perfect },
        std::pair{ "FC", arena::ClearType::FullCombo },
        std::pair{ "EXHARD", arena::ClearType::ExHard },
        std::pair{ "HARD", arena::ClearType::Hard },
        std::pair{ "NORMAL", arena::ClearType::Normal },
        std::pair{ "EASY", arena::ClearType::Easy },
        std::pair{ "AEASY", arena::ClearType::AssistEasy },
        std::pair{ "FAILED", arena::ClearType::Failed },
    };
    for (const auto& [name, expected] : clears) {
        const auto captured =
          source.captureFinal(makeFinalScore(QStringLiteral("arena-score-guid"),
                                             QString::fromLatin1(name))
                                .get());
        REQUIRE(captured.has_value());
        CHECK(captured->clearType == expected);
    }

    auto zero = makeFinalScore(QStringLiteral("arena-score-guid"),
                               QStringLiteral("FAILED"),
                               0.0,
                               { 0, 0, 0, 0, 0, 0 },
                               0);
    const auto zeroCaptured = source.captureFinal(zero.get());
    REQUIRE(zeroCaptured.has_value());
    CHECK(zeroCaptured->exScore == 0);
    CHECK(zeroCaptured->badPoorCount == 0);

    auto fallback =
      makeFinalScore(QStringLiteral("arena-score-guid"),
                     QStringLiteral("FAILED"),
                     5.0,
                     { 1, 1, 1, 1, 1, 2 },
                     4,
                     { gaugeInfo(QStringLiteral("HARD"), 100.0, 80.0, 70.0),
                       gaugeInfo(QStringLiteral("EASY"), 100.0, 80.0, 40.0) });
    const auto fallbackCaptured = source.captureFinal(fallback.get());
    REQUIRE(fallbackCaptured.has_value());
    CHECK(fallbackCaptured->finalGauge.type == arena::GaugeType::Easy);
    CHECK(fallbackCaptured->finalGauge.valueMilli == 40'000);
}

TEST_CASE("ArenaGameplaySource enforces score identity and lifetime",
          "[arena][ArenaGameplaySource]")
{
    auto fixture = makeRunner(
      { new FixedGauge(QStringLiteral("NORMAL"), 100.0, 100.0, 0.0) });
    arena::QtArenaGameplaySource source;
    REQUIRE(source.attach(fixture.runner.get()).has_value());

    auto wrong = makeFinalScore(QStringLiteral("other-score-guid"));
    CHECK(failure(source.captureFinal(wrong.get())) ==
          arena::ArenaGameplayCaptureFailure::WrongScore);
    auto unsupported = makeFinalScore(QStringLiteral("arena-score-guid"),
                                      QStringLiteral("NOPLAY"));
    CHECK(failure(source.captureFinal(unsupported.get())) ==
          arena::ArenaGameplayCaptureFailure::InvalidResult);
    for (const auto points :
         { std::numeric_limits<double>::quiet_NaN(), 1.5, 100'000'001.0 }) {
        auto invalid = makeFinalScore(QStringLiteral("arena-score-guid"),
                                      QStringLiteral("FAILED"),
                                      points,
                                      { 0, 0, 0, 0, 0, 0 },
                                      0);
        CHECK(failure(source.captureFinal(invalid.get())) ==
              arena::ArenaGameplayCaptureFailure::InvalidNumber);
    }

    source.detach();
    CHECK(failure(source.sample(1)) ==
          arena::ArenaGameplayCaptureFailure::NoRunner);
    auto final = makeFinalScore();
    CHECK(failure(source.captureFinal(final.get())) ==
          arena::ArenaGameplayCaptureFailure::NoRunner);

    REQUIRE(source.attach(fixture.runner.get()).has_value());
    fixture.runner.reset();
    CHECK(failure(source.sample(2)) ==
          arena::ArenaGameplayCaptureFailure::NoRunner);
    auto capturedAfterRunnerDestruction = source.captureFinal(final.get());
    REQUIRE(capturedAfterRunnerDestruction.has_value());
    CHECK(capturedAfterRunnerDestruction->exScore == 5);
}

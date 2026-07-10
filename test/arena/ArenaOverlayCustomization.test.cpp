#include "gameplay_logic/BmsGameReferee.h"
#include "gameplay_logic/BmsLiveScore.h"
#include "gameplay_logic/BmsNotes.h"
#include "gameplay_logic/BmsReplayData.h"
#include "gameplay_logic/ChartData.h"
#include "gameplay_logic/ChartRunner.h"
#include "gameplay_logic/NoteState.h"
#include "gameplay_logic/rules/HitRules.h"
#include "qml_components/Bga.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QPromise>
#include <QThread>

#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
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
    static char applicationName[] = "ArenaOverlayCustomizationTests";
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
makeChart() -> std::unique_ptr<gameplay_logic::ChartData>
{
    return std::make_unique<gameplay_logic::ChartData>(
      QStringLiteral("Arena customization"),
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
      60'000,
      120.0,
      120.0,
      120.0,
      120.0,
      120.0,
      0.0,
      0.0,
      0.0,
      QStringLiteral("arena-customization.bms"),
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
makeReadyRunner() -> RunnerFixture
{
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
        {},
        resource_managers::NoteOrderAlgorithm::Normal,
        resource_managers::NoteOrderAlgorithm::Normal,
        resource_managers::DpOptions::Off,
        {},
        0,
        std::chrono::duration_cast<std::chrono::nanoseconds>(60s).count(),
        QString(64, QChar(u'a')),
        QString(32, QChar(u'b')),
        gameplay_logic::ChartData::Keymode::K7,
        0,
        QStringLiteral("arena-customization-score"),
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
        60s,
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

auto
monotonicNowMs() -> int64_t
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

auto
countActions(const gameplay_logic::BmsLiveScore& score,
             gameplay_logic::HitEvent::Action action) -> int
{
    const auto replay = score.getReplayData();
    return static_cast<int>(std::ranges::count_if(
      replay->getHitEvents(),
      [action](const auto& event) { return event.getAction() == action; }));
}

auto
qmlSource(const char* relativePath) -> QString
{
    const auto path = QDir(QStringLiteral(ARENA_QML_SOURCE_ROOT))
                        .filePath(QString::fromUtf8(relativePath));
    QFile file(path);
    INFO("QML source: " << path.toStdString());
    REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    return QString::fromUtf8(file.readAll());
}

} // namespace

TEST_CASE("ArenaOverlayCustomization: suppression releases held lanes once and "
          "restores input",
          "[arena][ArenaOverlayCustomization]")
{
    auto fixture = makeReadyRunner();
    fixture.runner->start();
    REQUIRE(fixture.runner->getStatus() ==
            gameplay_logic::ChartRunner::Running);
    REQUIRE_FALSE(fixture.runner->inputSuppressed());

    fixture.runner->passKey(input::BmsKey::Col11,
                            gameplay_logic::ChartRunner::EventType::KeyPress,
                            monotonicNowMs());
    CHECK(countActions(*fixture.score,
                       gameplay_logic::HitEvent::Action::Press) == 1);

    fixture.runner->setInputSuppressed(true);
    CHECK(fixture.runner->inputSuppressed());
    CHECK(countActions(*fixture.score,
                       gameplay_logic::HitEvent::Action::Release) == 1);

    fixture.runner->setInputSuppressed(true);
    fixture.runner->passKey(input::BmsKey::Col12,
                            gameplay_logic::ChartRunner::EventType::KeyPress,
                            monotonicNowMs());
    fixture.runner->passKey(input::BmsKey::Col12,
                            gameplay_logic::ChartRunner::EventType::KeyRelease,
                            monotonicNowMs());
    CHECK(countActions(*fixture.score,
                       gameplay_logic::HitEvent::Action::Press) == 1);
    CHECK(countActions(*fixture.score,
                       gameplay_logic::HitEvent::Action::Release) == 1);

    fixture.runner->setInputSuppressed(false);
    fixture.runner->passKey(input::BmsKey::Col12,
                            gameplay_logic::ChartRunner::EventType::KeyPress,
                            monotonicNowMs());
    fixture.runner->passKey(input::BmsKey::Col12,
                            gameplay_logic::ChartRunner::EventType::KeyRelease,
                            monotonicNowMs());
    CHECK_FALSE(fixture.runner->inputSuppressed());
    CHECK(countActions(*fixture.score,
                       gameplay_logic::HitEvent::Action::Press) == 2);
    CHECK(countActions(*fixture.score,
                       gameplay_logic::HitEvent::Action::Release) == 2);
}

TEST_CASE("ArenaOverlayCustomization: suppression releases a held lane during "
          "the synchronized-start wait",
          "[arena][ArenaOverlayCustomization]")
{
    auto fixture = makeReadyRunner();
    REQUIRE(fixture.runner->getStatus() == gameplay_logic::ChartRunner::Ready);
    auto visualActions = QList<gameplay_logic::HitEvent::Action>{};
    QObject::connect(fixture.score,
                     &gameplay_logic::BmsLiveScore::hit,
                     [&visualActions](const gameplay_logic::HitEvent& hit) {
                         visualActions.push_back(hit.getAction());
                     });

    fixture.runner->passKey(input::BmsKey::Col11,
                            gameplay_logic::ChartRunner::EventType::KeyPress,
                            monotonicNowMs());
    fixture.runner->setInputSuppressed(true);

    REQUIRE(visualActions.size() == 2);
    CHECK(visualActions.at(0) == gameplay_logic::HitEvent::Action::Press);
    CHECK(visualActions.at(1) == gameplay_logic::HitEvent::Action::Release);
}

TEST_CASE("ArenaOverlayCustomization: suppression leaves chart time and status "
          "running",
          "[arena][ArenaOverlayCustomization]")
{
    auto fixture = makeReadyRunner();
    fixture.runner->start();
    REQUIRE(fixture.runner->getStatus() ==
            gameplay_logic::ChartRunner::Running);
    const auto elapsedBefore = fixture.player->getElapsed();

    fixture.runner->setInputSuppressed(true);
    QElapsedTimer timeout;
    timeout.start();
    while (fixture.player->getElapsed() <= elapsedBefore &&
           timeout.elapsed() < 250) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(1);
    }

    CHECK(fixture.runner->getStatus() == gameplay_logic::ChartRunner::Running);
    CHECK(fixture.player->getElapsed() > elapsedBefore);
}

TEST_CASE("ArenaOverlayCustomization: finish clears tracked lanes without a "
          "late release",
          "[arena][ArenaOverlayCustomization]")
{
    auto fixture = makeReadyRunner();
    fixture.runner->start();
    fixture.runner->passKey(input::BmsKey::Col11,
                            gameplay_logic::ChartRunner::EventType::KeyPress,
                            monotonicNowMs());
    const auto scores = fixture.runner->finish();
    for (auto* score : scores) {
        delete score;
    }
    const auto releasesAtFinish =
      countActions(*fixture.score, gameplay_logic::HitEvent::Action::Release);
    fixture.runner->setInputSuppressed(true);

    CHECK(fixture.runner->getStatus() == gameplay_logic::ChartRunner::Finished);
    CHECK(
      countActions(*fixture.score, gameplay_logic::HitEvent::Action::Release) ==
      releasesAtFinish);
}

TEST_CASE("ArenaOverlayCustomization: QML routes one Arena F2 owner and keeps "
          "legacy wrappers capability-free",
          "[arena][ArenaOverlayCustomization]")
{
    const auto host = qmlSource("RhythmGameQml/Arena/ArenaOverlayHost.qml");
    CHECK(host.count(QStringLiteral("sequence: \"F2\"")) == 1);
    CHECK(host.contains(QStringLiteral("setOverlayCustomizationActive")));
    CHECK(host.contains(QStringLiteral("arenaLegacyCustomizationShield")));

    const auto content = qmlSource("RhythmGameQml/ContentFrame.qml");
    CHECK(content.contains(
      QStringLiteral("!arenaOverlayHost.arenaShortcutEnabled")));

    const auto defaultGameplay = qmlSource(
      "share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml");
    CHECK(defaultGameplay.contains(
      QStringLiteral("function setArenaCustomizeMode(active)")));
    CHECK(defaultGameplay.contains(
      QStringLiteral("root.enabled && !root.arenaGameplayOwned")));

    const auto legacy = qmlSource("RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml");
    CHECK_FALSE(legacy.contains(
      QStringLiteral("function setArenaCustomizeMode(active)")));
    CHECK(legacy.contains(
      QStringLiteral("!root.arenaSession.overlayCustomizationActive")));
}

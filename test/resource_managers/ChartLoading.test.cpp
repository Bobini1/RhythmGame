#include "resource_managers/ChartFactory.h"
#include "resource_managers/SongAssetStore.h"
#include "resource_managers/DefineDb.h"
#include "qml_components/ChartLoader.h"
#include "gameplay_logic/CourseRunner.h"
#include "sounds/AudioEngine.h"
#include "gameplay_logic/rules/Lr2TimingWindows.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QImage>
#include <QFile>
#include <QJsonArray>
#include <QScopeGuard>
#include <QThread>
#include <QTimer>
#include <QVideoFrameFormat>

namespace resource_managers {
auto
loadBmpVideo(const std::filesystem::path& path, std::stop_token stop)
  -> std::unique_ptr<QMediaPlayer>;
}

namespace {
void
ensureApplication()
{
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static char name[] = "ChartLoading.test";
        static char* argv[] = { name, nullptr };
        static QCoreApplication application(argc, argv);
    }
}

auto
playerData(gameplay_logic::BmsScore* replay = nullptr)
  -> resource_managers::ChartFactory::PlayerSpecificData
{
    return { .profile = nullptr,
             .gauges = {},
             .hitRules =
               gameplay_logic::rules::HitRules{
                 gameplay_logic::rules::lr2_timing_windows::judgeNormal(),
                 [](std::chrono::nanoseconds, gameplay_logic::Judgement) {
                     return 2.0;
                 } },
             .replayedScore = replay,
             .noteOrderAlgorithm =
               resource_managers::NoteOrderAlgorithm::Normal,
             .noteOrderAlgorithmP2 =
               resource_managers::NoteOrderAlgorithm::Normal,
             .randomSeed = 0 };
}

auto
chart(const std::filesystem::path& directory)
  -> resource_managers::ChartDataFactory::ChartComponents
{
    return resource_managers::ChartDataFactory{}.loadChartData(
      "#TITLE Loading test\n#BPM 120\n#BMP01 image.png\n#00004:01\n#00111:01\n",
      directory / "chart.bms",
      [](auto range) { return range; });
}

template<typename Runner>
void
waitUntilReady(Runner& runner)
{
    QElapsedTimer timer;
    timer.start();
    while (runner.getStatus() == gameplay_logic::ChartRunner::Loading &&
           timer.elapsed() < 5000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    REQUIRE(runner.getStatus() == gameplay_logic::ChartRunner::Ready);
}
}

TEST_CASE("Chart loading reaches ready with image BGA and both players",
          "[ChartLoading]")
{
    ensureApplication();
    qputenv("RHYTHMGAME_AUDIO_BACKEND", "Null");
    sounds::AudioEngine engine;
    resource_managers::SongAssetStore assets;
    resource_managers::ChartFactory factory(&engine, nullptr, &assets);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    QImage image(256, 256, QImage::Format_RGBA8888);
    image.fill(Qt::blue);
    REQUIRE(image.save(directory.filePath("image.png")));
    const auto runner =
      factory.createChart(chart(support::qStringToPath(directory.path())),
                          playerData(),
                          playerData(),
                          2.0);
    waitUntilReady(*runner);
    REQUIRE(runner->getBga());
    CHECK(runner->getPlayer1()->getStatus() ==
          gameplay_logic::ChartRunner::Ready);
    CHECK(runner->getPlayer2()->getStatus() ==
          gameplay_logic::ChartRunner::Ready);
}

TEST_CASE("Cancelling chart loading does not wait for GUI callbacks",
          "[ChartLoading]")
{
    ensureApplication();
    qputenv("RHYTHMGAME_AUDIO_BACKEND", "Null");
    sounds::AudioEngine engine;
    resource_managers::SongAssetStore assets;
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    QElapsedTimer elapsed;
    elapsed.start();
    {
        resource_managers::ChartFactory factory(&engine, nullptr, &assets);
        auto runner =
          factory.createChart(chart(support::qStringToPath(directory.path())),
                              playerData(),
                              std::nullopt,
                              2.0);
        // Allow the worker to queue video setup without dispatching GUI events.
        QThread::msleep(50);
        runner.reset();
    }
    CHECK(elapsed.elapsed() < 5000);
    QCoreApplication::processEvents();
}

TEST_CASE("Video BGA loading accepts cancellation during media setup",
          "[ChartLoading]")
{
    ensureApplication();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    std::stop_source stop;
    QTimer cancellation;
    cancellation.setSingleShot(true);
    QObject::connect(
      &cancellation, &QTimer::timeout, [&] { stop.request_stop(); });
    cancellation.start(0);
    const auto video = resource_managers::loadBmpVideo(
      support::qStringToPath(directory.filePath("missing.mp4")),
      stop.get_token());
    CHECK_FALSE(video);
    // FFmpeg loads asynchronously, so the timer fires in the media wait loop.
    CHECK(stop.stop_requested());
}

TEST_CASE("Replay remains valid after its source score is destroyed",
          "[ChartLoading][BmsScore]")
{
    ensureApplication();
    qputenv("RHYTHMGAME_AUDIO_BACKEND", "Null");
    sounds::AudioEngine engine;
    resource_managers::SongAssetStore assets;
    resource_managers::ChartFactory factory(&engine, nullptr, &assets);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = support::qStringToPath(directory.path());
    auto components = chart(path);
    components.bmps.clear();
    const auto original =
      factory.createChart(components, playerData(), std::nullopt, 2.0);
    auto* live = original->getPlayer1()->getScore();
    auto result = live->getResult();
    auto replay = std::make_unique<gameplay_logic::BmsReplayData>(
      QList<gameplay_logic::HitEvent>{
        gameplay_logic::HitEvent{ 0,
                                  input::BmsKey::Col11,
                                  0,
                                  2'000'000'000,
                                  std::nullopt,
                                  gameplay_logic::HitEvent::Action::Press,
                                  true } },
      result->getGuid());
    auto source = std::make_unique<gameplay_logic::BmsScore>(
      std::move(result), std::move(replay), live->getGaugeHistory());
    source->setSubmissionState(
      gameplay_logic::BmsScore::SubmissionState::Submitted);
    const auto expected = source->getReplayData()->toJsonArray();
    const auto runner = factory.createChart(
      components, playerData(source.get()), std::nullopt, 2.0);
    auto* player =
      qobject_cast<gameplay_logic::RePlayer*>(runner->getPlayer1());
    REQUIRE(player);
    REQUIRE(player->getReplayedScore() != source.get());
    source.reset();
    waitUntilReady(*runner);
    CHECK(player->getReplayedScore()->parent() == player);
    CHECK(player->getReplayedScore()->getReplayData()->toJsonArray() ==
          expected);
    CHECK(player->getReplayedScore()->getSubmissionState() ==
          gameplay_logic::BmsScore::SubmissionState::Submitted);
    player->update(std::chrono::seconds(2), false);
    CHECK(player->getScore()->getReplayData()->getHitEvents().size() > 0);
}

TEST_CASE("Course replay owns the scores for stages loaded later",
          "[ChartLoading][BmsScore]")
{
    ensureApplication();
    qputenv("RHYTHMGAME_AUDIO_BACKEND", "Null");
    sounds::AudioEngine engine;
    resource_managers::SongAssetStore assets;
    resource_managers::ChartFactory factory(&engine, nullptr, &assets);
    resource_managers::ChartDataFactory dataFactory;
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto root = support::qStringToPath(directory.path());
    db::SqliteCppDb database(root / "songs.sqlite");
    resource_managers::defineDb(database);
    QNetworkAccessManager network;
    resource_managers::Profile profile(root / "songs.sqlite",
                                       root / "profile.sqlite",
                                       {},
                                       {},
                                       &network,
                                       &assets);
    QFile file(directory.filePath("course.bms"));
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write("#TITLE Course replay\n#BPM 120\n#00111:01\n") > 0);
    file.close();
    auto components = dataFactory.loadChartData(
      root / "course.bms", [](auto range) { return range; });
    components.chartData->save(database);
    const auto seed =
      factory.createChart(components, playerData(), std::nullopt, 2.0);
    auto* live = seed->getPlayer1()->getScore();
    auto stages = QList<gameplay_logic::BmsScore*>{};
    for (int stage = 0; stage < 2; ++stage) {
        stages.append(new gameplay_logic::BmsScore(
          live->getResult(), live->getReplayData(), live->getGaugeHistory()));
    }
    resource_managers::Course course{
        .db = &database,
        .name = "Replay course",
        .md5s = { components.chartData->getMd5(),
                  components.chartData->getMd5() },
        .paths = { directory.filePath("course.bms"),
                   directory.filePath("course.bms") }
    };
    auto source = gameplay_logic::BmsScoreCourse::fromScores(
      std::make_unique<gameplay_logic::BmsResultCourse>(
        "course-guid", course.getIdentifier(), stages, "DAN", 0, QStringList{}),
      stages);
    qml_components::ChartLoader loader(
      nullptr,
      nullptr,
      &dataFactory,
      &assets,
      [](std::chrono::nanoseconds, gameplay_logic::Judgement) { return 2.0; },
      [](auto*, double, int) {
          return QList<gameplay_logic::rules::BmsGauge*>{};
      },
      [](auto*, const auto&) {
          return QList<gameplay_logic::rules::BmsGauge*>{};
      },
      {},
      &factory,
      &database);
    SECTION("A saved partial course cannot supply a replay or note pattern")
    {
        QList<gameplay_logic::BmsScore*> partialStages{
            source->getScores().front()->clone().release()
        };
        auto partial = gameplay_logic::BmsScoreCourse::fromScores(
          std::make_unique<gameplay_logic::BmsResultCourse>(
            "partial",
            course.getIdentifier(),
            partialStages,
            "FAILED",
            0,
            QStringList{}),
          partialStages);
        for (const bool replay : { false, true }) {
            CHECK_FALSE(loader.loadCourse(course,
                                          &profile,
                                          false,
                                          replay,
                                          partial.get(),
                                          nullptr,
                                          false,
                                          false,
                                          nullptr));
        }
        return;
    }
    SECTION("Course mirror constraint preserves only permitted Mirror")
    {
        profile.getVars()->getGeneralVars()->setNoteOrderAlgorithm(
          resource_managers::NoteOrderAlgorithm::Mirror);
        for (const bool allowed : { false, true }) {
            course.constraints =
              allowed ? QStringList{ "grade_mirror" } : QStringList{};
            auto manual = std::unique_ptr<gameplay_logic::CourseRunner>(
              loader.loadCourse(course,
                                &profile,
                                false,
                                false,
                                nullptr,
                                nullptr,
                                false,
                                false,
                                nullptr));
            REQUIRE(manual);
            CHECK(manual->getPlayer1()->getScore()->getNoteOrderAlgorithm() ==
                  (allowed ? resource_managers::NoteOrderAlgorithm::Mirror
                           : resource_managers::NoteOrderAlgorithm::Normal));
        }
        return;
    }
    SECTION("A complete course retains the later stages") {}
    auto runner = std::unique_ptr<gameplay_logic::CourseRunner>(
      loader.loadCourse(course,
                        &profile,
                        false,
                        true,
                        source.get(),
                        nullptr,
                        false,
                        false,
                        nullptr));
    REQUIRE(runner);
    source.reset();
    waitUntilReady(*runner);
    REQUIRE(qobject_cast<gameplay_logic::RePlayer*>(runner->getPlayer1()));
    runner->proceed();
    CHECK(runner->getCurrentChartIndex() == 1);
    waitUntilReady(*runner);
    auto* second =
      qobject_cast<gameplay_logic::RePlayer*>(runner->getPlayer1());
    REQUIRE(second);
    CHECK(second->getReplayedScore()->getResult()->getMd5() == course.md5s[1]);
    runner.reset();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

TEST_CASE("BGA restores its frame or movie when its video sink is replaced",
          "[ChartLoading][Bga]")
{
    ensureApplication();
    QVideoSink first;
    QVideoSink second;
    SECTION("Still image and detach")
    {
        QVideoFrame image(
          QVideoFrameFormat(QSize(8, 8), QVideoFrameFormat::Format_RGBA8888));
        qml_components::Bga bga({},
                                { { std::chrono::nanoseconds(0), &image } });
        bga.setVideoSink(&first);
        bga.update(std::chrono::nanoseconds(1));
        CHECK(first.videoFrame().isValid());
        bga.setVideoSink(nullptr);
        bga.setVideoSink(&second);
        CHECK(second.videoFrame().size() == QSize(8, 8));
    }
    SECTION("Current movie")
    {
        QMediaPlayer movie;
        qml_components::Bga bga({ { std::chrono::nanoseconds(0), &movie } },
                                {});
        bga.setVideoSink(&first);
        bga.update(std::chrono::nanoseconds(1));
        CHECK(movie.videoSink() == &first);
        bga.setVideoSink(&second);
        CHECK(movie.videoSink() == &second);
    }
}

TEST_CASE("Chart metadata lookup accepts MD5 and SHA256 identifiers together",
          "[ChartLoading][ChartLoader]")
{
    ensureApplication();
    QTemporaryDir directory;
    const auto root = support::qStringToPath(directory.path());
    db::SqliteCppDb database(root / "songs.sqlite");
    resource_managers::defineDb(database);
    resource_managers::ChartDataFactory factory;
    resource_managers::SongAssetStore assets;
    const auto path = directory.filePath(QString::fromUtf8("chart # 音.bms"));
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    file.write("#TITLE Lookup\n#BPM 120\n#00111:01\n");
    file.close();
    auto chart = factory.loadChartData(support::qStringToPath(path),
                                       [](auto) { return 1; });
    chart.chartData->save(database);
    qml_components::ChartLoader loader(
      nullptr, nullptr, &factory, &assets, {}, {}, {}, {}, nullptr, &database);
    const auto md5 = chart.chartData->getMd5();
    const auto sha = chart.chartData->getSha256();
    const auto results = loader.loadChartDataFromDb(
      { md5.toLower(), "SHA256/" + sha.toLower(), "missing" });
    const auto cleanup = qScopeGuard([&] {
        for (const auto& result : results)
            delete result.value<gameplay_logic::ChartData*>();
    });
    REQUIRE(results.size() == 2);
    CHECK(results.value(md5).value<gameplay_logic::ChartData*>()->getSha256() ==
          sha);
    CHECK(results.value("sha256/" + sha)
            .value<gameplay_logic::ChartData*>()
            ->getMd5() == md5);
    const std::unique_ptr<gameplay_logic::ChartData> byFile(
      loader.loadChartData(QUrl::fromLocalFile(path).toString()));
    REQUIRE(byFile);
    CHECK(byFile->getMd5() == md5);
}

#include "arena/QtArenaRoundLoader.h"
#include "gameplay_logic/ChartData.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QThread>
#include <QThreadPool>

#include <array>
#include <memory>

namespace {

auto
makeChart(QString sha256 = QString(64, QChar(u'a')),
          QString md5 = QString(32, QChar(u'b')),
          QList<qint64> randomSequence = { 2, 1, 7 })
  -> std::unique_ptr<gameplay_logic::ChartData>
{
    return std::make_unique<gameplay_logic::ChartData>(
      QStringLiteral("Arena title"),
      QStringLiteral("Arena artist"),
      QStringLiteral("Arena subtitle"),
      QString{},
      QString{},
      QString{},
      QString{},
      QString{},
      75.0,
      160.0,
      10,
      3,
      !randomSequence.isEmpty(),
      std::move(randomSequence),
      3,
      2,
      1,
      4,
      5,
      1000,
      120.0,
      120.0,
      120.0,
      120.0,
      120.0,
      0.0,
      0.0,
      0.0,
      QStringLiteral("chart.bms"),
      0,
      std::move(sha256),
      std::move(md5),
      gameplay_logic::ChartData::Keymode::K7,
      QList<QList<qint64>>{},
      QList<gameplay_logic::BpmChange>{},
      0);
}

auto
waitUntil(const std::function<bool()>& predicate) -> bool
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < 5000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(1);
    }
    return predicate();
}

void
ensureCoreApplication()
{
    if (QCoreApplication::instance() != nullptr) {
        return;
    }
    static int argc = 1;
    static char name[] = "ArenaRoundLoader.test";
    static char* argv[] = { name, nullptr };
    static QCoreApplication application(argc, argv);
    (void)application;
}

auto
defaultConfig() -> resource_managers::ChartPlayConfig
{
    return {
        .noteOrderP1 = resource_managers::NoteOrderAlgorithm::Normal,
        .noteOrderP2 = resource_managers::NoteOrderAlgorithm::Normal,
        .dpMode = resource_managers::DpOptions::Off,
    };
}

auto
testLoader(arena::QtArenaRoundLoader::PlayConfigProvider configProvider,
           arena::QtArenaRoundLoader::PathResolver pathResolver = {},
           arena::QtArenaRoundLoader::RunnerLoader runnerLoader = {},
           arena::QtArenaRoundLoader::SeedGenerator seedGenerator = {})
  -> std::unique_ptr<arena::QtArenaRoundLoader>
{
    ensureCoreApplication();
    return std::make_unique<arena::QtArenaRoundLoader>(
      std::move(configProvider),
      std::move(pathResolver),
      std::move(runnerLoader),
      std::move(seedGenerator));
}

} // namespace

TEST_CASE("ArenaRoundLoader maps every deterministic selection option",
          "[arena][ArenaRoundLoader]")
{
    using arena::DpMode;
    using arena::NoteOrder;
    using resource_managers::DpOptions;
    using resource_managers::NoteOrderAlgorithm;

    constexpr std::array noteOrders{
        std::pair{ NoteOrderAlgorithm::Normal, NoteOrder::NormalOrMirror },
        std::pair{ NoteOrderAlgorithm::Mirror, NoteOrder::NormalOrMirror },
        std::pair{ NoteOrderAlgorithm::Random, NoteOrder::Random },
        std::pair{ NoteOrderAlgorithm::SRandom, NoteOrder::SRandom },
        std::pair{ NoteOrderAlgorithm::RRandom, NoteOrder::RRandom },
        std::pair{ NoteOrderAlgorithm::RandomPlus, NoteOrder::RandomPlus },
        std::pair{ NoteOrderAlgorithm::SRandomPlus, NoteOrder::SRandomPlus },
        std::pair{ NoteOrderAlgorithm::BeatorajaRandom,
                   NoteOrder::BeatorajaRandom },
        std::pair{ NoteOrderAlgorithm::BeatorajaRandomEx,
                   NoteOrder::BeatorajaRandomEx },
        std::pair{ NoteOrderAlgorithm::Lr2Random, NoteOrder::Lr2Random },
        std::pair{ NoteOrderAlgorithm::Lr2RandomEx, NoteOrder::Lr2RandomEx },
    };
    for (const auto& [source, expected] : noteOrders) {
        auto config = defaultConfig();
        config.noteOrderP1 = source;
        config.noteOrderP2 = source;
        auto loader = testLoader([config] { return config; },
                                 {},
                                 {},
                                 [] { return 0xfedcba9876543210ULL; });
        const auto result = loader->buildSelection(makeChart().get());
        REQUIRE(result.has_value());
        CHECK(result->noteOrderP1 == expected);
        CHECK(result->noteOrderP2 == expected);
        CHECK(result->laneSeed == QStringLiteral("fedcba9876543210"));
    }

    constexpr std::array dpModes{
        std::pair{ DpOptions::Off, DpMode::Off },
        std::pair{ DpOptions::Flip, DpMode::Flip },
        std::pair{ DpOptions::Lr2Flip, DpMode::Lr2Flip },
        std::pair{ DpOptions::Battle, DpMode::Battle },
    };
    for (const auto& [source, expected] : dpModes) {
        auto config = defaultConfig();
        config.dpMode = source;
        auto loader = testLoader([config] { return config; });
        const auto result = loader->buildSelection(makeChart().get());
        REQUIRE(result.has_value());
        CHECK(result->dpMode == expected);
    }
}

TEST_CASE("ArenaRoundLoader builds the immutable chart snapshot",
          "[arena][ArenaRoundLoader]")
{
    auto loader = testLoader([] { return defaultConfig(); },
                             {},
                             {},
                             [] { return 0x0123456789abcdefULL; });
    auto chart = makeChart(
      QString(64, QChar(u'A')), QString(32, QChar(u'B')), { 3, 1, 4 });

    const auto result = loader->buildSelection(chart.get());

    REQUIRE(result.has_value());
    CHECK(result->sha256 == QString(64, QChar(u'a')));
    CHECK(result->md5 == QString(32, QChar(u'b')));
    CHECK(result->title == QStringLiteral("Arena title"));
    CHECK(result->subtitle == QStringLiteral("Arena subtitle"));
    CHECK(result->artist == QStringLiteral("Arena artist"));
    CHECK(result->keyMode == 7);
    CHECK(result->randomSequence == QVector<qint64>{ 3, 1, 4 });
    CHECK(result->laneSeed == QStringLiteral("0123456789abcdef"));
    CHECK(result->randomizationVersion == 1);
}

TEST_CASE("ArenaRoundLoader rejects invalid selection inputs",
          "[arena][ArenaRoundLoader]")
{
    using arena::ArenaSelectionBuildFailure;
    auto loader = testLoader([] { return defaultConfig(); });

    CHECK(loader->buildSelection(nullptr).error() ==
          ArenaSelectionBuildFailure::InvalidChart);
    CHECK(
      loader->buildSelection(makeChart(QStringLiteral("bad")).get()).error() ==
      ArenaSelectionBuildFailure::InvalidSha256);
    CHECK(
      loader
        ->buildSelection(
          makeChart(QString(64, QChar(u'a')), QString(32, QChar(u'b')), { 0 })
            .get())
        .error() == ArenaSelectionBuildFailure::InvalidRandomSequence);
    QList<qint64> tooMany;
    tooMany.fill(1, arena::MaxRandomSequenceEntries + 1);
    CHECK(loader
            ->buildSelection(makeChart(QString(64, QChar(u'a')),
                                       QString(32, QChar(u'b')),
                                       std::move(tooMany))
                               .get())
            .error() == ArenaSelectionBuildFailure::InvalidRandomSequence);

    auto unsupported =
      testLoader([]() -> std::optional<resource_managers::ChartPlayConfig> {
          return std::nullopt;
      });
    CHECK(unsupported->buildSelection(makeChart().get()).error() ==
          ArenaSelectionBuildFailure::UnsupportedConfig);
}

TEST_CASE("ArenaRoundLoader probes the current local file by SHA-256",
          "[arena][ArenaRoundLoader]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("chart.bms"));
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    const auto contents = QByteArrayLiteral("arena probe contents");
    REQUIRE(file.write(contents) == contents.size());
    file.close();
    const auto digest =
      QCryptographicHash::hash(contents, QCryptographicHash::Sha256);

    auto loader = testLoader(
      [] { return defaultConfig(); },
      [path, digest](QByteArrayView requested) -> std::optional<QString> {
          return requested == digest ? std::optional{ path } : std::nullopt;
      });
    std::optional<arena::ArenaProbeResult> result;
    QObject::connect(
      loader.get(),
      &arena::ArenaRoundLoader::probeFinished,
      [&result](quint64 requestId, const arena::ArenaProbeResult& value) {
          if (requestId == 7) {
              result = value;
          }
      });

    loader->probe(7, digest);

    REQUIRE(waitUntil([&] { return result.has_value(); }));
    CHECK(result->failure == arena::ArenaProbeFailure::None);
    CHECK(result->observedSha256 == digest);
}

TEST_CASE("ArenaRoundLoader reports sanitized probe and load failures",
          "[arena][ArenaRoundLoader]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("chart.bms"));
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    const auto contents = QByteArrayLiteral("local bytes");
    REQUIRE(file.write(contents) == contents.size());
    file.close();
    const auto observed =
      QCryptographicHash::hash(contents, QCryptographicHash::Sha256);
    const auto expected = QByteArray(32, '\x01');

    auto loader = testLoader(
      [] { return defaultConfig(); },
      [path](QByteArrayView) -> std::optional<QString> { return path; },
      [](const QString&, const resource_managers::ChartPlayConfig&)
        -> gameplay_logic::ChartRunner* { return nullptr; });
    std::optional<arena::ArenaProbeResult> probe;
    QList<arena::ArenaLoadFailure> loadFailures;
    QObject::connect(loader.get(),
                     &arena::ArenaRoundLoader::probeFinished,
                     [&probe](quint64, const arena::ArenaProbeResult& value) {
                         probe = value;
                     });
    QObject::connect(loader.get(),
                     &arena::ArenaRoundLoader::loadFailed,
                     [&loadFailures](quint64, arena::ArenaLoadFailure value) {
                         loadFailures.push_back(value);
                     });

    loader->probe(1, expected);
    REQUIRE(waitUntil([&] { return probe.has_value(); }));
    CHECK(probe->failure == arena::ArenaProbeFailure::HashMismatch);
    CHECK(probe->observedSha256 == observed);

    auto unsupported = arena::ArenaRoundPlayConfig{};
    unsupported.randomizationVersion = 2;
    loader->load(2,
                 arena::ArenaRoundLoadRequest{ .sha256 = expected,
                                               .playConfig = unsupported });
    REQUIRE(waitUntil([&] { return !loadFailures.isEmpty(); }));
    CHECK(loadFailures.takeFirst() ==
          arena::ArenaLoadFailure::UnsupportedConfig);

    loader->load(
      3, arena::ArenaRoundLoadRequest{ .sha256 = expected, .playConfig = {} });
    REQUIRE(waitUntil([&] { return !loadFailures.isEmpty(); }));
    CHECK(loadFailures.takeFirst() == arena::ArenaLoadFailure::HashMismatch);
}

TEST_CASE(
  "ArenaRoundLoader loads only after a matching rehash and preserves config",
  "[arena][ArenaRoundLoader]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("chart.bms"));
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    const auto contents = QByteArrayLiteral("frozen arena chart");
    REQUIRE(file.write(contents) == contents.size());
    file.close();
    const auto digest =
      QCryptographicHash::hash(contents, QCryptographicHash::Sha256);

    const auto requestedConfig = arena::ArenaRoundPlayConfig{
        .randomSequence = { 2, 1, 3 },
        .noteOrderP1 = arena::NoteOrder::SRandomPlus,
        .noteOrderP2 = arena::NoteOrder::Lr2RandomEx,
        .dpMode = arena::DpMode::Lr2Flip,
        .laneSeed = 0xfedcba9876543210ULL,
    };
    auto expectedConfig = defaultConfig();
    expectedConfig.randomSequence = requestedConfig.randomSequence;
    expectedConfig.noteOrderP1 =
      resource_managers::NoteOrderAlgorithm::SRandomPlus;
    expectedConfig.noteOrderP2 =
      resource_managers::NoteOrderAlgorithm::Lr2RandomEx;
    expectedConfig.dpMode = resource_managers::DpOptions::Lr2Flip;
    expectedConfig.laneSeed = requestedConfig.laneSeed;
    int loadCalls = 0;
    auto loader = testLoader(
      [] { return defaultConfig(); },
      [path](QByteArrayView) -> std::optional<QString> { return path; },
      [&loadCalls,
       expectedConfig](const QString& resolvedPath,
                       const resource_managers::ChartPlayConfig& config)
        -> gameplay_logic::ChartRunner* {
          ++loadCalls;
          CHECK(resolvedPath.endsWith(QStringLiteral("chart.bms")));
          CHECK(config == expectedConfig);
          return nullptr;
      });
    std::optional<arena::ArenaLoadFailure> failure;
    QObject::connect(
      loader.get(),
      &arena::ArenaRoundLoader::loadFailed,
      [&failure](quint64 requestId, arena::ArenaLoadFailure value) {
          if (requestId == 9) {
              failure = value;
          }
      });

    loader->load(9,
                 arena::ArenaRoundLoadRequest{ .sha256 = digest,
                                               .playConfig = requestedConfig });

    REQUIRE(waitUntil([&] { return failure.has_value(); }));
    CHECK(*failure == arena::ArenaLoadFailure::ParseFailed);
    CHECK(loadCalls == 1);
}

TEST_CASE("ArenaRoundLoader resolves Normal/Mirror from local options",
          "[arena][ArenaRoundLoader]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("chart.bms"));
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    const auto contents = QByteArrayLiteral("normal mirror arena chart");
    REQUIRE(file.write(contents) == contents.size());
    file.close();
    const auto digest =
      QCryptographicHash::hash(contents, QCryptographicHash::Sha256);

    auto localConfig = defaultConfig();
    localConfig.noteOrderP1 = resource_managers::NoteOrderAlgorithm::Mirror;
    localConfig.noteOrderP2 = resource_managers::NoteOrderAlgorithm::Random;
    int loadCalls = 0;
    auto loader = testLoader(
      [localConfig] { return localConfig; },
      [path](QByteArrayView) -> std::optional<QString> { return path; },
      [&loadCalls](const QString&,
                   const resource_managers::ChartPlayConfig& config)
        -> gameplay_logic::ChartRunner* {
          ++loadCalls;
          CHECK(config.noteOrderP1 ==
                resource_managers::NoteOrderAlgorithm::Mirror);
          CHECK(config.noteOrderP2 ==
                resource_managers::NoteOrderAlgorithm::Normal);
          return nullptr;
      });
    std::optional<arena::ArenaLoadFailure> failure;
    QObject::connect(
      loader.get(),
      &arena::ArenaRoundLoader::loadFailed,
      [&failure](quint64 requestId, arena::ArenaLoadFailure value) {
          if (requestId == 10) {
              failure = value;
          }
      });

    loader->load(10,
                 arena::ArenaRoundLoadRequest{
                   .sha256 = digest,
                   .playConfig = arena::ArenaRoundPlayConfig{
                     .noteOrderP1 = arena::NoteOrder::NormalOrMirror,
                     .noteOrderP2 = arena::NoteOrder::NormalOrMirror,
                   } });

    REQUIRE(waitUntil([&] { return failure.has_value(); }));
    CHECK(*failure == arena::ArenaLoadFailure::ParseFailed);
    CHECK(loadCalls == 1);
}

TEST_CASE("ArenaRoundLoader cancellation is correlated and terminal",
          "[arena][ArenaRoundLoader]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("chart.bms"));
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    const auto contents = QByteArray(1024 * 1024, 'x');
    REQUIRE(file.write(contents) == contents.size());
    file.close();
    const auto digest =
      QCryptographicHash::hash(contents, QCryptographicHash::Sha256);

    auto loader = testLoader(
      [] { return defaultConfig(); },
      [path](QByteArrayView) -> std::optional<QString> { return path; });
    QList<arena::ArenaProbeResult> completions;
    QObject::connect(
      loader.get(),
      &arena::ArenaRoundLoader::probeFinished,
      [&completions](quint64 requestId, const arena::ArenaProbeResult& result) {
          if (requestId == 11) {
              completions.push_back(result);
          }
      });

    loader->probe(11, digest);
    loader->cancel(11);

    REQUIRE(completions.size() == 1);
    CHECK(completions.front().failure == arena::ArenaProbeFailure::Cancelled);
    CHECK(waitUntil(
      [&] { return QThreadPool::globalInstance()->activeThreadCount() == 0; }));
    QCoreApplication::processEvents();
    CHECK(completions.size() == 1);
}

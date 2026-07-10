#include "QtArenaRoundLoader.h"

#include "db/SqliteCppDb.h"
#include "gameplay_logic/ChartData.h"
#include "gameplay_logic/ChartRunner.h"
#include "qml_components/ChartLoader.h"
#include "qml_components/ProfileList.h"
#include "resource_managers/Profile.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrentRun>

#include <algorithm>
#include <cctype>
#include <random>
#include <utility>

namespace arena {
namespace {

auto
unicodeCodePointCount(QStringView value) -> std::optional<int>
{
    int count = 0;
    for (qsizetype i = 0; i < value.size(); ++i) {
        if (value[i].isHighSurrogate()) {
            if (i + 1 >= value.size() || !value[i + 1].isLowSurrogate()) {
                return std::nullopt;
            }
            ++i;
        } else if (value[i].isLowSurrogate()) {
            return std::nullopt;
        }
        ++count;
    }
    return count;
}

auto
validMetadata(QStringView value) -> bool
{
    const auto count = unicodeCodePointCount(value);
    return count && *count <= MaxSelectionMetadataCodePoints;
}

auto
isHex(QStringView value, qsizetype size) -> bool
{
    return value.size() == size &&
           std::all_of(value.begin(), value.end(), [](QChar character) {
               const auto value = character.toLower().unicode();
               return (value >= '0' && value <= '9') ||
                      (value >= 'a' && value <= 'f');
           });
}

auto
normalizedSha256(QByteArrayView value) -> std::optional<QByteArray>
{
    if (value.size() == 32) {
        return value.toByteArray();
    }
    if (value.size() != Sha256Characters ||
        !std::all_of(value.begin(), value.end(), [](char character) {
            const auto lower = static_cast<char>(
              std::tolower(static_cast<unsigned char>(character)));
            return (lower >= '0' && lower <= '9') ||
                   (lower >= 'a' && lower <= 'f');
        })) {
        return std::nullopt;
    }
    return QByteArray::fromHex(value.toByteArray());
}

auto
toArenaNoteOrder(resource_managers::NoteOrderAlgorithm value)
  -> std::optional<NoteOrder>
{
    using resource_managers::NoteOrderAlgorithm;
    switch (value) {
        case NoteOrderAlgorithm::Normal:
            return NoteOrder::Normal;
        case NoteOrderAlgorithm::Mirror:
            return NoteOrder::Mirror;
        case NoteOrderAlgorithm::Random:
            return NoteOrder::Random;
        case NoteOrderAlgorithm::SRandom:
            return NoteOrder::SRandom;
        case NoteOrderAlgorithm::RRandom:
            return NoteOrder::RRandom;
        case NoteOrderAlgorithm::RandomPlus:
            return NoteOrder::RandomPlus;
        case NoteOrderAlgorithm::SRandomPlus:
            return NoteOrder::SRandomPlus;
        case NoteOrderAlgorithm::BeatorajaRandom:
            return NoteOrder::BeatorajaRandom;
        case NoteOrderAlgorithm::BeatorajaRandomEx:
            return NoteOrder::BeatorajaRandomEx;
        case NoteOrderAlgorithm::Lr2Random:
            return NoteOrder::Lr2Random;
        case NoteOrderAlgorithm::Lr2RandomEx:
            return NoteOrder::Lr2RandomEx;
    }
    return std::nullopt;
}

auto
toArenaDpMode(resource_managers::DpOptions value) -> std::optional<DpMode>
{
    using resource_managers::DpOptions;
    switch (value) {
        case DpOptions::Off:
            return DpMode::Off;
        case DpOptions::Flip:
            return DpMode::Flip;
        case DpOptions::Lr2Flip:
            return DpMode::Lr2Flip;
        case DpOptions::Battle:
            return DpMode::Battle;
    }
    return std::nullopt;
}

auto
randomLaneSeed() -> quint64
{
    thread_local auto source = std::random_device{};
    thread_local auto generator = std::mt19937_64{ source() };
    return generator();
}

auto
validPlayConfig(const resource_managers::ChartPlayConfig& config) -> bool
{
    return config.isSupported() &&
           config.randomSequence.size() <= MaxRandomSequenceEntries &&
           std::ranges::none_of(config.randomSequence,
                                [](qint64 value) {
                                    return value < 1 ||
                                           value > MaxJsonSafeInteger;
                                }) &&
           toArenaNoteOrder(config.noteOrderP1).has_value() &&
           toArenaNoteOrder(config.noteOrderP2).has_value() &&
           toArenaDpMode(config.dpMode).has_value();
}

} // namespace

QtArenaRoundLoader::QtArenaRoundLoader(PlayConfigProvider playConfigProvider,
                                       PathResolver pathResolver,
                                       RunnerLoader runnerLoader,
                                       SeedGenerator seedGenerator,
                                       QObject* parent)
  : ArenaRoundLoader(parent)
  , m_playConfigProvider(std::move(playConfigProvider))
  , m_pathResolver(std::move(pathResolver))
  , m_runnerLoader(std::move(runnerLoader))
  , m_seedGenerator(seedGenerator ? std::move(seedGenerator)
                                  : SeedGenerator{ &randomLaneSeed })
{
}

QtArenaRoundLoader::QtArenaRoundLoader(qml_components::ProfileList* profileList,
                                       db::SqliteCppDb* songDb,
                                       qml_components::ChartLoader* chartLoader,
                                       QObject* parent)
  : QtArenaRoundLoader(
      [profileList]() -> std::optional<resource_managers::ChartPlayConfig> {
          if (profileList == nullptr ||
              profileList->getMainProfile() == nullptr) {
              return std::nullopt;
          }
          const auto* general =
            profileList->getMainProfile()->getVars()->getGeneralVars();
          if (general == nullptr) {
              return std::nullopt;
          }
          return resource_managers::ChartPlayConfig{
              .noteOrderP1 = general->getNoteOrderAlgorithm(),
              .noteOrderP2 = general->getNoteOrderAlgorithmP2(),
              .dpMode = general->getDpOptions(),
          };
      },
      [songDb](QByteArrayView sha256) -> std::optional<QString> {
          if (songDb == nullptr || sha256.size() != 32) {
              return std::nullopt;
          }
          auto query = songDb->createStatement(
            "SELECT path FROM charts WHERE lower(sha256) = ? ORDER BY path");
          query.bind(1, sha256.toByteArray().toHex().toStdString());
          const auto paths = query.executeAndGetAll<std::string>();
          for (const auto& path : paths) {
              const auto candidate = QString::fromStdString(path);
              if (QFileInfo::exists(candidate)) {
                  return candidate;
              }
          }
          return paths.empty()
                   ? std::nullopt
                   : std::optional{ QString::fromStdString(paths.front()) };
      },
      [profileList,
       chartLoader](const QString& path,
                    const resource_managers::ChartPlayConfig& config)
        -> gameplay_logic::ChartRunner* {
          if (profileList == nullptr || chartLoader == nullptr) {
              return nullptr;
          }
          return chartLoader->loadChartWithConfig(
            path, profileList->getMainProfile(), config);
      },
      {},
      parent)
{
}

QtArenaRoundLoader::~QtArenaRoundLoader()
{
    QList<QPointer<gameplay_logic::ChartRunner>> runners;
    for (auto& operation : m_operations) {
        operation.cancelled->store(true, std::memory_order_relaxed);
        if (operation.runner) {
            runners.push_back(operation.runner);
        }
    }
    m_operations.clear();
    for (const auto& runner : runners) {
        delete runner.data();
    }
}

auto
QtArenaRoundLoader::buildSelection(gameplay_logic::ChartData* chart)
  -> ArenaSelectionBuildResult
{
    if (chart == nullptr) {
        return std::unexpected(ArenaSelectionBuildFailure::InvalidChart);
    }
    const auto sha256 = chart->getSha256();
    if (!isHex(sha256, Sha256Characters)) {
        return std::unexpected(ArenaSelectionBuildFailure::InvalidSha256);
    }
    if (!validMetadata(chart->getTitle()) ||
        !validMetadata(chart->getSubtitle()) ||
        !validMetadata(chart->getArtist())) {
        return std::unexpected(ArenaSelectionBuildFailure::InvalidChart);
    }
    const auto keyMode = static_cast<int>(chart->getKeymode());
    if (keyMode != 5 && keyMode != 7 && keyMode != 10 && keyMode != 14) {
        return std::unexpected(ArenaSelectionBuildFailure::InvalidChart);
    }
    const auto& sequence = chart->getRandomSequence();
    if (sequence.size() > MaxRandomSequenceEntries ||
        std::ranges::any_of(sequence, [](qint64 value) {
            return value < 1 || value > MaxJsonSafeInteger;
        })) {
        return std::unexpected(
          ArenaSelectionBuildFailure::InvalidRandomSequence);
    }
    if (!m_playConfigProvider) {
        return std::unexpected(ArenaSelectionBuildFailure::UnsupportedConfig);
    }
    std::optional<resource_managers::ChartPlayConfig> config;
    try {
        config = m_playConfigProvider();
    } catch (...) {
        return std::unexpected(ArenaSelectionBuildFailure::UnsupportedConfig);
    }
    if (!config || !config->isSupported()) {
        return std::unexpected(ArenaSelectionBuildFailure::UnsupportedConfig);
    }
    const auto p1 = toArenaNoteOrder(config->noteOrderP1);
    const auto p2 = toArenaNoteOrder(config->noteOrderP2);
    const auto dp = toArenaDpMode(config->dpMode);
    if (!p1 || !p2 || !dp) {
        return std::unexpected(ArenaSelectionBuildFailure::UnsupportedConfig);
    }
    quint64 laneSeed{};
    try {
        laneSeed = m_seedGenerator();
    } catch (...) {
        return std::unexpected(ArenaSelectionBuildFailure::UnsupportedConfig);
    }

    SelectionSnapshot result{
        .sha256 = sha256.toLower(),
        .title = chart->getTitle(),
        .subtitle = chart->getSubtitle(),
        .artist = chart->getArtist(),
        .keyMode = keyMode,
        .randomSequence = sequence,
        .noteOrderP1 = *p1,
        .noteOrderP2 = *p2,
        .dpMode = *dp,
        .laneSeed = QString::number(laneSeed, 16)
                      .rightJustified(LaneSeedCharacters, QChar(u'0')),
        .randomizationVersion = resource_managers::chartRandomizationVersion,
    };
    const auto md5 = chart->getMd5();
    if (!md5.isEmpty()) {
        if (!isHex(md5, Md5Characters)) {
            return std::unexpected(ArenaSelectionBuildFailure::InvalidChart);
        }
        result.md5 = md5.toLower();
    }
    return result;
}

void
QtArenaRoundLoader::probe(quint64 requestId, const QByteArray& sha256)
{
    discard(requestId);
    const auto expected = normalizedSha256(sha256);
    if (!expected) {
        emit probeFinished(
          requestId,
          ArenaProbeResult{ .failure = ArenaProbeFailure::HashMismatch });
        return;
    }
    std::optional<QString> path;
    try {
        path = m_pathResolver ? m_pathResolver(*expected) : std::nullopt;
    } catch (...) {
        emit probeFinished(
          requestId,
          ArenaProbeResult{ .failure = ArenaProbeFailure::ReadFailed });
        return;
    }
    if (!path) {
        emit probeFinished(
          requestId,
          ArenaProbeResult{ .failure = ArenaProbeFailure::MissingFile });
        return;
    }
    startFileCheck(
      requestId, OperationKind::Probe, *expected, *path, std::nullopt);
}

void
QtArenaRoundLoader::load(quint64 requestId,
                         const ArenaRoundLoadRequest& request)
{
    discard(requestId);
    if (!validPlayConfig(request.playConfig)) {
        emit loadFailed(requestId, ArenaLoadFailure::UnsupportedConfig);
        return;
    }
    const auto expected = normalizedSha256(request.sha256);
    if (!expected) {
        emit loadFailed(requestId, ArenaLoadFailure::HashMismatch);
        return;
    }
    std::optional<QString> path;
    try {
        path = m_pathResolver ? m_pathResolver(*expected) : std::nullopt;
    } catch (...) {
        emit loadFailed(requestId, ArenaLoadFailure::ResourceFailed);
        return;
    }
    if (!path) {
        emit loadFailed(requestId, ArenaLoadFailure::MissingFile);
        return;
    }
    startFileCheck(requestId, OperationKind::Load, *expected, *path, request);
}

void
QtArenaRoundLoader::cancel(quint64 requestId)
{
    const auto found = m_operations.find(requestId);
    if (found == m_operations.end()) {
        return;
    }
    const auto kind = found->kind;
    const auto completionReported = found->completionReported;
    found->cancelled->store(true, std::memory_order_relaxed);
    if (found->runner) {
        found->runner->deleteLater();
    }
    m_operations.erase(found);
    if (completionReported) {
        return;
    }
    if (kind == OperationKind::Probe) {
        emit probeFinished(
          requestId,
          ArenaProbeResult{ .failure = ArenaProbeFailure::Cancelled });
    } else {
        emit loadFailed(requestId, ArenaLoadFailure::Cancelled);
    }
}

void
QtArenaRoundLoader::discard(quint64 requestId)
{
    const auto found = m_operations.find(requestId);
    if (found == m_operations.end()) {
        return;
    }
    found->cancelled->store(true, std::memory_order_relaxed);
    if (found->runner) {
        found->runner->deleteLater();
    }
    m_operations.erase(found);
}

void
QtArenaRoundLoader::startFileCheck(
  quint64 requestId,
  OperationKind kind,
  const QByteArray& expectedSha256,
  QString path,
  std::optional<ArenaRoundLoadRequest> loadRequest)
{
    const auto serial = m_nextSerial++;
    auto cancelled = std::make_shared<std::atomic_bool>(false);
    auto* watcher = new QFutureWatcher<FileCheckResult>(this);
    m_operations.insert(requestId,
                        Operation{ .kind = kind,
                                   .serial = serial,
                                   .cancelled = cancelled,
                                   .watcher = watcher,
                                   .path = path,
                                   .loadRequest = std::move(loadRequest) });
    connect(watcher,
            &QFutureWatcher<FileCheckResult>::finished,
            this,
            [this, requestId, serial, watcher] {
                finishFileCheck(requestId, serial);
                watcher->deleteLater();
            });
    watcher->setFuture(
      QtConcurrent::run([path = std::move(path), expectedSha256, cancelled] {
          if (cancelled->load(std::memory_order_relaxed)) {
              return FileCheckResult{
                  .failure = FileCheckFailure::Cancelled,
              };
          }
          if (!QFileInfo::exists(path)) {
              return FileCheckResult{
                  .failure = FileCheckFailure::MissingFile,
              };
          }
          QFile file(path);
          if (!file.open(QIODevice::ReadOnly)) {
              return FileCheckResult{
                  .failure = FileCheckFailure::ReadFailed,
              };
          }
          QCryptographicHash hasher(QCryptographicHash::Sha256);
          constexpr auto chunkSize = 1024 * 1024;
          while (!file.atEnd()) {
              if (cancelled->load(std::memory_order_relaxed)) {
                  return FileCheckResult{
                      .failure = FileCheckFailure::Cancelled,
                  };
              }
              const auto chunk = file.read(chunkSize);
              if (chunk.isNull() && file.error() != QFile::NoError) {
                  return FileCheckResult{
                      .failure = FileCheckFailure::ReadFailed,
                  };
              }
              hasher.addData(chunk);
          }
          auto observed = hasher.result();
          return FileCheckResult{
              .failure = observed == expectedSha256
                           ? FileCheckFailure::None
                           : FileCheckFailure::HashMismatch,
              .observedSha256 = std::move(observed),
          };
      }));
}

void
QtArenaRoundLoader::finishFileCheck(quint64 requestId, quint64 serial)
{
    const auto found = m_operations.find(requestId);
    if (found == m_operations.end() || found->serial != serial ||
        found->watcher == nullptr) {
        return;
    }
    const auto result = found->watcher->result();
    found->watcher = nullptr;
    if (found->kind == OperationKind::Probe) {
        ArenaProbeFailure failure = ArenaProbeFailure::None;
        switch (result.failure) {
            case FileCheckFailure::None:
                break;
            case FileCheckFailure::MissingFile:
                failure = ArenaProbeFailure::MissingFile;
                break;
            case FileCheckFailure::HashMismatch:
                failure = ArenaProbeFailure::HashMismatch;
                break;
            case FileCheckFailure::ReadFailed:
                failure = ArenaProbeFailure::ReadFailed;
                break;
            case FileCheckFailure::Cancelled:
                failure = ArenaProbeFailure::Cancelled;
                break;
        }
        m_operations.erase(found);
        emit probeFinished(
          requestId,
          ArenaProbeResult{ .failure = failure,
                            .observedSha256 = result.observedSha256 });
        return;
    }
    switch (result.failure) {
        case FileCheckFailure::None:
            prepareRunner(requestId, serial);
            return;
        case FileCheckFailure::MissingFile:
            failLoad(requestId, serial, ArenaLoadFailure::MissingFile);
            return;
        case FileCheckFailure::HashMismatch:
            failLoad(requestId, serial, ArenaLoadFailure::HashMismatch);
            return;
        case FileCheckFailure::ReadFailed:
            failLoad(requestId, serial, ArenaLoadFailure::ResourceFailed);
            return;
        case FileCheckFailure::Cancelled:
            failLoad(requestId, serial, ArenaLoadFailure::Cancelled);
            return;
    }
}

void
QtArenaRoundLoader::prepareRunner(quint64 requestId, quint64 serial)
{
    auto found = m_operations.find(requestId);
    if (found == m_operations.end() || found->serial != serial ||
        !found->loadRequest) {
        return;
    }
    const auto path = found->path;
    const auto playConfig = found->loadRequest->playConfig;
    gameplay_logic::ChartRunner* runner = nullptr;
    try {
        if (m_runnerLoader) {
            runner = m_runnerLoader(path, playConfig);
        }
    } catch (...) {
        runner = nullptr;
    }
    if (runner == nullptr) {
        failLoad(requestId, serial, ArenaLoadFailure::ParseFailed);
        return;
    }
    runner->setParent(this);
    runner->holdStart();
    found = m_operations.find(requestId);
    if (found == m_operations.end() || found->serial != serial) {
        runner->deleteLater();
        return;
    }
    found->runner = runner;
    connect(runner, &QObject::destroyed, this, [this, requestId, serial] {
        const auto current = m_operations.find(requestId);
        if (current == m_operations.end() || current->serial != serial) {
            return;
        }
        const auto reported = current->completionReported;
        m_operations.erase(current);
        if (!reported) {
            emit loadFailed(requestId, ArenaLoadFailure::ResourceFailed);
        }
    });
    connect(runner,
            &gameplay_logic::ChartRunner::statusChanged,
            this,
            [this, requestId, serial] { observeRunner(requestId, serial); });
    observeRunner(requestId, serial);
}

void
QtArenaRoundLoader::observeRunner(quint64 requestId, quint64 serial)
{
    const auto found = m_operations.find(requestId);
    if (found == m_operations.end() || found->serial != serial ||
        found->runner == nullptr || found->completionReported) {
        return;
    }
    if (found->runner->getStatus() == gameplay_logic::ChartRunner::Ready) {
        found->completionReported = true;
        emit loadFinished(requestId, found->runner);
    } else if (found->runner->getStatus() ==
               gameplay_logic::ChartRunner::Finished) {
        failLoad(requestId, serial, ArenaLoadFailure::ResourceFailed);
    }
}

void
QtArenaRoundLoader::failLoad(quint64 requestId,
                             quint64 serial,
                             ArenaLoadFailure failure)
{
    const auto found = m_operations.find(requestId);
    if (found == m_operations.end() || found->serial != serial) {
        return;
    }
    if (found->runner) {
        found->runner->deleteLater();
    }
    m_operations.erase(found);
    emit loadFailed(requestId, failure);
}

} // namespace arena

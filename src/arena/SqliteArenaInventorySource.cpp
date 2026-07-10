#include "SqliteArenaInventorySource.h"

#include "ArenaBinaryProtocol.h"

#include <SQLiteCpp/SQLiteCpp.h>

#include <QtConcurrentRun>

#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <utility>

namespace arena {
namespace {

auto
hexNibble(char value) -> int
{
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    return -1;
}

auto
decodeSha256(const std::string& value) -> std::optional<std::array<char, 32>>
{
    if (value.size() != ArenaSha256Bytes * 2) {
        return std::nullopt;
    }
    std::array<char, 32> decoded{};
    for (size_t i = 0; i < decoded.size(); ++i) {
        const auto high = hexNibble(value[i * 2]);
        const auto low = hexNibble(value[i * 2 + 1]);
        if (high < 0 || low < 0) {
            return std::nullopt;
        }
        decoded[i] = static_cast<char>((high << 4) | low);
    }
    return decoded;
}

} // namespace

SqliteArenaInventorySource::SqliteArenaInventorySource(
  std::filesystem::path songDbPath,
  QObject* parent)
  : ArenaInventorySource(parent)
  , m_songDbPath(std::move(songDbPath))
{
    connect(&m_watcher,
            &QFutureWatcher<BuildResult>::finished,
            this,
            &SqliteArenaInventorySource::handleBuildFinished);
}

SqliteArenaInventorySource::~SqliteArenaInventorySource()
{
    disconnect(&m_watcher, nullptr, this, nullptr);
    if (m_watcher.isRunning()) {
        m_watcher.waitForFinished();
    }
}

auto
SqliteArenaInventorySource::generation() const -> qint64
{
    return m_generation;
}

void
SqliteArenaInventorySource::requestSnapshot(quint64 requestId)
{
    auto request =
      BuildRequest{ .requestId = requestId, .generation = m_generation };
    if (!m_active) {
        startBuild(request);
        return;
    }
    m_active->superseded = true;
    setPending(request);
}

void
SqliteArenaInventorySource::cancel(quint64 requestId)
{
    if (m_active && m_active->requestId == requestId) {
        m_active->cancelled = true;
    }
    if (m_pending && m_pending->requestId == requestId) {
        m_pending.reset();
    }
}

void
SqliteArenaInventorySource::commitLibraryMutation()
{
    ++m_generation;
    if (m_active && !m_active->cancelled) {
        m_active->superseded = true;
        setPending(BuildRequest{ .requestId = m_active->requestId,
                                 .generation = m_generation });
    }
    emit generationChanged(m_generation);
}

void
SqliteArenaInventorySource::setPending(BuildRequest request)
{
    m_pending = request;
}

void
SqliteArenaInventorySource::startBuild(BuildRequest request)
{
    m_active = request;
    const auto path = m_songDbPath;
    const auto capturedGeneration = request.generation;
    m_watcher.setFuture(QtConcurrent::run([path, capturedGeneration] {
        return buildSnapshot(path, capturedGeneration);
    }));
}

void
SqliteArenaInventorySource::handleBuildFinished()
{
    if (!m_active) {
        return;
    }
    const auto request = *m_active;
    const auto result = m_watcher.result();
    m_active.reset();

    if (!request.cancelled && !request.superseded &&
        result.generation == m_generation) {
        if (result.failure) {
            emit snapshotFailed(request.requestId, *result.failure);
        } else {
            emit snapshotReady(
              request.requestId,
              ArenaInventorySnapshot{ .libraryGeneration = result.generation,
                                      .packedSha256 = result.packedSha256 });
        }
    } else if (!request.cancelled && !m_pending &&
               result.generation != m_generation) {
        setPending(BuildRequest{ .requestId = request.requestId,
                                 .generation = m_generation });
    }

    if (m_pending) {
        const auto next = *m_pending;
        m_pending.reset();
        startBuild(next);
    }
}

auto
SqliteArenaInventorySource::buildSnapshot(
  const std::filesystem::path& songDbPath,
  qint64 generation) -> BuildResult
{
    auto result = BuildResult{ .generation = generation };
    try {
        SQLite::Database connection(songDbPath,
                                    SQLite::OPEN_READONLY |
                                      SQLite::OPEN_FULLMUTEX); // NOLINT
        SQLite::Statement query(connection,
                                "SELECT DISTINCT lower(sha256) "
                                "FROM charts "
                                "WHERE length(sha256) = 64 "
                                "ORDER BY lower(sha256) COLLATE BINARY "
                                "LIMIT 250001");
        result.packedSha256.reserve(ArenaMaxInventoryBytes);
        std::array<char, 32> previous{};
        bool hasPrevious = false;
        qsizetype count = 0;
        while (query.executeStep()) {
            if (count == ArenaMaxInventoryHashes) {
                result.packedSha256.clear();
                result.failure = ArenaInventoryFailure::TooManyCharts;
                return result;
            }
            const auto decoded = decodeSha256(query.getColumn(0).getString());
            if (!decoded ||
                (hasPrevious && std::memcmp(previous.data(),
                                            decoded->data(),
                                            ArenaSha256Bytes) >= 0)) {
                result.packedSha256.clear();
                result.failure = ArenaInventoryFailure::InvalidHash;
                return result;
            }
            result.packedSha256.append(decoded->data(), ArenaSha256Bytes);
            previous = *decoded;
            hasPrevious = true;
            ++count;
        }
    } catch (const std::exception&) {
        result.packedSha256.clear();
        result.failure = ArenaInventoryFailure::DatabaseError;
    }
    return result;
}

} // namespace arena

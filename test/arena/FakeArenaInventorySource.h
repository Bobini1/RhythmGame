#pragma once

#include "arena/ArenaInventorySource.h"

#include <QVector>

#include <utility>

namespace arena::test {

class FakeArenaInventorySource final : public ArenaInventorySource
{
  public:
    using ArenaInventorySource::ArenaInventorySource;

    qint64 currentGeneration{ 1 };
    QVector<quint64> requests{};
    QVector<quint64> cancellations{};

    [[nodiscard]] auto generation() const -> qint64 override
    {
        return currentGeneration;
    }
    void requestSnapshot(quint64 requestId) override
    {
        requests.push_back(requestId);
    }
    void cancel(quint64 requestId) override
    {
        cancellations.push_back(requestId);
    }

    void advanceGeneration()
    {
        ++currentGeneration;
        emit generationChanged(currentGeneration);
    }
    void succeed(quint64 requestId, QByteArray packedSha256)
    {
        emit snapshotReady(requestId,
                           ArenaInventorySnapshot{
                             .libraryGeneration = currentGeneration,
                             .packedSha256 = std::move(packedSha256),
                           });
    }
    void fail(quint64 requestId, ArenaInventoryFailure failure)
    {
        emit snapshotFailed(requestId, failure);
    }
};

} // namespace arena::test

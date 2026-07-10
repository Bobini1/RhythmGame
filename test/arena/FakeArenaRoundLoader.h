#ifndef RHYTHMGAME_FAKEARENAROUNDLOADER_H
#define RHYTHMGAME_FAKEARENAROUNDLOADER_H

#include "arena/ArenaRoundLoader.h"

namespace arena::test {

class FakeArenaRoundLoader final : public ArenaRoundLoader
{
  public:
    std::optional<ArenaSelectionBuildResult> nextSelection;
    QVector<std::pair<quint64, QByteArray>> probes;
    QVector<std::pair<quint64, ArenaRoundLoadRequest>> loads;
    QVector<quint64> cancellations;

    using ArenaRoundLoader::ArenaRoundLoader;

    auto buildSelection(gameplay_logic::ChartData*)
      -> ArenaSelectionBuildResult override
    {
        if (!nextSelection) {
            return std::unexpected(ArenaSelectionBuildFailure::InvalidChart);
        }
        return *nextSelection;
    }

    void probe(quint64 requestId, const QByteArray& sha256) override
    {
        probes.emplace_back(requestId, sha256);
    }

    void load(quint64 requestId, const ArenaRoundLoadRequest& request) override
    {
        loads.emplace_back(requestId, request);
    }

    void cancel(quint64 requestId) override
    {
        cancellations.push_back(requestId);
    }
};

} // namespace arena::test

#endif // RHYTHMGAME_FAKEARENAROUNDLOADER_H

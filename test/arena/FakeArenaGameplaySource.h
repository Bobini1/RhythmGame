#pragma once

#include "arena/ArenaGameplaySource.h"
#include "gameplay_logic/BmsScore.h"
#include "gameplay_logic/ChartRunner.h"

#include <QPointer>
#include <QVector>

#include <functional>

namespace arena::test {

class FakeArenaGameplaySource final : public ArenaGameplaySource
{
  public:
    using ArenaGameplaySource::ArenaGameplaySource;

    std::expected<QString, ArenaGameplayCaptureFailure> attachResult{
        QStringLiteral("arena-score-guid")
    };
    std::function<std::expected<TelemetrySnapshot, ArenaGameplayCaptureFailure>(
      quint32)>
      sampleHandler;
    std::function<std::expected<FinalResult, ArenaGameplayCaptureFailure>(
      gameplay_logic::BmsScore*)>
      finalHandler;
    QPointer<gameplay_logic::ChartRunner> attachedRunner;
    mutable QVector<quint32> sampledSequences;
    mutable QVector<QPointer<gameplay_logic::BmsScore>> capturedScores;
    int attachCount{};
    int detachCount{};

    auto attach(gameplay_logic::ChartRunner* runner)
      -> std::expected<QString, ArenaGameplayCaptureFailure> override
    {
        ++attachCount;
        attachedRunner = runner;
        return attachResult;
    }

    void detach() override
    {
        ++detachCount;
        attachedRunner.clear();
    }

    [[nodiscard]] auto sample(quint32 sequence) const
      -> std::expected<TelemetrySnapshot, ArenaGameplayCaptureFailure> override
    {
        sampledSequences.append(sequence);
        if (sampleHandler) {
            return sampleHandler(sequence);
        }
        return std::unexpected(ArenaGameplayCaptureFailure::NoRunner);
    }

    [[nodiscard]] auto captureFinal(gameplay_logic::BmsScore* score) const
      -> std::expected<FinalResult, ArenaGameplayCaptureFailure> override
    {
        capturedScores.append(score);
        if (finalHandler) {
            return finalHandler(score);
        }
        return std::unexpected(ArenaGameplayCaptureFailure::InvalidResult);
    }
};

} // namespace arena::test

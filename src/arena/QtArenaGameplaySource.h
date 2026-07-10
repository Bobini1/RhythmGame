#pragma once

#include "ArenaGameplaySource.h"

#include <QPointer>

namespace arena {

class QtArenaGameplaySource final : public ArenaGameplaySource
{
    Q_OBJECT

  public:
    using ArenaGameplaySource::ArenaGameplaySource;

    auto attach(gameplay_logic::ChartRunner* runner)
      -> std::expected<QString, ArenaGameplayCaptureFailure> override;
    void detach() override;
    [[nodiscard]] auto sample(quint32 sequence) const
      -> std::expected<TelemetrySnapshot, ArenaGameplayCaptureFailure> override;
    [[nodiscard]] auto captureFinal(gameplay_logic::BmsScore* score) const
      -> std::expected<FinalResult, ArenaGameplayCaptureFailure> override;

  private:
    QPointer<gameplay_logic::ChartRunner> m_runner;
    QString m_expectedScoreGuid;
};

} // namespace arena

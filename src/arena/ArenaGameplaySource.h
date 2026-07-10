#pragma once

#include "ArenaTypes.h"

#include <QObject>
#include <QString>

#include <expected>

namespace gameplay_logic {
class BmsScore;
class ChartRunner;
}

namespace arena {

enum class ArenaGameplayCaptureFailure
{
    NoRunner,
    WrongScore,
    InvalidNumber,
    UnsupportedGauge,
    InvalidResult,
};

class ArenaGameplaySource : public QObject
{
    Q_OBJECT

  public:
    using QObject::QObject;
    ~ArenaGameplaySource() override = default;

    virtual auto attach(gameplay_logic::ChartRunner* runner)
      -> std::expected<QString, ArenaGameplayCaptureFailure> = 0;
    virtual void detach() = 0;
    [[nodiscard]] virtual auto sample(quint32 sequence) const
      -> std::expected<TelemetrySnapshot, ArenaGameplayCaptureFailure> = 0;
    [[nodiscard]] virtual auto captureFinal(gameplay_logic::BmsScore* score)
      const -> std::expected<FinalResult, ArenaGameplayCaptureFailure> = 0;
};

} // namespace arena

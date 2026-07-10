#ifndef RHYTHMGAME_CHARTPLAYCONFIG_H
#define RHYTHMGAME_CHARTPLAYCONFIG_H

#include "Vars.h"

#include <QList>

#include <cstdint>
#include <utility>

namespace resource_managers {

inline constexpr auto chartRandomizationVersion = 1;

/** Values that must be identical for every participant in a synchronized run.
 */
struct ChartPlayConfig
{
    QList<qint64> randomSequence;
    NoteOrderAlgorithm noteOrderP1{ NoteOrderAlgorithm::Normal };
    NoteOrderAlgorithm noteOrderP2{ NoteOrderAlgorithm::Normal };
    DpOptions dpMode{ DpOptions::Off };
    std::uint64_t laneSeed{};
    int randomizationVersion{ chartRandomizationVersion };

    [[nodiscard]] auto isSupported() const -> bool
    {
        return randomizationVersion == chartRandomizationVersion;
    }

    bool operator==(const ChartPlayConfig&) const = default;
};

/**
 * Strict cursor used while parsing #RANDOM. The parse is accepted only when
 * every supplied value was consumed and every directive had a supplied value
 * in its declared range.
 */
class ExactRandomSequence
{
    QList<qint64> values;
    qsizetype cursor{};
    bool valid{ true };

  public:
    explicit ExactRandomSequence(QList<qint64> values)
      : values(std::move(values))
    {
    }

    [[nodiscard]] auto next(qint64 upperBound) -> qint64
    {
        if (cursor >= values.size()) {
            valid = false;
            return 1;
        }
        const auto value = values[cursor++];
        if (value < 1 || value > upperBound) {
            valid = false;
            return 1;
        }
        return value;
    }

    [[nodiscard]] auto complete() const -> bool
    {
        return valid && cursor == values.size();
    }
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTPLAYCONFIG_H

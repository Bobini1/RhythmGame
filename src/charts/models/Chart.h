//
// Created by bobini on 09.07.2022.
//

#ifndef RHYTHMGAME_CHART_H
#define RHYTHMGAME_CHART_H

#include <optional>
#include <string>
#include "charts/behaviour/SongDataWriter.h"

namespace charts::models {

/**
 * @brief Base class for all charts.
 *
 * This class is used to represent a chart of any format. A class supports a set
 * of common operations related to obtaining metadata, saving, etc.
 */
class Chart
{
  public:
    virtual ~Chart() = default;

    /**
     * @brief Writes full chart data to lua.
     * @param writer Writer to write to.
     */
    virtual auto writeFullData(behaviour::SongDataWriter writer) const
      -> void = 0;

    // virtual auto saveToDb() const -> void = 0;
};
} // namespace charts::models

#endif // RHYTHMGAME_CHART_H

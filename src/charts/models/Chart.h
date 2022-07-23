//
// Created by bobini on 09.07.2022.
//

#ifndef RHYTHMGAME_CHART_H
#define RHYTHMGAME_CHART_H

#include <optional>
#include <string>
#include "charts/behaviour/SongDataWriter.h"

namespace charts::models {
class Chart
{
  public:
    virtual ~Chart() = default;
    virtual auto writeFullData(behaviour::SongDataWriter writer) const
      -> void = 0;
    // virtual auto saveToDb() const -> void = 0;
};
} // namespace charts::models

#endif // RHYTHMGAME_CHART_H

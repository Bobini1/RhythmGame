//
// Created by bobini on 09.07.2022.
//

#ifndef RHYTHMGAME_CHART_H
#define RHYTHMGAME_CHART_H

#include <optional>
#include <string>
#include "charts/models/BmsMeta.h"

namespace charts::models {
class Chart
{
  public:
    Chart(std::string title, std::string artist, double bpm);
    [[nodiscard]] auto getTitle() const -> const std::string&;
    [[nodiscard]] auto getArtist() const -> const std::string&;
    [[nodiscard]] auto getBpm() const -> double;

  private:
    std::string title;
    std::string artist;
    double bpm;
};
} // namespace charts::models

#endif // RHYTHMGAME_CHART_H

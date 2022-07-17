//
// Created by bobini on 09.07.2022.
//

#ifndef RHYTHMGAME_CHART_H
#define RHYTHMGAME_CHART_H

#include <optional>
#include "ChartInfo.h"

namespace charts::models {
class Chart
{
  public:
    [[nodiscard]] auto getTitle() const -> const std::string&;
    [[nodiscard]] auto getArtist() const -> const std::string&;
    [[nodiscard]] auto getBpm() const -> const std::string&;

  private:
    std::string title;
    std::string artist;
    std::string bpm;
    std::variant<BMSMeta, SMMeta> meta;

  public:
};
} // namespace charts::models

#endif // RHYTHMGAME_CHART_H

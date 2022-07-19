//
// Created by bobini on 09.07.2022.
//

#ifndef RHYTHMGAME_CHART_H
#define RHYTHMGAME_CHART_H

#include <optional>
#include <variant>
#include <string>
#include "charts/models/BmsMeta.h"

namespace charts::chart_readers {
template<typename>
struct action;
} // namespace charts::chart_readers

namespace charts::models {
class Chart
{
  public:
    Chart(std::string title,
          std::string artist,
          double bpm,
          std::variant<BmsMeta> meta);
    [[nodiscard]] auto getTitle() const -> const std::string&;
    [[nodiscard]] auto getArtist() const -> const std::string&;
    [[nodiscard]] auto getBpm() const -> double;
    [[nodiscard]] auto getMeta() const -> const std::variant<BmsMeta>&;

    template<typename>
    friend struct charts::chart_readers::action;

  private:
    std::string title;
    std::string artist;
    double bpm;
    std::variant<BmsMeta> meta;
};
} // namespace charts::models

#endif // RHYTHMGAME_CHART_H

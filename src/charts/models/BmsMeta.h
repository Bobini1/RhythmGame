//
// Created by bobini on 17.07.2022.
//

#ifndef RHYTHMGAME_BMSMETA_H
#define RHYTHMGAME_BMSMETA_H

#include <string>
#include <optional>

namespace charts::chart_readers {
template<typename>
struct action;
} // namespace charts::chart_readers

class BmsMeta
{
  public:
    BmsMeta(std::optional<std::string> genre,
            std::optional<std::string> subtitle,
            std::optional<std::string> subartist);
    [[nodiscard]] auto getGenre() const -> const std::optional<std::string>&;
    [[nodiscard]] auto getSubtitle() const -> const std::optional<std::string>&;
    [[nodiscard]] auto getSubartist() const
      -> const std::optional<std::string>&;

    template<typename>
    friend struct charts::chart_readers::action;

  private:
    std::optional<std::string> genre;
    std::optional<std::string> subtitle;
    std::optional<std::string> subartist;
};

#endif // RHYTHMGAME_BMSMETA_H

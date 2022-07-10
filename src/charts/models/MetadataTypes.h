//
// Created by bobini on 09.07.2022.
//

#ifndef RHYTHMGAME_METADATATYPES_H
#define RHYTHMGAME_METADATATYPES_H

#include <chrono>
#include <type_safe/strong_typedef.hpp>
#include <type_safe/types.hpp>
namespace charts::models {
using Offset = std::chrono::milliseconds;
using Bpm = type_safe::floating_point<double>;
using Level =
  std::variant<type_safe::integer<int>, type_safe::floating_point<double>>;
using NoteCount = type_safe::integer<unsigned int>;
using Title = type_safe::strong_typedef<struct TitleTag, std::string>;
using Artist = type_safe::strong_typedef<struct ArtistTag, std::string>;
using Genre = type_safe::strong_typedef<struct GenreTag, std::string>;
using ChartType = type_safe::strong_typedef<struct ChartTypeTag, std::string>;
using Difficulty = type_safe::strong_typedef<struct DifficultyTag, std::string>;
} // namespace charts::models

#endif // RHYTHMGAME_METADATATYPES_H

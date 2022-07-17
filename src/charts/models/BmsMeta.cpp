//
// Created by bobini on 17.07.2022.
//

#include "BmsMeta.h"

#include <utility>
BmsMeta::BmsMeta(std::optional<std::string> genre,
                 std::optional<std::string> subtitle,
                 std::optional<std::string> subartist)
  : genre(std::move(genre))
  , subtitle(std::move(subtitle))
  , subartist(std::move(subartist))
{
}
auto
BmsMeta::getGenre() const -> const std::optional<std::string>&
{
    return genre;
}
auto
BmsMeta::getSubtitle() const -> const std::optional<std::string>&
{
    return subtitle;
}
auto
BmsMeta::getSubartist() const -> const std::optional<std::string>&
{
    return subartist;
}

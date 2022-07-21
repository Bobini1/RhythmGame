//
// Created by bobini on 09.07.2022.
//

#include "Chart.h"

#include <utility>
auto
charts::models::Chart::getTitle() const -> const std::string&
{
    return title;
}
auto
charts::models::Chart::getArtist() const -> const std::string&
{
    return artist;
}
auto
charts::models::Chart::getBpm() const -> double
{
    return bpm;
}
charts::models::Chart::Chart(std::string title, std::string artist, double bpm)
  : title(std::move(title))
  , artist(std::move(artist))
  , bpm(std::move(bpm))
{
}
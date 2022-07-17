//
// Created by bobini on 09.07.2022.
//

#include "Chart.h"
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
charts::models::Chart::getBpm() const -> const std::string&
{
    return bpm;
}

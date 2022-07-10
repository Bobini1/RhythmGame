//
// Created by bobini on 09.07.2022.
//

#ifndef RHYTHMGAME_CHARTINFO_H
#define RHYTHMGAME_CHARTINFO_H

#include <string>
#include <boost/icl/interval_map.hpp>
#include <variant>

#include "MetadataTypes.h"
namespace charts::models {
struct ChartInfo
{
    Title title;
    Artist artist;
    Genre genre;
    ChartType chartType;
    Difficulty difficulty;
    Level level;
    NoteCount noteCount;
    boost::icl::interval_map<Offset, Bpm> bpms;
    Offset length;
};
} // namespace charts::models

#endif // RHYTHMGAME_CHARTINFO_H

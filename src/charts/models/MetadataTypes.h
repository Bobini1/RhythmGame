//
// Created by bobini on 09.07.2022.
//

#ifndef RHYTHMGAME_BPMS_H
#define RHYTHMGAME_BPMS_H

#include <boost/icl/interval_map.hpp>
#include <chrono>

using Bpms = boost::icl::interval_map<std::chrono::duration<float, std::milli>, double>;

#endif // RHYTHMGAME_BPMS_H

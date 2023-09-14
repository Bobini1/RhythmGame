//
// Created by bobini on 10.09.23.
//

#ifndef RHYTHMGAME_TIMINGWINDOWS_H
#define RHYTHMGAME_TIMINGWINDOWS_H
#include <chrono>
#include <boost/icl/split_interval_map.hpp>
#include "gameplay_logic/Judgement.h"
namespace gameplay_logic::rules {
using TimingWindows =
  boost::icl::split_interval_map<std::chrono::nanoseconds, Judgement>;
} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_TIMINGWINDOWS_H

//
// Created by bobini on 10.09.23.
//

#include "Lr2HitValues.h"
auto
gameplay_logic::rules::lr2_hit_values::getLr2HitValue(
  const gameplay_logic::rules::TimingWindows& timingWindows,
  std::chrono::nanoseconds offset) -> double
{
    auto iter = timingWindows.find(offset);
    if (iter == timingWindows.end()) {
        return 0.0;
    }
    switch (iter->second) {
        case Judgement::Perfect:
            return 2;
        case Judgement::Great:
            return 1;
        default:
            return 0.0;
    }
}

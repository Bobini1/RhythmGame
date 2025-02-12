//
// Created by bobini on 10.09.23.
//

#include "Lr2HitValues.h"
auto
gameplay_logic::rules::lr2_hit_values::getLr2HitValue(
  std::chrono::nanoseconds offset,
  const Judgement judgement) -> double
{
    switch (judgement) {
        case Judgement::Perfect:
            return 2;
        case Judgement::Great:
            return 1;
        default:
            return 0.0;
    }
}

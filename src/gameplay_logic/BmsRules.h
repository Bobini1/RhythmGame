//
// Created by bobini on 22.06.23.
//

#ifndef RHYTHMGAME_BMSRULES_H
#define RHYTHMGAME_BMSRULES_H
#include <iterator>
#include <optional>
#include <chrono>
#include "BmsPoints.h"
#include "TimePoint.h"

namespace gameplay_logic {
class BmsRules
{
  public:
    auto judgeHit(gameplay_logic::TimePoint noteTime,
                  gameplay_logic::TimePoint hitTime) -> std::optional<BmsPoints>
    {
        using namespace std::chrono_literals;
        if (hitTime < noteTime - 135ms) {
            return std::nullopt;
        }
        if (hitTime > noteTime + 135ms) {
            return std::nullopt;
        }
        return BmsPoints{ 1.0 };
    }
    template<std::ranges::input_range Range>
    auto getMisses(Range&& notes, gameplay_logic::TimePoint time)
      -> std::vector<gameplay_logic::TimePoint>
        requires std::same_as<std::ranges::range_value_t<Range>,
                              gameplay_logic::TimePoint>
    {
        using namespace std::chrono_literals;
        auto misses = std::vector<gameplay_logic::TimePoint>{};
        for (auto&& noteTime : notes) {
            if (noteTime < time - 135ms) {
                misses.push_back(noteTime - 135ms);
            } else {
                break;
            }
        }
        return misses;
    }
    auto invisibleNoteHit(gameplay_logic::TimePoint noteTime,
                          gameplay_logic::TimePoint hitTime) -> bool
    {
        using namespace std::chrono_literals;
        if (hitTime < noteTime - 135ms) {
            return false;
        }
        if (hitTime > noteTime + 135ms) {
            return false;
        }
        return true;
    }
    template<std::ranges::input_range Range>
    auto countInvisibleSkipped(Range&& range, gameplay_logic::TimePoint time)
      -> int
        requires std::same_as<std::ranges::range_value_t<Range>,
                              gameplay_logic::TimePoint>
    {
        using namespace std::chrono_literals;
        auto count = 0;
        for (auto&& noteTime : range) {
            if (noteTime < time - 135ms) {
                count++;
            } else {
                break;
            }
        }
        return count;
    }
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSRULES_H

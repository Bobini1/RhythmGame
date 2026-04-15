//
// Created by PC on 12/04/2026.
//

#include "TimingWindowsFromHash.h"

#include <magic_enum/magic_enum.hpp>

namespace support {

auto
timingWindowsFromHash(
  const QHash<gameplay_logic::Judgement, QPair<qint64, qint64>>& hash)
  -> gameplay_logic::rules::TimingWindows
{
    using Nanos = boost::icl::interval<std::chrono::nanoseconds>;
    auto map = gameplay_logic::rules::TimingWindows{};
    for (auto j = static_cast<int>(gameplay_logic::Judgement::EmptyPoor);
         j <= static_cast<int>(gameplay_logic::Judgement::Perfect);
         j++) {
        const auto judgement = static_cast<gameplay_logic::Judgement>(j);
        const auto p = hash.value(judgement);
        const auto early = p.first;
        const auto late = p.second;
        map.set({ Nanos::closed(std::chrono::nanoseconds(early),
                                std::chrono::nanoseconds(late)),
                  judgement });
    }
    return map;
}
} // namespace support
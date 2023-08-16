//
// Created by bobini on 22.06.23.
//

#include "BmsRules.h"
auto
gameplay_logic::BmsRules::visibleNoteHit(std::span<NoteType>& notes,
                                         gameplay_logic::TimePoint hitTime,
                                         gameplay_logic::TimePoint chartStart)
  -> std::optional<std::pair<BmsPoints, std::span<NoteType>::iterator>>
{
    using namespace std::chrono_literals;
    for (auto iter = notes.begin(); iter != notes.end(); iter++) {
        auto& [sound, noteTime, hit] = *iter;
        if (hit) {
            continue;
        }
        if (hitTime < chartStart + noteTime - 135ms) {
            continue;
        }
        if (hitTime > chartStart + noteTime + 135ms) {
            return std::nullopt;
        }
        hit = true;
        if (sound != nullptr) {
            sound->play();
        }
        return { { BmsPoints{
                     1.0, Judgement::PERFECT, chartStart + noteTime - hitTime },
                   iter } };
    }
    return std::nullopt;
}
auto
gameplay_logic::BmsRules::getMisses(std::span<NoteType> notes,
                                    gameplay_logic::TimePoint time,
                                    gameplay_logic::TimePoint chartStart)
  -> std::vector<gameplay_logic::TimePoint>
{
    using namespace std::chrono_literals;
    auto misses = std::vector<gameplay_logic::TimePoint>{};
    for (auto& [sound, noteTime, hit] : notes) {
        if (hit) {
            continue;
        }
        auto timePoint = chartStart + noteTime;
        if (timePoint < time - 135ms) {
            misses.push_back(timePoint - 135ms);
        } else {
            hit = true;
            break;
        }
    }
    return misses;
}
void
gameplay_logic::BmsRules::invisibleNoteHit(std::span<NoteType>& notes,
                                           gameplay_logic::TimePoint hitTime,
                                           gameplay_logic::TimePoint chartStart)
{
    using namespace std::chrono_literals;
    for (auto& [sound, noteTime, hit] : notes) {
        if (hit) {
            continue;
        }
        if (hitTime < chartStart + noteTime - 135ms) {
            continue;
        }
        if (hitTime > chartStart + noteTime + 135ms) {
            return;
        }
        hit = true;
        if (sound != nullptr) {
            sound->play();
        }
        return;
    }
}
auto
gameplay_logic::BmsRules::skipInvisible(std::span<NoteType> notes,
                                        gameplay_logic::TimePoint time,
                                        gameplay_logic::TimePoint chartStart)
  -> int
{
    using namespace std::chrono_literals;
    auto count = 0;
    for (auto& [sound, noteTime, hit] : notes) {
        if (hit) {
            continue;
        }
        if (chartStart + noteTime < time - 135ms) {
            count++;
        } else {
            break;
        }
    }
    return count;
}

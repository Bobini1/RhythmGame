//
// Created by bobini on 22.06.23.
//

#include "BmsRules.h"
auto
gameplay_logic::BmsRules::visibleNoteHit(std::span<NoteType>& notes,
                                         std::chrono::nanoseconds hitOffset)
  -> std::optional<std::pair<BmsPoints, std::span<NoteType>::iterator>>
{
    using namespace std::chrono_literals;
    for (auto iter = notes.begin(); iter != notes.end(); iter++) {
        auto& [sound, noteTime, hit] = *iter;
        if (hit) {
            continue;
        }
        if (hitOffset < noteTime - 135ms) {
            continue;
        }
        if (hitOffset > noteTime + 135ms) {
            return std::nullopt;
        }
        hit = true;
        if (sound != nullptr) {
            sound->play();
        }
        return { { BmsPoints(
                     1.0, Judgement::PERFECT, (hitOffset - noteTime).count()),
                   iter } };
    }
    return std::nullopt;
}
auto
gameplay_logic::BmsRules::getMisses(std::span<NoteType> notes,
                                    std::chrono::nanoseconds offsetFromStart)
  -> std::vector<std::chrono::nanoseconds>
{
    using namespace std::chrono_literals;
    auto misses = std::vector<std::chrono::nanoseconds>{};
    for (auto& [sound, noteTime, hit] : notes) {
        if (hit) {
            continue;
        }
        if (offsetFromStart > noteTime + 135ms) {
            misses.push_back(noteTime + 135ms);
        } else {
            hit = true;
            break;
        }
    }
    return misses;
}
void
gameplay_logic::BmsRules::invisibleNoteHit(std::span<NoteType>& notes,
                                           std::chrono::nanoseconds hitOffset)
{
    using namespace std::chrono_literals;
    for (auto& [sound, noteTime, hit] : notes) {
        if (hit) {
            continue;
        }
        if (hitOffset < noteTime - 135ms) {
            continue;
        }
        if (hitOffset > noteTime + 135ms) {
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
gameplay_logic::BmsRules::skipInvisible(
  std::span<NoteType> notes,
  std::chrono::nanoseconds offsetFromStart) -> int
{
    using namespace std::chrono_literals;
    auto count = 0;
    for (auto& [sound, noteTime, hit] : notes) {
        if (hit) {
            continue;
        }
        if (noteTime < offsetFromStart - 135ms) {
            count++;
        } else {
            break;
        }
    }
    return count;
}

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
  -> std::pair<std::vector<std::chrono::nanoseconds>, int>
{
    using namespace std::chrono_literals;
    auto misses = std::vector<std::chrono::nanoseconds>{};
    auto count = 0;
    for (auto& [sound, noteTime, hit] : notes) {
        if (hit) {
            count++;
            continue;
        }
        // todo: +135
        if (offsetFromStart > noteTime) {
            misses.push_back(noteTime);
            count++;
            if (sound != nullptr) {
                sound->play();
            }
        } else {
            break;
        }
    }
    return { std::move(misses), count };
}
auto
gameplay_logic::BmsRules::invisibleNoteHit(std::span<NoteType>& notes,
                                           std::chrono::nanoseconds hitOffset)
  -> bool
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
            return false;
        }
        hit = true;
        sound->play();
        return true;
    }
    return false;
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
            count++;
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
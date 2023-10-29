//
// Created by bobini on 24.10.23.
//

#ifndef RHYTHMGAME_CNHITRULES_H
#define RHYTHMGAME_CNHITRULES_H

#include "StandardBmsHitRules.h"
namespace gameplay_logic::rules {
class CnHitRules : public StandardBmsHitRules
{
  public:
    auto getMissesAndLnEndHits(std::span<NoteType> notes,
                               int& currentNoteIndex,
                               std::chrono::nanoseconds offsetFromStart)
      -> std::pair<std::vector<MissData>, std::vector<HitResult>> override;
};
} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_CNHITRULES_H

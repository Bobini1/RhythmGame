//
// Created by PC on 12/04/2026.
//

#ifndef RHYTHMGAME_TIMINGWINDOWSFROMHASH_H
#define RHYTHMGAME_TIMINGWINDOWSFROMHASH_H
#include <QHash>
#include <QPair>
#include "gameplay_logic/Judgement.h"
#include "gameplay_logic/rules/TimingWindows.h"
namespace support {

auto
timingWindowsFromHash(
  const QHash<gameplay_logic::Judgement, QPair<qint64, qint64>>& hash)
  -> gameplay_logic::rules::TimingWindows;

} // namespace support

#endif // RHYTHMGAME_TIMINGWINDOWSFROMHASH_H

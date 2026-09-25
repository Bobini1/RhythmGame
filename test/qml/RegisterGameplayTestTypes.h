#pragma once

#include "gameplay_logic/ChartRunner.h"
#include <QtQml/qqml.h>

inline void
registerGameplayTestTypes()
{
    static const auto registration =
      qmlRegisterUncreatableType<gameplay_logic::ChartRunner>(
        "RhythmGameQml", 1, 0, "ChartRunner", "runner supplied");
    (void)registration;
}

#pragma once

#include "sounds/AudioEngine.h"

// Qt's convenience macro would rewrite RealtimeMixer::emit. The fixture is
// testing active include discovery, not macro interaction.
#ifdef emit
#undef emit
#endif

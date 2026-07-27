#include "gameplay_logic/BmsGaugeHistory.h"
#include "gameplay_logic/BmsLiveScore.h"
#include "gameplay_logic/BmsResult.h"
#include "gameplay_logic/BmsScore.h"
#include "gameplay_logic/ChartData.h"
#include "resource_managers/ChartPlayConfig.h"
#include "resource_managers/ChartPlayOptions.h"
#include "support/GeneratePermutation.h"

static_assert(sizeof(resource_managers::ChartPlayConfig) > 0);

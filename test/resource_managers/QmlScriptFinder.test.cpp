//
// Created by bobini on 29.12.22.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "resource_managers/FindAssetsFolderBoost.h"
#include "resource_managers/QmlScriptFinderImpl.h"
#include "resource_managers/LoadConfig.h"

TEST_CASE("QmlScriptFinder can find the main script", "[qml][scriptfinder]")
{
    auto scriptRoot =
      resource_managers::findAssetsFolder() / "themes" / "Default" / "scripts";
    auto overrides = resource_managers::loadConfig(scriptRoot / "scripts.ini");
    auto finder =
      resource_managers::QmlScriptFinderImpl{ scriptRoot,
                                              overrides["scriptNames"] };
    auto script = finder("Main");
    REQUIRE(!script.empty());
}
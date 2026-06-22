#include "input/InputTranslator.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("beatoraja analog scratch ticks use wrapped 0.009 axis quanta")
{
    using input::InputTranslator;

    CHECK(InputTranslator::computeAnalogScratchTicks(0.0, 0.0) == 0);
    CHECK(InputTranslator::computeAnalogScratchTicks(0.0, 0.009) == 1);
    CHECK(InputTranslator::computeAnalogScratchTicks(0.0, -0.009) == -1);
    CHECK(InputTranslator::computeAnalogScratchTicks(0.0, 0.010) == 2);
    CHECK(InputTranslator::computeAnalogScratchTicks(0.0, -0.010) == -2);

    CHECK(InputTranslator::computeAnalogScratchTicks(0.99, -0.99) == 3);
    CHECK(InputTranslator::computeAnalogScratchTicks(-0.99, 0.99) == -3);
}

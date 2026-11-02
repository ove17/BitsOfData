// TestAssertOverride.cpp

#include "TestAssertOverride.hpp"
#include "TestAssertFunction.h"

extern "C" {

    void cpputest_assert_failed(
        const char *expression,
        const char *file,
        int line)
    {
        throw AssertionFailure();
    }

}

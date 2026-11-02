/* TestAssertOverride.hpp
 *
 * To catch asserts using cpputest:
 *
 * 1) put the following files in the project/test directory:
 *      TestAssertOverride.cpp
 *      TestAssertOverride.hpp
 *      TestAssertFunction.h
 *      testInclude/assert.h
 *
 * 2) add to the makefile:
 *      TEST_SOURCES += $(TEST_DIR)/TestAssertOverride.cpp
 *      CFLAGS += -ItestInclude
 *
 * 3) use CHECK_THROWS in the Test*.cpp:
 *      CHECK_THROWS(AssertionFailure, functionThatTriggersAssert());
 *
 */

#include <exception>

class AssertionFailure : public std::exception
{
public:
    const char* what() const noexcept override
    {
        return "C assertion failed";
    }
};

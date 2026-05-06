// cpputest.h

#pragma once

#include <stdint.h>
#include "CppUTest/TestHarness.h"

void CHECK_UINT16_ARRAY_EQUAL_LOCATION(const uint16_t* expected,
                                       const uint16_t* actual,
                                       size_t len,
                                       const char* file,
                                       int line);

#define CHECK_UINT16_ARRAY_EQUAL(expected, actual, len) \
CHECK_UINT16_ARRAY_EQUAL_LOCATION(expected, actual, len, __FILE__, __LINE__)

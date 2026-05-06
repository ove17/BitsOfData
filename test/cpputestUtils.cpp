// cpputestUtils.cpp

#include "cpputestUtils.h"


void CHECK_UINT16_ARRAY_EQUAL_LOCATION(const uint16_t* expected,
                                       const uint16_t* actual,
                                       size_t len,
                                       const char* file,
                                       int line)
{
    SimpleString exp = "Expected :";
    SimpleString act = "\tActual   :";
    bool areDifferent = false;
    for (size_t i = 0; i < len; i++) {
        exp += StringFromFormat("\t%i", expected[i]);
        act += StringFromFormat("\t%i", actual[i]);
        if (expected[i] != actual[i]) areDifferent = true;
    }
    if (areDifferent) {
        SimpleString msg = exp + "\n" + act;
        FAIL_LOCATION(msg.asCharString(), file, line);
    }
}

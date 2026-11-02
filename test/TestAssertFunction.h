// TestAssertFunction.h

#ifndef TEST_ASSERT_FUNCTION_H
#define TEST_ASSERT_FUNCTION_H

#ifdef __cplusplus
extern "C" {
#endif

void cpputest_assert_failed(
    const char *expression,
    const char *file,
    int line
);

#ifdef __cplusplus
}
#endif

#endif

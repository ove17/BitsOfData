// testInclude/assert.h

#ifndef TEST_ASSERT_H
#define TEST_ASSERT_H

#include_next <assert.h>

#undef assert

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

#define assert(expr) \
((expr) ? (void)0 : cpputest_assert_failed(#expr, __FILE__, __LINE__))

#endif

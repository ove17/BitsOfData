/*
 * BitUtils.h
 */

#ifndef __math_utils_h__
#define __math_utils_h__


#include <stdint.h>


static inline int16_t mu_limitValue(const int16_t minValue,
                                 const int16_t value,
                                 const int16_t maxValue) {
    return (value >= maxValue) ? maxValue :
           (value <= minValue) ? minValue : value;
}

#endif

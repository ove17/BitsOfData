/*
 * BitUtils.h
 */

#ifndef MATH_UTILS_H
#define MATH_UTILS_H


#include <stdint.h>


static inline int16_t mu_limitValue(const int16_t minValue,
                                    const int16_t value,
                                    const int16_t maxValue) {
    return (value >= maxValue) ? maxValue :
           (value <= minValue) ? minValue : value;
}


static inline uint8_t mu_getNumDigits(const uint16_t value) {
    return  (value < 10)    ? 1 :
            (value < 100)   ? 2 :
            (value < 1000)  ? 3 :
            (value < 10000) ? 4 : 5;
}


#endif

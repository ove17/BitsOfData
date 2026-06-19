/*
 * MathUtils.h
 */

#ifndef MATH_UTILS_H
#define MATH_UTILS_H


#include <stdint.h>



typedef enum {
    MU_NO_OPERATOR,
    MU_GREATER_THAN,
    MU_GREATER_THAN_OR_EQUAL,
    MU_EQUAL,
    MU_LESS_THAN,
    MU_LESS_THAN_OR_EQUAL,
    MU_NOT_EQUAL
} mu_operatorT;


static inline int16_t mu_limitValue(const int16_t minValue,
                                    const int16_t value,
                                    const int16_t maxValue) {
    return (value >= maxValue) ? maxValue :
           (value <= minValue) ? minValue : value;
}


static inline uint8_t mu_getNumDigits(const uint16_t value) {
    if (value < 10)    return 1;
    if (value < 100)   return 2;
    if (value < 1000)  return 3;
    if (value < 10000) return 4;
    return 5;
}


static inline bool mu_evalCondition(const uint16_t value,
                                    const mu_operatorT operator,
                                    const uint16_t target) {
    switch(operator) {
        case MU_LESS_THAN :
            return value < target;
        case MU_LESS_THAN_OR_EQUAL :
            return value <= target;
        case MU_EQUAL :
            return value == target;
        case MU_GREATER_THAN :
            return value > target;
        case MU_GREATER_THAN_OR_EQUAL :
            return value >= target;
        case MU_NOT_EQUAL :
            return value != target;
        default :
            return false;
    }
}


#endif

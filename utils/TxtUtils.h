/*
 * TxtUtils.h
 */

#ifndef TXT_UTILS_H
#define TXT_UTILS_H


#include <stdint.h>
#include <assert.h>
#include "MathUtils.h"


// returns the operator in the first 1 or 2 characters of txt
static inline mu_operatorT tu_getOperator(const char* txt) {
    switch(txt[0]) {
        case '=' :
            if (txt[1] == '=') return MU_EQUAL;
            return MU_NO_OPERATOR;
        case '!' :
            if (txt[1] == '=') return MU_NOT_EQUAL;
            return MU_NO_OPERATOR;
        case '>' :
            if (txt[1] == '=') return MU_GREATER_THAN_OR_EQUAL;
            return MU_GREATER_THAN;
        case '<' :
            if (txt[1] == '=') return MU_LESS_THAN_OR_EQUAL;
            return MU_LESS_THAN;
        default :
            return MU_NO_OPERATOR;
    }
}


// returns the number of characters in the operator
static inline uint8_t tu_getOperatorLength(const mu_operatorT op) {
    switch (op) {
        case MU_EQUAL:
        case MU_NOT_EQUAL:
        case MU_GREATER_THAN_OR_EQUAL:
        case MU_LESS_THAN_OR_EQUAL:
            return 2;
        case MU_GREATER_THAN:
        case MU_LESS_THAN:
            return 1;
        default:
            return 0;
    }
}

#endif

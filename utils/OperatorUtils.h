// OperatorUtils. h

#ifndef OPERATOR_UTILS_H
#define OPERATOR_UTILS_H


typedef enum {
    ou_invalid_operator,
    ou_greater_than,
    ou_greater_than_or_equal,
    ou_equal,
    ou_less_than,
    ou_less_than_or_equal,
    ou_not_equal
} ou_operatorT;


// returns the operator in the first 1 or 2 characters of txt
static inline ou_operatorT ou_getOperator(const char* txt) {
    switch(txt[0]) {
        case '=' :
            if (txt[1] == '=') return ou_equal;
            return ou_invalid_operator;
        case '!' :
            if (txt[1] == '=') return ou_not_equal;
            return ou_invalid_operator;
        case '>' :
            if (txt[1] == '=') return ou_greater_than_or_equal;
            return ou_greater_than;
        case '<' :
            if (txt[1] == '=') return ou_less_than_or_equal;
            return ou_less_than;
        default :
            return ou_invalid_operator;
    }
}


// returns the number of characters in the operator
static inline uint8_t ou_getOperatorLength(const ou_operatorT ifOperator) {
    switch (ifOperator) {
        case ou_equal:
        case ou_not_equal:
        case ou_greater_than_or_equal:
        case ou_less_than_or_equal:
            return 2;
        case ou_greater_than:
        case ou_less_than:
            return 1;
        default:
            return 0;
    }
}


static inline bool ou_evalCondition(const uint16_t value,
                                    const ou_operatorT ifOperator,
                                    const uint16_t target) {
    switch(ifOperator) {
        case ou_less_than :
            return value < target;
        case ou_less_than_or_equal :
            return value <= target;
        case ou_equal :
            return value == target;
        case ou_greater_than :
            return value > target;
        case ou_greater_than_or_equal :
            return value >= target;
        case ou_not_equal :
            return value != target;
        default :
            return false;
    }
}


#endif

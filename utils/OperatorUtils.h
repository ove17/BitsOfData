// OperatorUtils. h

#ifndef OPERATOR_UTILS_H
#define OPERATOR_UTILS_H


typedef enum {
    OU_INVALID_OPERATOR,
    OU_GREATER_THAN,
    OU_GREATER_THAN_OR_EQUAL,
    OU_EQUAL,
    OU_LESS_THAN,
    OU_LESS_THAN_OR_EQUAL,
    OU_NOT_EQUAL
} ou_operatorT;


// returns the operator in the first 1 or 2 characters of txt
static inline ou_operatorT ou_getOperator(const char* txt) {
    switch(txt[0]) {
        case '=' :
            if (txt[1] == '=') return OU_EQUAL;
            return OU_INVALID_OPERATOR;
        case '!' :
            if (txt[1] == '=') return OU_NOT_EQUAL;
            return OU_INVALID_OPERATOR;
        case '>' :
            if (txt[1] == '=') return OU_GREATER_THAN_OR_EQUAL;
            return OU_GREATER_THAN;
        case '<' :
            if (txt[1] == '=') return OU_LESS_THAN_OR_EQUAL;
            return OU_LESS_THAN;
        default :
            return OU_INVALID_OPERATOR;
    }
}


// returns the number of characters in the operator
static inline uint8_t ou_getOperatorLength(const ou_operatorT ifOperator) {
    switch (ifOperator) {
        case OU_EQUAL:
        case OU_NOT_EQUAL:
        case OU_GREATER_THAN_OR_EQUAL:
        case OU_LESS_THAN_OR_EQUAL:
            return 2;
        case OU_GREATER_THAN:
        case OU_LESS_THAN:
            return 1;
        default:
            return 0;
    }
}


#endif

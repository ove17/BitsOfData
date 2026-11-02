/* TemplateLanguage.c
 */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "TemplateLanguage.h"

#define MAX_NUM_TOKENS 20
#define NOT_IN_IF_TAG 0xFF


static tl_formatTokenT FormatTokens[MAX_NUM_TOKENS];

enum {
    PLAIN_TEXT,
    START_TAG,
    COLUMN_ID,
    RECORD_ID,
    NUM_RECORDS,
    PARENT_COLUMN_ID,
    IF_LITERAL_VALUE,
    IF_TAG_LEFT_HAND_SIDE,
    IF_TAG_RIGHT_HAND_SIDE,
    ELSE_TAG,
    ENDIF_TAG
};


static inline bool isNumber(char c) {  // TODO: to utils
    return (c >= '0' && c <= '9');
}


static uint8_t updateNumber(const uint8_t number,
                            const uint8_t chr) {
    assert(isNumber(chr) && "invalid char");
    assert((number < 25 || (number == 25 && chr <= '5')) && "number > 255");
    return number * 10 + chr - '0';
}


static void increaseTokenNo(uint8_t *tokenNo) {
    *tokenNo += 1;
    if (*tokenNo < MAX_NUM_TOKENS) {
        FormatTokens[*tokenNo].type = TL_INVALID;
    }
}


static void storeText(uint8_t *tokenNo,
                      const uint8_t txtStart,
                      const uint8_t txtLength) {
    assert(*tokenNo < MAX_NUM_TOKENS);
    FormatTokens[*tokenNo].type = TL_TEXT;
    FormatTokens[*tokenNo].text.length = txtLength;
    FormatTokens[*tokenNo].text.start = txtStart;
    increaseTokenNo(tokenNo);
}


static void storeValue(uint8_t *tokenNo,
                       const tl_tokenTypeT tokenTtype,
                       const tl_valueTypeT valueType,
                       const uint8_t number) {
    assert(*tokenNo < MAX_NUM_TOKENS);
    FormatTokens[*tokenNo].type = tokenTtype;
    FormatTokens[*tokenNo].value.type = valueType;
    FormatTokens[*tokenNo].value.number = number;
    increaseTokenNo(tokenNo);
}


static void storeValidOperator(uint8_t tokenNo,
                               ou_operatorT operator) {
    if (operator != OU_INVALID_OPERATOR) {
        FormatTokens[tokenNo].rhs.ifOperator = operator;
    }
}


static void clearFormatProperties(const uint8_t tokenNo) {
    FormatTokens[tokenNo].value.leading0 = false;
    FormatTokens[tokenNo].value.asHex = false;
    FormatTokens[tokenNo].value.offset1 = false;
}


static void clearIfLhs(const uint8_t tokenNo) {
    FormatTokens[tokenNo].lhs.elseIndex = 0;
    FormatTokens[tokenNo].lhs.endIndex = 0;
}


tl_formatTokenT *tl_parseFormat(const char *formatString) {
    ou_operatorT operator = OU_INVALID_OPERATOR;
    uint8_t number = 0;
    uint8_t tokenNo = 0;
    uint8_t ifTokenNo = NOT_IN_IF_TAG;
    tl_tokenTypeT tokenType = TL_INVALID;
    FormatTokens[0].type = TL_INVALID;
    uint8_t state = PLAIN_TEXT;
    uint8_t plainTxtLen = 0;
    uint8_t plainTxtStart = 0;

    for (uint8_t i = 0; formatString[i] != '\0'; i++) {
        const char chr = formatString[i];
        assert((i < 255 || chr == '\0') && "formatString > 255 characters");
        switch (state) {

            case PLAIN_TEXT : {
                if (chr == '{') {
                    if (plainTxtLen > 0) {
                        storeText(&tokenNo, plainTxtStart, plainTxtLen);
                        plainTxtLen = 0;
                    }
                    number = 0;
                    operator = OU_INVALID_OPERATOR;
                    clearFormatProperties(tokenNo);
                    state = START_TAG;
                } else {
                    assert(chr != '}' && "stray closing brace");
                    if (plainTxtLen == 0) {
                        plainTxtStart = i;
                    }
                    plainTxtLen++;
                }
                break;
            }

            case START_TAG : {
                switch (chr) {
                    case '&' :
                        tokenType = TL_VALUE;
                        state = COLUMN_ID;
                        break;
                    case '#' :
                        tokenType = TL_VALUE;
                        state = RECORD_ID;
                        break;
                    case '*' :
                        state = NUM_RECORDS;
                        break;
                    case '^' : // TODO: consider if o/x/+ properties are allowed
                        state = PARENT_COLUMN_ID;
                        break;
                    case '?' :
                        i++; // skips to '&'
                        assert(formatString[i] == '&');
                        assert(ifTokenNo == NOT_IN_IF_TAG && "nested ifs are invalid");
                        clearIfLhs(tokenNo);
                        ifTokenNo = tokenNo;
                        state = IF_TAG_LEFT_HAND_SIDE;
                        break;
                    case ':' :
                        state = ELSE_TAG;
                        assert(ifTokenNo != NOT_IN_IF_TAG && "else tag must follow if");
                        break;
                    case ';' :
                        assert(ifTokenNo != NOT_IN_IF_TAG && "endif tag must follow if");
                        state = ENDIF_TAG;
                        break;
                    case 'o' : // remains in START_TAG state!
                        FormatTokens[tokenNo].value.leading0 = true;
                        break;
                    case 'x' : // remains in START_TAG state!
                        FormatTokens[tokenNo].value.asHex = true;
                        break;
                    default :
                        assert(0 && "Invalid first character in tag");
                        break;
                }
                break;
            }

            case COLUMN_ID :
                if (chr == '}' ) {
                    storeValidOperator(tokenNo, operator);
                    storeValue(&tokenNo, tokenType, TL_VALUE_COLUMN_ID, number);
                    state = PLAIN_TEXT;
                } else if (chr == '+') {
                    FormatTokens[tokenNo].value.offset1 = true;
                } else {
                    number = updateNumber(number, chr);
                }
                break;

            case RECORD_ID :
                if (chr == '+') {
                    FormatTokens[tokenNo].value.offset1 = true;
                } else {
                    assert(chr == '}' && "invalid character in recordId tag");
                    storeValidOperator(tokenNo, operator);
                    storeValue(&tokenNo, tokenType, TL_VALUE_RECORD_ID, 0);
                    state = PLAIN_TEXT;
                }
                break;

            case NUM_RECORDS :
                assert(chr == '}' && "invalid character in num records tag");
                storeValue(&tokenNo, TL_VALUE, TL_VALUE_NUM_RECORDS, 0);
                state = PLAIN_TEXT;
                break;

            case PARENT_COLUMN_ID :
                if (chr == '}') {
                    storeValue(&tokenNo, TL_VALUE, TL_VALUE_PARENT_COLUMN_ID, number);
                    state = PLAIN_TEXT;
                } else {
                    number = updateNumber(number, chr);
                }
                break;

            case IF_TAG_LEFT_HAND_SIDE :
                if (chr == '=' || chr == '>' || chr == '<'  || chr == '!' ) {
                    storeValue(&tokenNo, TL_IF_LHS, TL_VALUE_COLUMN_ID, number);
                    operator = ou_getOperator(&formatString[i]);
                    assert(operator != OU_INVALID_OPERATOR);
                    if (ou_getOperatorLength(operator) == 2) i++; // skip 2nd operator char
                    number = 0;
                    tokenType = TL_IF_RHS;
                    if(isNumber(formatString[i + 1])) {
                        state = IF_LITERAL_VALUE;
                    } else {
                        state = IF_TAG_RIGHT_HAND_SIDE;
                    }
                } else if (chr == '}') {
                    storeValue(&tokenNo, TL_IF_LHS, TL_VALUE_COLUMN_ID, number);
                    state = PLAIN_TEXT;
                } else {
                    number = updateNumber(number, chr);
                }
                break;

            case IF_LITERAL_VALUE :
                if (chr == '}') {
                    storeValidOperator(tokenNo, operator);
                    storeValue(&tokenNo, tokenType, TL_VALUE_LITERAL, number);
                    state = PLAIN_TEXT;
                } else {
                    number = updateNumber(number, chr);
                }
                break;

            case IF_TAG_RIGHT_HAND_SIDE :
                if (chr == '#') {
                    state = RECORD_ID;
                } else if (chr == '&') {
                    state = COLUMN_ID;
                } else {
                    assert(0 && "invalid character in if tag RHS");
                }
                break;

            case ELSE_TAG :
                assert(chr == '}' && "invalid character in ELSE tag");
                assert(FormatTokens[ifTokenNo].lhs.elseIndex == 0 && "2 else tags found");
                FormatTokens[ifTokenNo].lhs.elseIndex = tokenNo;
                state = PLAIN_TEXT;
                break;

            case ENDIF_TAG :
                assert(chr == '}' && "invalid character in ENDIF tag");
                FormatTokens[ifTokenNo].lhs.endIndex = tokenNo;
                ifTokenNo = NOT_IN_IF_TAG;
                state = PLAIN_TEXT;
                break;

            default :
                assert(0 && "invalid state");
                break;
        }
    }
    if (plainTxtLen > 0) {
        // the current (=last) token was plain text that is to be saved:
        storeText(&tokenNo, plainTxtStart, plainTxtLen);
    }
    assert(tokenNo > 0 && "no formatString found");
    assert(ifTokenNo == NOT_IN_IF_TAG && "if tag without endif");
    assert(state == PLAIN_TEXT && "missing closing brace");
    return FormatTokens;
}

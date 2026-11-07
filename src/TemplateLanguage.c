/* TemplateLanguage.c
 */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "TemplateLanguage.h"
#include "OperatorUtils.h"

#define NOT_IN_IF_TAG 0xFF
#define RESET_TEXT_PARSER '\xFF'


//TODO: consider if this should be input (including MAX_NUM_TOKENS)
static tl_tokenT Tokens[tl_MAX_NUM_TOKENS];

enum {
    plain_text,
    checkIfTag,
    tag_prefix,
    main_tag,
    column_id,
    record_id,
    num_records,
    parent_column_id,
    if_literal_value,
    if_tag_left_hand_side,
    if_tag_right_hand_side,
    else_tag,
    endif_tag
};


static inline bool isNumber(const char c) {  // TODO: to utils
    return (c >= '0' && c <= '9');
}


static uint8_t updateValue(const uint8_t value,
                           const uint8_t chr) {
    assert(isNumber(chr) && "invalid char");
    assert((value < 25 || (value == 25 && chr <= '5')) && "number > 255");
    return value * 10 + chr - '0';
}


static void increasetokenId(uint8_t *tokenId) {
    *tokenId += 1;
    if (*tokenId < tl_MAX_NUM_TOKENS) {
        Tokens[*tokenId].type = tl_invalid;
    }
}


static void storeText(uint8_t *tokenId,
                      const uint8_t txtStart,
                      const uint8_t txtLength) {
    assert(*tokenId < tl_MAX_NUM_TOKENS);
    Tokens[*tokenId].type = tl_text;
    Tokens[*tokenId].text.length = txtLength;
    Tokens[*tokenId].text.start = txtStart;
    increasetokenId(tokenId);
}


static void storeValue(uint8_t *tokenId,
                       const tl_tokenTypeT tokenTtype,
                       const tl_valueTypeT valueType,
                       const uint8_t value) {
    assert(*tokenId < tl_MAX_NUM_TOKENS);
    Tokens[*tokenId].type = tokenTtype;
    Tokens[*tokenId].valType = valueType;
    Tokens[*tokenId].value = value;
    increasetokenId(tokenId);
}


static void storeOperatorIfValid(const uint8_t tokenId,
                                 const ou_operatorT operator) {
    if (operator != ou_invalid_operator) {
        Tokens[tokenId].rhs.ifOperator = operator;
    }
}


//static void clearFormatProperties(const uint8_t tokenId) {
static void clearFormatProperties(const uint8_t  tokenId) {
    Tokens[tokenId].properties.leading0 = false;
    Tokens[tokenId].properties.asHex = false;
    Tokens[tokenId].properties.offset1 = false;
    Tokens[tokenId].properties.isFixed = false;
    Tokens[tokenId].properties.isHidden = false;
}


static bool parseText(uint8_t *tokenId,
                      const char chr,
                      const uint8_t charId) {
    static uint8_t plainTxtLen = 0;
    static uint8_t plainTxtStart = 0;
    if (chr == RESET_TEXT_PARSER) {
        plainTxtLen = 0;
        plainTxtStart = 0;
        return false;
    }
    if (chr == '{') {
        if (plainTxtLen > 0) {
            storeText(tokenId, plainTxtStart, plainTxtLen);
            plainTxtLen = 0;
        }
        return true;
    }
    assert(chr != '}' && "stray closing brace");
    if (plainTxtLen == 0) {
        plainTxtStart = charId;
    }
    plainTxtLen++;
    return false;
}


static void clearIfLhs(const uint8_t tokenId) {
    Tokens[tokenId].lhs.elseIndex = 0;
    Tokens[tokenId].lhs.endIndex = 0;
}


tl_tokenT *tl_parseFormat(const char *formatString) {
    ou_operatorT operator = ou_invalid_operator;
    uint8_t value = 0;
    uint8_t tokenId = 0;
    uint8_t iftokenId = NOT_IN_IF_TAG;
    tl_tokenTypeT tokenType = tl_invalid;
    Tokens[0].type = tl_invalid;
    uint8_t state = plain_text;
    parseText(&tokenId, RESET_TEXT_PARSER, 0);

    for (uint8_t i = 0; formatString[i] != '\0'; i++) {
        const char chr = formatString[i];
        assert((i < 255 || chr == '\0') && "formatString > 255 characters");

        switch (state) {
            case plain_text : {
                bool tagIsFound = parseText(&tokenId, chr, i);
                if (tagIsFound) {
                    value = 0;
                    operator = ou_invalid_operator;
                    clearFormatProperties(tokenId);
                    state = checkIfTag;
                }
                break;
            }

            case checkIfTag :
                state = tag_prefix;
                if (chr == '?') { // MUST be the 1st char in a tag
                    assert(iftokenId == NOT_IN_IF_TAG
                    && "nested ifs are invalid");
                    clearIfLhs(tokenId);
                    iftokenId = tokenId;
                    tokenType = tl_if_lhs;
                    break;
                }
                // intentional conditional fall through if no ? found

            case tag_prefix : {
                switch (chr) {
                    case 'o' :
                        Tokens[tokenId].properties.leading0 = true;
                        break;
                    case 'x' :
                        Tokens[tokenId].properties.asHex = true;
                        break;
                    case 'h' :
                        Tokens[tokenId].properties.isHidden = true;
                        break;
                    case 'f' :
                        Tokens[tokenId].properties.isFixed = true;
                        break;
                    default :
                        state = main_tag;
                        break;
                }
                if (state != main_tag) {
                    break; // intentional conditional fall-through
                }
            case main_tag :
                switch (chr) {
                    case '&' :
                        if (tokenType != tl_if_lhs) {
                            tokenType = tl_value;
                        }
                        state = column_id;
                        break;
                    case '#' :
                        assert(!Tokens[tokenId].properties.asHex
                                && !Tokens[tokenId].properties.isHidden
                                && !Tokens[tokenId].properties.isFixed);
                        tokenType = tl_value;
                        state = record_id;
                        break;
                    case '*' :
                        assert(!Tokens[tokenId].properties.asHex
                                && !Tokens[tokenId].properties.isHidden
                                && !Tokens[tokenId].properties.isFixed);
                        state = num_records;
                        break;
                    case '^' :
                        assert(!Tokens[tokenId].properties.asHex
                                && !Tokens[tokenId].properties.isHidden
                                && !Tokens[tokenId].properties.isFixed
                                && !Tokens[tokenId].properties.leading0);
                        state = parent_column_id;
                        break;
                    case ':' :
                    case ';' :
                        assert(iftokenId != NOT_IN_IF_TAG
                                && "else tag must follow if");
                        assert(formatString[i - 1] == '{'
                                && "invalid char in else tag");
                        assert(formatString[i + 1] == '}'
                                && "invalid character in ELSE or ENDIF tag");
                        state = (chr == ':') ? else_tag : endif_tag ;
                        break;
                    default :
                        assert(0 && "Invalid first character in tag");
                        break;
                }
                break;
            }

            case column_id :
                switch(chr) {
                    case '}' :
                        storeOperatorIfValid(tokenId, operator);
                        storeValue(&tokenId, tokenType, tl_value_column_id, value);
                        state = plain_text;
                        break;
                    case '+' :
                        Tokens[tokenId].properties.offset1 = true;
                        break;
                    case '=' :
                    case '>' :
                    case '<' :
                    case '!' :
                        assert(tokenType == tl_if_lhs);
                        storeValue(&tokenId, tl_if_lhs, tl_value_column_id, value);
                        operator = ou_getOperator(&formatString[i]);
                        assert(operator != ou_invalid_operator);
                        if (ou_getOperatorLength(operator) == 2) {
                            i++; // skip 2nd operator char
                        }
                        value = 0;
                        tokenType = tl_if_rhs;
                        state = if_tag_right_hand_side;
                        break;
                    default:
                        value = updateValue(value, chr);
                        break;
                }
                break;

            case record_id :
                switch(chr) {
                    case '+' :
                        Tokens[tokenId].properties.offset1 = true;
                        break;
                    case '}' :
                        storeOperatorIfValid(tokenId, operator);
                        storeValue(&tokenId, tokenType, tl_value_record_id, 0);
                        state = plain_text;
                        break;
                    default :
                        assert(0 && "invalid character in recordId tag");
                        break;
                }
                break;

            case num_records :
                assert(chr == '}' && "invalid character in num records tag");
                storeValue(&tokenId, tl_value, tl_value_num_records, 0);
                state = plain_text;
                break;

            case parent_column_id :
                if (chr == '}') {
                    storeValue(&tokenId, tl_value, tl_value_parent_column_id, value);
                    state = plain_text;
                } else {
                    value = updateValue(value, chr);
                }
                break;

            case if_tag_right_hand_side :
                if (chr == '#') {
                    state = record_id;
                } else if (chr == '&') {
                    state = column_id;
                }
                if (state != if_tag_right_hand_side) {
                    break; // intentional conditional fall-through
                }
            case if_literal_value :
                if (chr == '}') {
                    storeOperatorIfValid(tokenId, operator);
                    storeValue(&tokenId, tokenType, tl_value_literal, value);
                    state = plain_text;
                } else {
                    value = updateValue(value, chr);
                }
                break;

            case else_tag :
                assert(Tokens[iftokenId].lhs.elseIndex == 0
                        && "2 else tags found");
                Tokens[iftokenId].lhs.elseIndex = tokenId;
                state = plain_text;
                break;

            case endif_tag :
                Tokens[iftokenId].lhs.endIndex = tokenId;
                if(Tokens[iftokenId].lhs.elseIndex == 0) {
                    // no else tag found, elseIndex = endIndex
                    Tokens[iftokenId].lhs.elseIndex = tokenId;
                }
                iftokenId = NOT_IN_IF_TAG;
                state = plain_text;
                break;

            default :
                assert(0 && "invalid state");
                break;
        }
    }
    // if the current (=last) token was plain text, store it:
    parseText(&tokenId, '{', 0);

    assert(tokenId > 0 && "no formatString found");
    assert(iftokenId == NOT_IN_IF_TAG && "if tag without endif");
    assert(state == plain_text && "missing closing brace");
    return Tokens;
}

/* TemplateLanguage.h
 *
 * Parser for the BDB template language (BDB-TL), its syntax is descried in
 *      include/BitsOfDataTemplates.h
 * BDB-TL is intended to format the output of the BitsOfData database, so it is
 *  only intended to be used by programmers.
 *
 * The parser takes a format string as input. Its result is an aray of
 *  tl_formatTokenT structs.
 * There is no explicit numTokens variable: the number of tokens is indicated by
 *  the the end of valid tokens, i.e. the first occurance of:
 *      FormatToken[i].type == tl_invalid
 *
 * The parser checks formatString syntax, syntax errors result in assertion
 *  failures, so the user (= programmer of the caller) is responsible.
 *
 * The resulting tl_formatTokenT array still needs interpreting, e.g. database
 *  calls and if statements are not executed by the parser.
 *
 * The format of the tokens is defined in this header.
 * Additional notes about the tokens:
 *  - a tl_text token only points to the text in the formatString, it does not
 *      hold the text itself.
 *  - a tl_value token may hold a literal number or a database (parent)columnId,
 *      but it may also indicate a type, like the current recordId or number of
 *      records of the curent table.
 *  - The TL_IF_* tokens also hold values, they use the same tl_valueT
 *      substruct as tl_value
 *  - A condition, e.g. {?&0}, results in a stand alone tl_if_lhs token
 *  - A comparison, e.g. {?&0==0}, results in a tl_if_lhs token, immediately
 *      followed by a tl_if_rhs token.
 *  - The jump indices of tl_if_lhs always point forwards to the 1st token
 *      after the {:} or {;} tag. If the {;} tag is the last in the formatString
 *      endIndex will point 1 token after the last valid one (i.e. the 1st
 *      tl_invalid token).
 *  - If no {:} is present between {?..} and {;}, the elseIndex will be set to
 *      the same value as the endIndex.
 *
 * The following are considered syntax errors and will trigger asserts:
 *  - an empty format string
 *  - a format string longer than 255 characters (where str[256] == '\0')
 *  - an opening brace without a closing brace
 *  - a closing brace without an opening brace
 *  - a formatString that results in more than MAX_NUM_TOKENS
 *  - a number > 255 between braces (columnId, literal)
 *  - a recordId with an offset {*+}
 *  - an if tag without an endif tag
 *  - an endif or else tag without an if tag
 *  - nested if tags
 *  - any invalid (combination of) pre- and postfixes
 */

#ifndef TEMPLATE_LANGUAGE_H
#define TEMPLATE_LANGUAGE_H

#include <stdint.h>
#include <stdbool.h>

#include "OperatorUtils.h"

#define tl_MAX_NUM_TOKENS 20


typedef enum {
    tl_value_none,
// values with format properties:
    tl_value_column_id,
    tl_value_record_id,
    tl_value_num_records,
// values without format properties:
    tl_value_parent_column_id, // NOTE: inherits parent column properties
    tl_value_literal
} tl_valueTypeT;


typedef struct {
    bool leading0;
    bool offset1;
    bool asHex;
    bool isHidden;
    bool isFixed;
} tl_valuePropT;

typedef enum {
    tl_invalid, // no valid token(type)
    tl_text,
    tl_value,
    tl_if_lhs,
    tl_if_rhs,
} tl_tokenTypeT;

/* FIXME do this or delete:
typedef enum {
    tl_invalid, // no valid token(type)
    tl_text,
    tl_value_column_id,
    tl_value_record_id,
    tl_value_num_records,
    tl_value_parent_column_id, // NOTE: inherits parent column properties
    tl_if_lhs_column_id,    // OR: lhs_solo, lhs_with_rhs??
    tl_if_rhs_column_id,
    tl_if_rhs_record_id,
    tl_if_rhs_literal
} tl_tokenTypeT;
*/

typedef struct {
    tl_tokenTypeT type;
    tl_valueTypeT valType; // TODO: could/should this be integrated into tl_tokenTypeT?
    uint8_t value;
    tl_valuePropT properties;

    union {
        struct {
            uint8_t editPer; // editing per 1, 2 digits. 0 means edit whole number
        } val;

        struct {
            uint8_t start;  // index in the textFmt string
            uint8_t length;
        } text;

        struct { // elseIndex == endIndex means there is no else
            uint8_t elseIndex;
            uint8_t endIndex;
        } lhs;

        struct {
            ou_operatorT ifOperator;
        } rhs;
    };
} tl_tokenT;


tl_tokenT *tl_parseFormat(const char *formatString);

#endif

/* TemplateLanguage.h
 *
 * Parser for the BDB template language (BDB-TL), its syntax is descried in
 *      include/BitsOfDataTemplates.h\
 * BDB-TL is intended to format the output of the BitsOfData database, so it is
 *  only intended to be used by programmers.
 *
 * The parser takes a format string as input. Its result is an aray of
 *  tl_formatTokenT structs.
 * There is no explicit numTokens variable: the first
 *  FormatToken[i].type == TL_INVALID indicates the end of valid tokens and
 *  hence the number of tokens.
 * The parser checks formatString syntax, syntax errors result in assertion
 *  failures, so the user (= programmer of the caller) is responsible.
 *
 * The resulting tl_formatTokenT array still needs interpreting, e.g. database
 *  calls and if statements are not executed by the parser.
 *
 * The format of the tokens is defined in this header.
 * Additional notes about the tokens:
 *  - a TL_TEXT token only points to the text in the formatString, it does not
 *      hold the text itself.
 *  - a TL_VALUE token may hold a literal number or a database (parent)columnId,
 *      but it may also indicate a type, like the current recordId or number of
 *      records of the curent table.
 *  - The TL_IF_* tokens also hold values, they use the same tl_valueT
 *      substruct as TL_VALUE
 *  - A condition, e.g. {?&0}, results in a stand alone TL_IF_LHS token
 *  - A comparison, e.g. {?&0==0}, results in a TL_IF_LHS token, immediately
 *      followed by a TL_IF_RHS token.
 *  - The jump indices of TL_IF_LHS always point forwards to the 1st token
 *      after the {:} or {;} tag. If the {;} tag is the last in the formatString
 *      endIndex will point 1 token after the last valid one (i.e. the 1st
 *      TL_INVALID token).
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
 */

#ifndef TEMPLATE_LANGUAGE_H
#define TEMPLATE_LANGUAGE_H

#include <stdint.h>
#include <stdbool.h>

#include "OperatorUtils.h"


typedef enum {
// values with format properties:
    TL_VALUE_COLUMN_ID,
    TL_VALUE_RECORD_ID,
    TL_VALUE_NUM_RECORDS,
// values without format properties:
    TL_VALUE_PARENT_COLUMN_ID, // FIXME: inherits parent column props, or overrides them?
    TL_VALUE_LITERAL,
} tl_valueTypeT;


typedef struct {
    tl_valueTypeT type; // TODO: could/should this be integrated into tl_tokenTypeT?
    uint8_t number;
    bool leading0;
    bool offset1;
    bool asHex;
} tl_valueT;


typedef enum {
    TL_INVALID, // no valid token(type)
    TL_TEXT,
    TL_VALUE,
    TL_IF_LHS,
    TL_IF_RHS,
} tl_tokenTypeT;


typedef struct {
    tl_tokenTypeT type;
    tl_valueT value;
    union {
        struct {
            uint8_t start;  // index in the textFmt string
            uint8_t length;
        } text;

        struct {
            uint8_t elseIndex;  // elseIndex == 0 means there is no else
            uint8_t endIndex;
        } lhs;

        struct {
            ou_operatorT ifOperator;
        } rhs;
    };
} tl_formatTokenT;


tl_formatTokenT *tl_parseFormat(const char *formatString);

/*
 * FIXME: overweeg:
 *
bool tl_parseFormat(
    const char *formatString,
    tl_formatTokenT *tokens,
    uint8_t maxTokens
);


// FIXME: voorlopig dit niet:
typedef struct {
    const char *fmtString; // TODO: ptr naar de string, of de txtIndex van de string?
    uint8_t numTokens;  // FIXME: NOT needed: TL_INVALID or MAX_NUM_TOKENS!
    uint8_t numColumns;
        / * numColumns = aantal kolomreferenties dat tijdens een specifieke
         * uitvoering actief is. DUS kan de parser dit niet berekenen, maar
         * wanneer de interpreter dit berekend heeft, kan deze hier wel
         * opgeslagen worden.
         * numColumns == 0 of == 0xFF means "not yet determined"
         * /
    FormatToken tokens[FORMAT_MAX_TOKENS];  // result
} ParsedFormatT;

// tp_parseFormatString(const char *fmtStr)

// on the consumer side:
(static) uint16_t evaluateFormatValue(&tl_valueT formatValue); // + db arguments?

result = mu_evalCondition(
    left,
    token->condition.operator,
    right
);
*/

#endif

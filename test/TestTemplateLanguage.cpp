/* TestTemplateLanguage.cpp
 *
 * TODO:
 *  - alle formatstrings moeten een keer aangeroepen zijn om MAX_NUM_TOKENS te
 *      checken
 */

#include "CppUTest/TestHarness.h"
#include "cpputestUtils.h"

#include "TestAssertOverride.hpp"

extern "C" {
    #include "TemplateLanguage.h"
    #include "OperatorUtils.h"
}


TEST_GROUP(TemplateLanguage) {
    void setup() {
    }

    void teardown() {
    }
};


// SYNTAX ERRORS causing asserts


TEST(TemplateLanguage,
     parsingEmptyString_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat(""));
}


TEST(TemplateLanguage,
     parsing256charString_triggersAnAssert) {
    char str[257];
    memset(str, 'A', 256);
    str[256] = '\0';
    CHECK_THROWS(AssertionFailure, tl_parseFormat(str));
}


TEST(TemplateLanguage,
     parsingMaxNumTokensPlus1_triggersAnAssert) {
    CHECK_THROWS(AssertionFailure,
        tl_parseFormat("{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&5}t{&1}")
    );
}


TEST(TemplateLanguage,
     parsingColumnId256_triggersAnAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{&256}"));
}


TEST(TemplateLanguage,
     parsingRecordId_withOffset_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{*+}"));
}


TEST(TemplateLanguage,
      parsingTwoConsecutiveOpeningTag_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{{&1}b"));
}


TEST(TemplateLanguage,
      parsingOpeningTagWithoutClosingTag_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{&1"));
}


TEST(TemplateLanguage,
      parsingTwoConsecutiveClosingTags_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{&1}}b"));
}


TEST(TemplateLanguage,
      parsingTextWithStrayClosingBrace_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("abc}def"));
}


TEST(TemplateLanguage,
      parsingTextWithlastClosingBrace_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("abcdef}"));
}


TEST(TemplateLanguage,
      parsingIfWithInvalidCharacterInEndIf_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1>23}txt{;0}next"));
}


TEST(TemplateLanguage,
     parsingIfWithNestedIf_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{?&1>23}b{?&3}c{:}d{;}e"));
}


TEST(TemplateLanguage,
     parsingIfWithNestedIfInElse_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{?&1>23}b{:}d{?&3}c{;}e"));
}


TEST(TemplateLanguage,
     parsingIfWithTwoEndifs_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{?&1>23}b{;}c{;}d"));
}


TEST(TemplateLanguage,
     parsingIfWithTwoElses_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{?&1>23}b{:}c{:}d{;}"));
}


TEST(TemplateLanguage,
     parsingIfWithoutEndif_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{?&1>23}b{&1}c"));
}


TEST(TemplateLanguage,
     parsingIfWithElseWithoutEndif_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{?&1>23}b{:}c"));
}


TEST(TemplateLanguage,
     parsingEndIfWithoutIf_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{;}b"));
}


TEST(TemplateLanguage,
     parsingElseWithoutIf_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{:}b"));
}


TEST(TemplateLanguage,
      parsingIfWithInvalidCharacterInElse_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1>23}a{:9}b{;}C"));
}


// TEXT


TEST(TemplateLanguage,
     parsing255charString_succeeds) {
    char str[256];
    memset(str, 'A', 255);
    str[255] = '\0';
    tl_formatTokenT *result = tl_parseFormat(str);
    BYTES_EQUAL(TL_TEXT, result[0].type);
    BYTES_EQUAL(255, result[0].text.length);
}


TEST(TemplateLanguage,
     parsingPlainText_returnsTokenWithPlainText) {
    tl_formatTokenT *result = tl_parseFormat("text:*&");
    BYTES_EQUAL(0, result[0].text.start);
    BYTES_EQUAL(7, result[0].text.length);
}


TEST(TemplateLanguage,
     parsingPlainText_returnsTokenWithTokenTypeTEXT) {
    tl_formatTokenT *result = tl_parseFormat("text:*&");
    BYTES_EQUAL(TL_TEXT, result[0].type);
}


TEST(TemplateLanguage,
     parsingPlainTextAfterColumnId_returnsTokenWithPlainText_andTokenTypeTEXT) {
    tl_formatTokenT *result = tl_parseFormat("{&3}TXT");
    BYTES_EQUAL(4, result[1].text.start);
    BYTES_EQUAL(3, result[1].text.length);
    BYTES_EQUAL(TL_TEXT, result[1].type);
}


TEST(TemplateLanguage,
     parsingPlainTextBeforeColumnId_returnsTokenWithPlainText_andTokenTypeTEXT) {
    tl_formatTokenT *result = tl_parseFormat("@#{&8}");
    BYTES_EQUAL(0, result[0].text.start);
    BYTES_EQUAL(2, result[0].text.length);
    BYTES_EQUAL(TL_TEXT, result[0].type);
}


// COLUMN_ID


TEST(TemplateLanguage,
     parsingColumnId0_returnsTokenWithColumnId0) {
    tl_formatTokenT *result = tl_parseFormat("{&0}");
    BYTES_EQUAL(0, result[0].value.number);
}


TEST(TemplateLanguage,
     parsingColumnId_returnsTokenWithTokenTypeVALUE) {
    tl_formatTokenT *result = tl_parseFormat("{&0}");
    BYTES_EQUAL(TL_VALUE, result[0].type);
}


TEST(TemplateLanguage,
     parsingColumnId_returnsTokenWithTokenValueTypeCOLUMN_ID) {
    tl_formatTokenT *result = tl_parseFormat("{&0}");
    BYTES_EQUAL(TL_VALUE_COLUMN_ID, result[0].value.type);
}


TEST(TemplateLanguage,
     parsingSingleToken_setsSecondTokenToTypeINVALID) {
    tl_formatTokenT *result = tl_parseFormat("{&5}");
    BYTES_EQUAL(TL_INVALID, result[1].type);
}


TEST(TemplateLanguage,
     parsing2valueTokens_sets3rdTokenToTypeINVALID) {
    tl_formatTokenT *result = tl_parseFormat("{&5}{&2}");
    BYTES_EQUAL(TL_INVALID, result[2].type);
}


TEST(TemplateLanguage,
     parsing2ndTokenAsTxt_sets3rdTokenToTypeINVALID) {
    tl_formatTokenT *result = tl_parseFormat("{&5}txt");
    BYTES_EQUAL(TL_INVALID, result[2].type);
}


TEST(TemplateLanguage,
     parsingMaxMinus1tokens_setsLastPlus1TokenToTypeINVALID) {
    const char *str = "t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t";
    tl_formatTokenT *result = tl_parseFormat(str);
    BYTES_EQUAL(TL_INVALID, result[19].type);
}

TEST(TemplateLanguage,
     parsingMaxNumTokens_setsLastTokenToTypeTL_VALUE) {
    const char *str = "t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&5}";
    tl_formatTokenT *result = tl_parseFormat(str);
    BYTES_EQUAL(5, result[19].value.number);
    BYTES_EQUAL(TL_VALUE, result[19].type);
}


TEST(TemplateLanguage,
     parsingMaxNumTokens_setsLastTokenToTypeTL_TEXT) {
    const char *str = "{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&5}t";
    tl_formatTokenT *result = tl_parseFormat(str);
    BYTES_EQUAL(49, result[19].text.start);
    BYTES_EQUAL(TL_TEXT, result[19].type);
}


TEST(TemplateLanguage,
     parsingColumnId1_returnsTokenWithColumnId1) {
    tl_formatTokenT *result = tl_parseFormat("{&1}");
    BYTES_EQUAL(1, result[0].value.number);
}


TEST(TemplateLanguage,
     parsingColumnId255_returnsTokenWithColumnId255) {
    tl_formatTokenT *result = tl_parseFormat("{&255}");
    BYTES_EQUAL(255, result[0].value.number);
}


TEST(TemplateLanguage,
     parsingTwoColumns_returnsArrayWithTwoTokens) {
    tl_formatTokenT *result = tl_parseFormat("{&5}{&2}");
    BYTES_EQUAL(5, result[0].value.number);
    BYTES_EQUAL(2, result[1].value.number);
}


TEST(TemplateLanguage,
     parsingColumnId_PlainText_ColumnId_returnsVALUEasLastTokenType) {
    tl_formatTokenT *result = tl_parseFormat("{&3}@#{&8}");
    BYTES_EQUAL(TL_VALUE, result[2].type);
}


TEST(TemplateLanguage,
     parsingColumnId_withLeading0_returnsTokenValueWithLeading0) {
    tl_formatTokenT *result = tl_parseFormat("{o&1}");
    CHECK_TRUE(result[0].value.leading0);
}


TEST(TemplateLanguage,
     parsingColumnId_withoutLeading0_afterOneWith_returnsTokenWithoutLeading0) {
    tl_formatTokenT *result = tl_parseFormat("{o&1}");
    CHECK_TRUE(result[0].value.leading0);
    result = tl_parseFormat("{&2}");
    CHECK_FALSE(result[0].value.leading0);
}


TEST(TemplateLanguage,
     parsingColumnId_withHex_returnsTokenValueWithHex) {
    tl_formatTokenT *result = tl_parseFormat("{x&1}");
    CHECK_TRUE(result[0].value.asHex);
}


TEST(TemplateLanguage,
     parsingColumnId_withoutHex_afterOneWith_returnsTokenValueWithoutHex) {
    tl_formatTokenT *result = tl_parseFormat("{x&2}");
    CHECK_TRUE(result[0].value.asHex);
    result = tl_parseFormat("{&3}");
    CHECK_FALSE(result[0].value.asHex);
}


TEST(TemplateLanguage,
     parsingColumnId_withOffset_returnsTokenValueWithOffset) {
    tl_formatTokenT *result = tl_parseFormat("{&1+}");
    CHECK_TRUE(result[0].value.offset1);
}


TEST(TemplateLanguage,
     parsingColumnId_withoutOffset_afterOneWith_returnsTokenValueWithoutOffset) {
    tl_formatTokenT *result = tl_parseFormat("{&2+}");
    CHECK_TRUE(result[0].value.offset1);
    result = tl_parseFormat("{&3}");
    CHECK_FALSE(result[0].value.offset1);
}


// RECORD_ID


TEST(TemplateLanguage,
     parsingRecordId_returnsTokenWithTypeVALUE) {
    tl_formatTokenT *result = tl_parseFormat("{#}");
    BYTES_EQUAL(TL_VALUE, result[0].type);
}


TEST(TemplateLanguage,
     parsingRecordId_returnsTokenValueWithTypeRECORD_ID) {
    tl_formatTokenT *result = tl_parseFormat("{#}");
    BYTES_EQUAL(TL_VALUE_RECORD_ID, result[0].value.type);
}


TEST(TemplateLanguage,
     parsingRecordId_withOffset_returnsTokenValueWithOffset) {
    tl_formatTokenT *result = tl_parseFormat("{#+}");
    CHECK_TRUE(result[0].value.offset1);
}


TEST(TemplateLanguage,
     parsingRecordId_withoutOffset_afterOneWith_returnsTokenValueWithoutOffset) {
    tl_formatTokenT *result = tl_parseFormat("{#+}");
    CHECK_TRUE(result[0].value.offset1);
    result = tl_parseFormat("{#}");
    CHECK_FALSE(result[0].value.offset1);
}


TEST(TemplateLanguage,
     parsingRecordId_withLeading0_returnsTokenValueWithLeading0) {
    tl_formatTokenT *result = tl_parseFormat("{o#}");
    CHECK_TRUE(result[0].value.leading0);
}


// NUM_RECORDS


TEST(TemplateLanguage,
     parsingNumRecords_returnsTokenWithTypeVALUE) {
    tl_formatTokenT *result = tl_parseFormat("{*}");
    BYTES_EQUAL(TL_VALUE, result[0].type);
}


TEST(TemplateLanguage,
     parsingNumRecords_returnsTokenValueWithTypeNUM_RECORDS) {
    tl_formatTokenT *result = tl_parseFormat("{*}");
    BYTES_EQUAL(TL_VALUE_NUM_RECORDS, result[0].value.type);
}


TEST(TemplateLanguage,
     parsingNumRecords_withLeading0_returnsTokenValueWithLeading0) {
    tl_formatTokenT *result = tl_parseFormat("{o*}");
    CHECK_TRUE(result[0].value.leading0);
}


// PARENT


TEST(TemplateLanguage,
     parsingParent_returnsTokenWithTypeVALUE) {
    tl_formatTokenT *result = tl_parseFormat("{^1}");
    BYTES_EQUAL(TL_VALUE, result[0].type);
}


TEST(TemplateLanguage,
     parsingParentRecordId_returnsTokenValueWithTypePARENT_COLUMN_ID) {
    tl_formatTokenT *result = tl_parseFormat("{^0}");
    BYTES_EQUAL(TL_VALUE_PARENT_COLUMN_ID, result[0].value.type);
}


TEST(TemplateLanguage,
     parsingParentRecordId_returnsTokenValue) {
    tl_formatTokenT *result = tl_parseFormat("{^8}");
    BYTES_EQUAL(8, result[0].value.number);
}


// IF LHS


TEST(TemplateLanguage,
     parsingIfLhs_returnsTokenWithTypeIF_LHS) {
    tl_formatTokenT *result = tl_parseFormat("{?&1}{;}");
    BYTES_EQUAL(TL_IF_LHS, result[0].type);
}


TEST(TemplateLanguage,
     parsingIfLhs_returnsTokenWithValueTypeCOLUMN_ID) {
    tl_formatTokenT *result = tl_parseFormat("{?&1}{;}");
    BYTES_EQUAL(TL_VALUE_COLUMN_ID, result[0].value.type);
}


TEST(TemplateLanguage,
     parsingIfLhs_returnsTokenWithValueColumnId) {
    tl_formatTokenT *result = tl_parseFormat("{?&12}{;}");
    BYTES_EQUAL(12, result[0].value.number);
}


TEST(TemplateLanguage,
     parsingIfLhs_followedByTxt_returns2nsTokenTxt) {
    tl_formatTokenT *result = tl_parseFormat("{?&12}next{;}");
    BYTES_EQUAL(TL_TEXT, result[1].type);
    BYTES_EQUAL(6, result[1].text.start);
}


// IF RHS = COLUMN_ID


TEST(TemplateLanguage,
     parsingIfRhsWithColumnId_returns2ndTokenWithTypeIF_RHS) {
    tl_formatTokenT *result = tl_parseFormat("{?&8==&1}x{;}");
    BYTES_EQUAL(TL_IF_LHS, result[0].type);
    BYTES_EQUAL(TL_IF_RHS, result[1].type);
    BYTES_EQUAL(TL_TEXT, result[2].type);
}


TEST(TemplateLanguage,
     parsingIfRhsWithColumnId_returns2ndTokenWithValueTypeCOLUMN_ID) {
    tl_formatTokenT *result = tl_parseFormat("{?&2==&3}{;}");
    BYTES_EQUAL(TL_VALUE_COLUMN_ID, result[0].value.type);
    BYTES_EQUAL(TL_VALUE_COLUMN_ID, result[1].value.type);
}


TEST(TemplateLanguage,
     parsingIfRhsWithColumnId_returns2ndTokenWithValueColumnId) {
    tl_formatTokenT *result = tl_parseFormat("{?&4==&5}{;}");
    BYTES_EQUAL(4, result[0].value.number);
    BYTES_EQUAL(5, result[1].value.number);
}


// IF RHS = RECORD_ID


TEST(TemplateLanguage,
     parsingIfRhsWithRecordId_returns2ndTokenWithTypeIF_RHS) {
    tl_formatTokenT *result = tl_parseFormat("{?&8==#}x{;}");
    BYTES_EQUAL(TL_IF_LHS, result[0].type);
    BYTES_EQUAL(TL_IF_RHS, result[1].type);
    BYTES_EQUAL(TL_TEXT, result[2].type);
}


TEST(TemplateLanguage,
     parsingIfRhsWithRecordId_returns2ndTokenWithValueTypeRECORD_ID) {
    tl_formatTokenT *result = tl_parseFormat("{?&2==#}{;}");
    BYTES_EQUAL(TL_VALUE_COLUMN_ID, result[0].value.type);
    BYTES_EQUAL(TL_VALUE_RECORD_ID, result[1].value.type);
}


TEST(TemplateLanguage,
     parsingIfRhsWithRecordId_returns2ndTokenWithValueRecordId) {
    tl_formatTokenT *result = tl_parseFormat("{?&5==#}{;}");
    BYTES_EQUAL(5, result[0].value.number);
    BYTES_EQUAL(0, result[1].value.number);
}


// IF RHS = LITERAL VALUE


TEST(TemplateLanguage,
     parsingIfRhsWithLiteralValue_returns2ndTokenWithTypeIF_RHS) {
    tl_formatTokenT *result = tl_parseFormat("{?&8==67}x{;}");
    BYTES_EQUAL(TL_IF_LHS, result[0].type);
    BYTES_EQUAL(TL_IF_RHS, result[1].type);
    BYTES_EQUAL(TL_TEXT, result[2].type);
}


TEST(TemplateLanguage,
     parsingIfRhsWithLiteralValue_returns2ndTokenWithValueTypeLITERAL_VALUE) {
    tl_formatTokenT *result = tl_parseFormat("{?&2==13}{;}");
    BYTES_EQUAL(TL_VALUE_COLUMN_ID, result[0].value.type);
    BYTES_EQUAL(TL_VALUE_LITERAL, result[1].value.type);
}


TEST(TemplateLanguage,
     parsingIfRhsWithLiteralValue_returns2ndTokenWithLiteralValue) {
    tl_formatTokenT *result = tl_parseFormat("{?&2==123}{;}");
    BYTES_EQUAL(2, result[0].value.number);
    BYTES_EQUAL(123, result[1].value.number);
}


// OPERATORS


TEST(TemplateLanguage,
     parsingIfEqualsLiteral_returnsRhsWithOperatorEQUAL) {
    tl_formatTokenT *result = tl_parseFormat("{?&1==23}{;}");
    BYTES_EQUAL(OU_EQUAL, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value.number);
}


TEST(TemplateLanguage,
     parsingIfEqualsColumnId_returnsRhsWithOperatorEQUAL) {
    tl_formatTokenT *result = tl_parseFormat("{?&1==&23}{;}");
    BYTES_EQUAL(OU_EQUAL, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value.number);
}


TEST(TemplateLanguage,
     parsingIfEqualsRecordId_returnsRhsWithOperatorEQUAL) {
    tl_formatTokenT *result = tl_parseFormat("{?&1==#}{;}");
    BYTES_EQUAL(OU_EQUAL, result[1].rhs.ifOperator);
}


TEST(TemplateLanguage,
     parsingIfNotEqual_returnsRhsWithOperatorNOT_EQUAL) {
    tl_formatTokenT *result = tl_parseFormat("{?&1!=23}{;}");
    BYTES_EQUAL(OU_NOT_EQUAL, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value.number);
}


TEST(TemplateLanguage,
     parsingIfLessOrEqual_returnsRhsWithOperatorLESS_OR_EQUAL) {
    tl_formatTokenT *result = tl_parseFormat("{?&1<=23}{;}");
    BYTES_EQUAL(OU_LESS_THAN_OR_EQUAL, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value.number);
}


TEST(TemplateLanguage,
     parsingIfGreaterOrEqual_returnsRhsWithOperatorGREATER_OR_EQUAL) {
    tl_formatTokenT *result = tl_parseFormat("{?&1>=23}{;}");
    BYTES_EQUAL(OU_GREATER_THAN_OR_EQUAL, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value.number);
}


TEST(TemplateLanguage,
     parsingIfLess_returnsRhsWithOperatorLESS_THAN) {
    tl_formatTokenT *result = tl_parseFormat("{?&1<23}{;}");
    BYTES_EQUAL(OU_LESS_THAN, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value.number);
}


TEST(TemplateLanguage,
     parsingIfGreater_returnsRhsWithOperatorGREATER_THAN) {
    tl_formatTokenT *result = tl_parseFormat("{?&1>23}{;}");
    BYTES_EQUAL(OU_GREATER_THAN, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value.number);
}


// ENDIF


TEST(TemplateLanguage,
     parsingIfWithEndIf_returnsLhsWithElseIndex0) {
    tl_formatTokenT *result = tl_parseFormat("{?&1>23}txt{;}");
    BYTES_EQUAL(0, result[0].lhs.elseIndex);
}


TEST(TemplateLanguage,
     parsingIfWithEndIf_returnsLhsWithEndIndexPointingToNextToken) {
    tl_formatTokenT *result = tl_parseFormat("{?&1>23}txt{;}next");
    BYTES_EQUAL(0, result[0].lhs.elseIndex);
    BYTES_EQUAL(3, result[0].lhs.endIndex);
}


TEST(TemplateLanguage,
     parsingIfWithEndIf_whereIfIsNot1stToken_returnsLhsWhereEndIndexIsNextToken) {
    tl_formatTokenT *result = tl_parseFormat("start{?&1>23}txt{;}");
    BYTES_EQUAL(0, result[1].lhs.elseIndex);
    BYTES_EQUAL(4, result[1].lhs.endIndex);
}


TEST(TemplateLanguage,
     parsingIfWithEndIf_withOtherTokenInside_returnsLhsWhereEndIndexIsNextToken) {
    tl_formatTokenT *result = tl_parseFormat("{?&1>23}{&3}{;}");
    BYTES_EQUAL(0, result[0].lhs.elseIndex);
    BYTES_EQUAL(3, result[0].lhs.endIndex);
}


TEST(TemplateLanguage,
     parsingTwoConsecutiveIfs_returnsValidResult) {
    tl_formatTokenT *result = tl_parseFormat("a{?&1<9}b{;}c{?&2>0}d{;}e");
    // 1st if:
    BYTES_EQUAL(TL_IF_LHS, result[1].type);
    BYTES_EQUAL(0, result[1].lhs.elseIndex);
    BYTES_EQUAL(4, result[1].lhs.endIndex);
    // 2nd if:
    BYTES_EQUAL(TL_IF_LHS, result[5].type);
    BYTES_EQUAL(0, result[5].lhs.elseIndex);
    BYTES_EQUAL(8, result[5].lhs.endIndex);
}


// ELSE


TEST(TemplateLanguage,
     parsingIfWithElse_returnsLhsWithElseIndexAndEndIndex) {
    tl_formatTokenT *result = tl_parseFormat("{?&1>0}a{:}b{;}");
    BYTES_EQUAL(3, result[0].lhs.elseIndex);
    BYTES_EQUAL(4, result[0].lhs.endIndex);
}


TEST(TemplateLanguage,
     parsingIfWithElse_withTokensInside_worksAsExpected) {
    tl_formatTokenT *result = tl_parseFormat("a{?&1>0}b{*}c{:}d{#}e{;}");
    BYTES_EQUAL(6, result[1].lhs.elseIndex);
    BYTES_EQUAL(9, result[1].lhs.endIndex);
}


TEST(TemplateLanguage,
     parsingTwoConsecutiveIfWithElse_returnsValidResult) {
    const char *str = "a{?&1<9}b{:}c{;}d{?&2>0}e{:}f{;}g";
    tl_formatTokenT *result = tl_parseFormat(str);
    // 1st if:
    BYTES_EQUAL(TL_IF_LHS, result[1].type);
    BYTES_EQUAL(4, result[1].lhs.elseIndex);
    BYTES_EQUAL(5, result[1].lhs.endIndex);
    // 2nd if:
    BYTES_EQUAL(TL_IF_LHS, result[6].type);
    BYTES_EQUAL(9, result[6].lhs.elseIndex);
    BYTES_EQUAL(10, result[6].lhs.endIndex);
}


////


TEST_GROUP(FormatString) {
    uint8_t tokenNo;

    void setup() {
    }

    void teardown() {
    }

    bool isTokenEqual(tl_formatTokenT *token1,
                      tl_formatTokenT *token2) {
        const tl_tokenTypeT tokenType = token1->type;
        BYTES_EQUAL_TEXT(tokenType,
                         token2->type,
                         StringFromFormat("tokenNo = %d",
                                          tokenNo).asCharString());
        switch (tokenType) {
            case TL_TEXT :
                return isTxtTokenEqual(token1, token2);
            case TL_VALUE :
                return isValueTokenEqual(token1, token2);
            case TL_IF_LHS :
                return isLhsTokenEqual(token1, token2);
            case TL_IF_RHS :
                return isRhsTokenEqual(token1, token2);
            default :
                return false;
        }
    }

    bool isTxtTokenEqual(tl_formatTokenT *token1,
                         tl_formatTokenT *token2) {
        BYTES_EQUAL_TEXT(token1->text.start,
                         token2->text.start,
                         StringFromFormat("text.start @ tokenNo = %d",
                                          tokenNo).asCharString());
        BYTES_EQUAL_TEXT(token1->text.length,
                         token2->text.length,
                         StringFromFormat("text.length @ tokenNo = %d",
                                          tokenNo).asCharString());
        return false;
    }

    bool isValueTokenEqual(tl_formatTokenT *token1,
                           tl_formatTokenT *token2) {
        BYTES_EQUAL_TEXT(token1->value.type,
                         token2->value.type,
                         StringFromFormat("value.type @ tokenNo = %d",
                                          tokenNo).asCharString());
        BYTES_EQUAL_TEXT(token1->value.number,
                         token2->value.number,
                         StringFromFormat("value.number @ tokenNo = %d",
                                          tokenNo).asCharString());
        BYTES_EQUAL_TEXT(token1->value.leading0,
                         token2->value.leading0,
                         StringFromFormat("value.leading0 @ tokenNo = %d",
                                          tokenNo).asCharString());
        BYTES_EQUAL_TEXT(token1->value.offset1,
                         token2->value.offset1,
                         StringFromFormat("value.offset1 @ tokenNo = %d",
                                          tokenNo).asCharString());
        BYTES_EQUAL_TEXT(token1->value.asHex,
                         token2->value.asHex,
                         StringFromFormat("value.asHex @ tokenNo = %d",
                                          tokenNo).asCharString());
        return false;
    }

    bool isLhsTokenEqual(tl_formatTokenT *token1,
                         tl_formatTokenT *token2) {
        isValueTokenEqual(token1, token2);
        BYTES_EQUAL_TEXT(token1->lhs.elseIndex,
                         token2->lhs.elseIndex,
                         StringFromFormat("lhs.elseIndex @ tokenNo = %d",
                                          tokenNo).asCharString());
        BYTES_EQUAL_TEXT(token1->lhs.endIndex,
                         token2->lhs.endIndex,
                         StringFromFormat("lhs.endIndex @ tokenNo = %d",
                                          tokenNo).asCharString());
        return false;
    }

    bool isRhsTokenEqual(tl_formatTokenT *token1,
                         tl_formatTokenT *token2) {
        isValueTokenEqual(token1, token2);
        BYTES_EQUAL_TEXT(token1->rhs.ifOperator,
                         token2->rhs.ifOperator,
                         StringFromFormat("rhs.ifOperator @ tokenNo = %d",
                                          tokenNo).asCharString());
        return false;
    }
};


TEST(FormatString,
     RealisticFormat1) {
    static const char *fmtString = " {&1} {?&2==0}Light{:}F{o&2}{;} {&3}\0";
    static tl_formatTokenT fmtTokens[10] = {
        [0] = {
            .type = TL_TEXT,
            .text.start = 0,
            .text.length = 1,
        },
        [1] = {
            .type = TL_VALUE,
            .value.type = TL_VALUE_COLUMN_ID,
            .value.number = 1,
        },
        [2] = {
            .type = TL_TEXT,
            .text.start = 5,
            .text.length = 1,
        },
        [3] = {
            .type = TL_IF_LHS,
            .lhs.elseIndex = 6,
            .lhs.endIndex = 8,
            .value.type = TL_VALUE_COLUMN_ID,
            .value.number = 2,
        },
        [4] = {
            .type = TL_IF_RHS,
            .rhs.ifOperator = OU_EQUAL,
            .value.type = TL_VALUE_LITERAL,
            .value.number = 0,
        },
        [5] = {
            .type = TL_TEXT,
            .text.start = 14,
            .text.length = 5,
        },
        [6] = {
            .type = TL_TEXT,
            .text.start = 22,
            .text.length = 1,
        },
        [7] = {
            .type = TL_VALUE,
            .value.type = TL_VALUE_COLUMN_ID,
            .value.number = 2,
            .value.leading0 = true,
        },
        [8] = {
            .type = TL_TEXT,
            .text.start = 31,
            .text.length = 1,
        },
        [9] = {
            .type = TL_VALUE,
            .value.type = TL_VALUE_COLUMN_ID,
            .value.number = 3,
        },

    };
    tl_formatTokenT *result = tl_parseFormat(fmtString);
    for (tokenNo = 0; tokenNo < 10; tokenNo++) {
        isTokenEqual(&fmtTokens[tokenNo], &result[tokenNo]);
    }
}

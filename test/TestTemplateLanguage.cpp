/* TestTemplateLanguage.cpp
 *
 * TODO:
 *  - alle formatstrings moeten een keer aangeroepen zijn om MAX_NUM_TOKENS te
 *      checken
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/UtestMacros.h"
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


// SYNTAX_ERRORS: TEXT


TEST(TemplateLanguage,
      parsingTextWithStrayClosingBrace_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("abc}def"));
}


TEST(TemplateLanguage,
      parsingTextWithlastClosingBrace_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("abcdef}"));
}


// SYNTAX ERRORS: IF


TEST(TemplateLanguage,
     parsingIf_withNestedIfInTrueSection_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{?&1>23}b{?&3}c{:}d{;}e"));
}


TEST(TemplateLanguage,
     parsingIf_withNestedIfInFalseSection_triggersAssert) {
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


// SYNTAX ERRORS: ELSE


TEST(TemplateLanguage,
     parsingElseWithoutIf_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{:}b"));
}


TEST(TemplateLanguage,
      parsingIf_withPostfixInElseTag_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1>23}a{:+}b{;}C"));
}


TEST(TemplateLanguage,
      parsingIf_withPrefixInElseTag_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1>23}a{o:}b{;}C"));
}


// SYNTAX ERRORS: ENDIF


TEST(TemplateLanguage,
     parsingEndIfWithoutIf_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{;}b"));
}


TEST(TemplateLanguage,
     parsingIfWithElseWithoutEndif_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("a{?&1>23}b{:}c"));
}


TEST(TemplateLanguage,
      parsingIf_withPostfixInEndIfTag_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1>23}a{:}b{;+}C"));
}


TEST(TemplateLanguage,
      parsingIf_withPrefixInEndIfTag_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1>23}a{:}b{o;}C"));
}


// SYNTAX ERRORS: RHS


TEST(TemplateLanguage,
      parsingIf_withInvalidTagInRhs_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==^2}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==*2}"));
}


TEST(TemplateLanguage,
      parsingIf_withPlusPostfixInRhsLiteral_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==2+}"));
}


TEST(TemplateLanguage,
      parsingIf_withAnyPrefixInRhsLiteral_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==o12}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==x12}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==h12}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==f12}"));
}


TEST(TemplateLanguage,
      parsingIf_withAnyPrefixInRhsRecordId_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==o#12}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==x#12}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==h#12}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==f#12}"));
}


TEST(TemplateLanguage,
      parsingIf_withAnyPrefixInRhsColumnIdId_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==o&12}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==x&12}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==h&12}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{?&1==f&12}"));
}


// SYNTAX ERRORS: numRecords


TEST(TemplateLanguage,
     parsingNumRecords_withPostfixPlus_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{*+}"));
}


TEST(TemplateLanguage,
     parsingNumRecords_withAnyPrefixOtherThanLeading0_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{x*}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{h*}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{f*}"));
}


// SYNTAX ERRORS: recordId


TEST(TemplateLanguage,
      parsingRecordId_withAnyPrefixOtherThanLeading0_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{x#}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{h#}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{f#}"));
}


// SYNTAX ERRORS: parentColumnId


TEST(TemplateLanguage,
      parsingParentColumnId_withAnyPrefix_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{o^}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{x^}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{h^}"));
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{f^}"));
}


TEST(TemplateLanguage,
     parsingParentColumnId_withPostfixPlus_triggersAssert) {
    CHECK_THROWS(AssertionFailure, tl_parseFormat("{^+}"));
}



/*
 * TODO: invalid:

 &# #& *& &* #* *# (also ^)

 * TODO valid ones:

 o#+
 o*

*/


// TEXT


TEST(TemplateLanguage,
     parsing255charString_succeeds) {
    char str[256];
    memset(str, 'A', 255);
    str[255] = '\0';
    tl_tokenT *result = tl_parseFormat(str);
    BYTES_EQUAL(tl_text, result[0].type);
    BYTES_EQUAL(255, result[0].text.length);
}


TEST(TemplateLanguage,
     parsingPlainText_returnsTokenWithPlainText) {
    tl_tokenT *result = tl_parseFormat("text:*&");
    BYTES_EQUAL(0, result[0].text.start);
    BYTES_EQUAL(7, result[0].text.length);
}


TEST(TemplateLanguage,
     parsingPlainText_returnsTokenWithTokenTypeTEXT) {
    tl_tokenT *result = tl_parseFormat("text:*&");
    BYTES_EQUAL(tl_text, result[0].type);
}


TEST(TemplateLanguage,
     parsingPlainTextAfterColumnId_returnsTokenWithPlainText_andTokenTypeTEXT) {
    tl_tokenT *result = tl_parseFormat("{&3}TXT");
    BYTES_EQUAL(4, result[1].text.start);
    BYTES_EQUAL(3, result[1].text.length);
    BYTES_EQUAL(tl_text, result[1].type);
}


TEST(TemplateLanguage,
     parsingPlainTextBeforeColumnId_returnsTokenWithPlainText_andTokenTypeTEXT) {
    tl_tokenT *result = tl_parseFormat("@#{&8}");
    BYTES_EQUAL(0, result[0].text.start);
    BYTES_EQUAL(2, result[0].text.length);
    BYTES_EQUAL(tl_text, result[0].type);
}


// COLUMN_ID


TEST(TemplateLanguage,
     parsingColumnId0_returnsTokenWithColumnId0) {
    tl_tokenT *result = tl_parseFormat("{&0}");
    BYTES_EQUAL(0, result[0].value);
}


TEST(TemplateLanguage,
     parsingColumnId_returnsTokenWithTokenTypeVALUE) {
    tl_tokenT *result = tl_parseFormat("{&0}");
    BYTES_EQUAL(tl_value, result[0].type);
}


TEST(TemplateLanguage,
     parsingColumnId_returnsTokenWithTokenValueTypeCOLUMN_ID) {
    tl_tokenT *result = tl_parseFormat("{&0}");
    BYTES_EQUAL(tl_value_column_id, result[0].valType);
}


TEST(TemplateLanguage,
     parsingSingleToken_setsSecondTokenToTypeINVALID) {
    tl_tokenT *result = tl_parseFormat("{&5}");
    BYTES_EQUAL(tl_invalid, result[1].type);
}


TEST(TemplateLanguage,
     parsing2valueTokens_sets3rdTokenToTypeINVALID) {
    tl_tokenT *result = tl_parseFormat("{&5}{&2}");
    BYTES_EQUAL(tl_invalid, result[2].type);
}


TEST(TemplateLanguage,
     parsing2ndTokenAsTxt_sets3rdTokenToTypeINVALID) {
    tl_tokenT *result = tl_parseFormat("{&5}txt");
    BYTES_EQUAL(tl_invalid, result[2].type);
}


TEST(TemplateLanguage,
     parsingMaxMinus1tokens_setsLastPlus1TokenToTypeINVALID) {
    const char *str = "t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t";
    tl_tokenT *result = tl_parseFormat(str);
    BYTES_EQUAL(tl_invalid, result[19].type);
}


TEST(TemplateLanguage,
     parsingMaxNumTokens_setsLastTokenToTypetl_value) {
    const char *str = "t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&5}";
    tl_tokenT *result = tl_parseFormat(str);
    BYTES_EQUAL(5, result[19].value);
    BYTES_EQUAL(tl_value, result[19].type);
}


TEST(TemplateLanguage,
     parsingMaxNumTokens_setsLastTokenToTypetl_text) {
    const char *str = "{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&0}t{&5}t";
    tl_tokenT *result = tl_parseFormat(str);
    BYTES_EQUAL(49, result[19].text.start);
    BYTES_EQUAL(tl_text, result[19].type);
}


TEST(TemplateLanguage,
     parsingColumnId1_returnsTokenWithColumnId1) {
    tl_tokenT *result = tl_parseFormat("{&1}");
    BYTES_EQUAL(1, result[0].value);
}


TEST(TemplateLanguage,
     parsingColumnId255_returnsTokenWithColumnId255) {
    tl_tokenT *result = tl_parseFormat("{&255}");
    BYTES_EQUAL(255, result[0].value);
}


TEST(TemplateLanguage,
     parsingTwoColumns_returnsArrayWithTwoTokens) {
    tl_tokenT *result = tl_parseFormat("{&5}{&2}");
    BYTES_EQUAL(5, result[0].value);
    BYTES_EQUAL(2, result[1].value);
}


TEST(TemplateLanguage,
     parsingColumnId_PlainText_ColumnId_returnsVALUEasLastTokenType) {
    tl_tokenT *result = tl_parseFormat("{&3}@#{&8}");
    BYTES_EQUAL(tl_value, result[2].type);
}


TEST(TemplateLanguage,
     parsingColumnId_withLeading0_returnsTokenValueWithLeading0) {
    tl_tokenT *result = tl_parseFormat("{o&1}");
    CHECK_TRUE(result[0].properties.leading0);
}


TEST(TemplateLanguage,
     parsingColumnId_withoutLeading0_afterOneWith_returnsTokenWithoutLeading0) {
    tl_tokenT *result = tl_parseFormat("{o&1}");
    CHECK_TRUE(result[0].properties.leading0);
    result = tl_parseFormat("{&2}");
    CHECK_FALSE(result[0].properties.leading0);
}


TEST(TemplateLanguage,
     parsingColumnId_withHex_returnsTokenValueWithHex) {
    tl_tokenT *result = tl_parseFormat("{x&1}");
    CHECK_TRUE(result[0].properties.asHex);
}


TEST(TemplateLanguage,
     parsingColumnId_withoutHex_afterOneWith_returnsTokenValueWithoutHex) {
    tl_tokenT *result = tl_parseFormat("{x&2}");
    CHECK_TRUE(result[0].properties.asHex);
    result = tl_parseFormat("{&3}");
    CHECK_FALSE(result[0].properties.asHex);
}


TEST(TemplateLanguage,
     parsingColumnId_withOffset_returnsTokenValueWithOffset) {
    tl_tokenT *result = tl_parseFormat("{&1+}");
    CHECK_TRUE(result[0].properties.offset1);
}


TEST(TemplateLanguage,
     parsingColumnId_withoutOffset_afterOneWith_returnsTokenValueWithoutOffset) {
    tl_tokenT *result = tl_parseFormat("{&2+}");
    CHECK_TRUE(result[0].properties.offset1);
    result = tl_parseFormat("{&3}");
    CHECK_FALSE(result[0].properties.offset1);
}


// RECORD_ID


TEST(TemplateLanguage,
     parsingRecordId_returnsTokenWithTypeVALUE) {
    tl_tokenT *result = tl_parseFormat("{#}");
    BYTES_EQUAL(tl_value, result[0].type);
}


TEST(TemplateLanguage,
     parsingRecordId_returnsTokenValueWithTypeRECORD_ID) {
    tl_tokenT *result = tl_parseFormat("{#}");
    BYTES_EQUAL(tl_value_record_id, result[0].valType);
}


TEST(TemplateLanguage,
     parsingRecordId_withOffset_returnsTokenValueWithOffset) {
    tl_tokenT *result = tl_parseFormat("{#+}");
    CHECK_TRUE(result[0].properties.offset1);
}


TEST(TemplateLanguage,
     parsingRecordId_withoutOffset_afterOneWith_returnsTokenValueWithoutOffset) {
    tl_tokenT *result = tl_parseFormat("{#+}");
    CHECK_TRUE(result[0].properties.offset1);
    result = tl_parseFormat("{#}");
    CHECK_FALSE(result[0].properties.offset1);
}


TEST(TemplateLanguage,
     parsingRecordId_withLeading0_returnsTokenValueWithLeading0) {
    tl_tokenT *result = tl_parseFormat("{o#}");
    CHECK_TRUE(result[0].properties.leading0);
}


TEST(TemplateLanguage,
     parsingRecordId_withPrefixLeading0_andPostfixPlus_isValid) {
    tl_tokenT *result = tl_parseFormat("{o#+}");
    BYTES_EQUAL(tl_value_record_id, result[0].valType);
    CHECK_TRUE(result[0].properties.leading0);
    CHECK_TRUE(result[0].properties.offset1);
}


// NUM_RECORDS


TEST(TemplateLanguage,
     parsingNumRecords_returnsTokenWithTypeVALUE) {
    tl_tokenT *result = tl_parseFormat("{*}");
    BYTES_EQUAL(tl_value, result[0].type);
}


TEST(TemplateLanguage,
     parsingNumRecords_returnsTokenValueWithTypeNUM_RECORDS) {
    tl_tokenT *result = tl_parseFormat("{*}");
    BYTES_EQUAL(tl_value_num_records, result[0].valType);
}


TEST(TemplateLanguage,
     parsingNumRecords_withLeading0_returnsTokenValueWithLeading0) {
    tl_tokenT *result = tl_parseFormat("{o*}");
    CHECK_TRUE(result[0].properties.leading0);
}


// PARENT


TEST(TemplateLanguage,
     parsingParent_returnsTokenWithTypeVALUE) {
    tl_tokenT *result = tl_parseFormat("{^1}");
    BYTES_EQUAL(tl_value, result[0].type);
}


TEST(TemplateLanguage,
     parsingParentRecordId_returnsTokenValueWithTypePARENT_COLUMN_ID) {
    tl_tokenT *result = tl_parseFormat("{^0}");
    BYTES_EQUAL(tl_value_parent_column_id, result[0].valType);
}


TEST(TemplateLanguage,
     parsingParentRecordId_returnsTokenValue) {
    tl_tokenT *result = tl_parseFormat("{^8}");
    BYTES_EQUAL(8, result[0].value);
}


// IF LHS


TEST(TemplateLanguage,
     parsingIfLhs_returnsTokenWithTypeIF_LHS) {
    tl_tokenT *result = tl_parseFormat("{?&1}{;}");
    BYTES_EQUAL(tl_if_lhs, result[0].type);
}


TEST(TemplateLanguage,
     parsingIfLhs_returnsTokenWithValueTypeCOLUMN_ID) {
    tl_tokenT *result = tl_parseFormat("{?&1}{;}");
    BYTES_EQUAL(tl_value_column_id, result[0].valType);
}


TEST(TemplateLanguage,
     parsingIfLhs_returnsTokenWithValueColumnId) {
    tl_tokenT *result = tl_parseFormat("{?&12}{;}");
    BYTES_EQUAL(12, result[0].value);
}


TEST(TemplateLanguage,
     parsingIfLhs_followedByTxt_returns2nsTokenTxt) {
    tl_tokenT *result = tl_parseFormat("{?&12}next{;}");
    BYTES_EQUAL(tl_text, result[1].type);
    BYTES_EQUAL(6, result[1].text.start);
}


// IF RHS = COLUMN_ID


TEST(TemplateLanguage,
     parsingIfRhsWithColumnId_returns2ndTokenWithTypeIF_RHS) {
    tl_tokenT *result = tl_parseFormat("{?&8==&1}x{;}");
    BYTES_EQUAL(tl_if_lhs, result[0].type);
    BYTES_EQUAL(tl_if_rhs, result[1].type);
    BYTES_EQUAL(tl_text, result[2].type);
}


TEST(TemplateLanguage,
     parsingIfRhsWithColumnId_returns2ndTokenWithValueTypeCOLUMN_ID) {
    tl_tokenT *result = tl_parseFormat("{?&2==&3}{;}");
    BYTES_EQUAL(tl_value_column_id, result[0].valType);
    BYTES_EQUAL(tl_value_column_id, result[1].valType);
}


TEST(TemplateLanguage,
     parsingIfRhsWithColumnId_returns2ndTokenWithValueColumnId) {
    tl_tokenT *result = tl_parseFormat("{?&4==&5}{;}");
    BYTES_EQUAL(4, result[0].value);
    BYTES_EQUAL(5, result[1].value);
}


TEST(TemplateLanguage,
     parsingIf_withPlusPostfixInRhsColumnId_isValid) {
    tl_tokenT *result = tl_parseFormat("{?&1==&2+} {;}");
    BYTES_EQUAL(tl_value_column_id, result[1].valType);
    CHECK_TRUE(result[1].properties.offset1);
}


// IF RHS = RECORD_ID


TEST(TemplateLanguage,
     parsingIfRhsWithRecordId_returns2ndTokenWithTypeIF_RHS) {
    tl_tokenT *result = tl_parseFormat("{?&8==#}x{;}");
    BYTES_EQUAL(tl_if_lhs, result[0].type);
    BYTES_EQUAL(tl_if_rhs, result[1].type);
    BYTES_EQUAL(tl_text, result[2].type);
}


TEST(TemplateLanguage,
     parsingIfRhsWithRecordId_returns2ndTokenWithValueTypeRECORD_ID) {
    tl_tokenT *result = tl_parseFormat("{?&2==#}{;}");
    BYTES_EQUAL(tl_value_column_id, result[0].valType);
    BYTES_EQUAL(tl_value_record_id, result[1].valType);
}


TEST(TemplateLanguage,
     parsingIfRhsWithRecordId_returns2ndTokenWithValueRecordId) {
    tl_tokenT *result = tl_parseFormat("{?&5==#}{;}");
    BYTES_EQUAL(5, result[0].value);
    BYTES_EQUAL(0, result[1].value);
}


TEST(TemplateLanguage,
     parsingIf_withPlusPostfixInRhsRecordId_isValid) {
    tl_tokenT *result = tl_parseFormat("{?&1==#+} {;}");
    BYTES_EQUAL(tl_value_record_id, result[1].valType);
    CHECK_TRUE(result[1].properties.offset1);
}


// IF RHS = LITERAL VALUE


TEST(TemplateLanguage,
     parsingIfRhsWithLiteralValue_returns2ndTokenWithTypeIF_RHS) {
    tl_tokenT *result = tl_parseFormat("{?&8==67}x{;}");
    BYTES_EQUAL(tl_if_lhs, result[0].type);
    BYTES_EQUAL(tl_if_rhs, result[1].type);
    BYTES_EQUAL(tl_text, result[2].type);
}


TEST(TemplateLanguage,
     parsingIfRhsWithLiteralValue_returns2ndTokenWithValueTypeLITERAL_VALUE) {
    tl_tokenT *result = tl_parseFormat("{?&2==13}{;}");
    BYTES_EQUAL(tl_value_column_id, result[0].valType);
    BYTES_EQUAL(tl_value_literal, result[1].valType);
}


TEST(TemplateLanguage,
     parsingIfRhsWithLiteralValue_returns2ndTokenWithLiteralValue) {
    tl_tokenT *result = tl_parseFormat("{?&2==123}{;}");
    BYTES_EQUAL(2, result[0].value);
    BYTES_EQUAL(123, result[1].value);
}


// OPERATORS


TEST(TemplateLanguage,
     parsingIfEqualsLiteral_returnsRhsWithOperatorEQUAL) {
    tl_tokenT *result = tl_parseFormat("{?&1==23}{;}");
    BYTES_EQUAL(ou_equal, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value);
}


TEST(TemplateLanguage,
     parsingIfEqualsColumnId_returnsRhsWithOperatorEQUAL) {
    tl_tokenT *result = tl_parseFormat("{?&1==&23}{;}");
    BYTES_EQUAL(ou_equal, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value);
}


TEST(TemplateLanguage,
     parsingIfEqualsRecordId_returnsRhsWithOperatorEQUAL) {
    tl_tokenT *result = tl_parseFormat("{?&1==#}{;}");
    BYTES_EQUAL(ou_equal, result[1].rhs.ifOperator);
}


TEST(TemplateLanguage,
     parsingIfNotEqual_returnsRhsWithOperatorNOT_EQUAL) {
    tl_tokenT *result = tl_parseFormat("{?&1!=23}{;}");
    BYTES_EQUAL(ou_not_equal, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value);
}


TEST(TemplateLanguage,
     parsingIfLessOrEqual_returnsRhsWithOperatorLESS_OR_EQUAL) {
    tl_tokenT *result = tl_parseFormat("{?&1<=23}{;}");
    BYTES_EQUAL(ou_less_than_or_equal, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value);
}


TEST(TemplateLanguage,
     parsingIfGreaterOrEqual_returnsRhsWithOperatorGREATER_OR_EQUAL) {
    tl_tokenT *result = tl_parseFormat("{?&1>=23}{;}");
    BYTES_EQUAL(ou_greater_than_or_equal, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value);
}


TEST(TemplateLanguage,
     parsingIfLess_returnsRhsWithOperatorLESS_THAN) {
    tl_tokenT *result = tl_parseFormat("{?&1<23}{;}");
    BYTES_EQUAL(ou_less_than, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value);
}


TEST(TemplateLanguage,
     parsingIfGreater_returnsRhsWithOperatorGREATER_THAN) {
    tl_tokenT *result = tl_parseFormat("{?&1>23}{;}");
    BYTES_EQUAL(ou_greater_than, result[1].rhs.ifOperator);
    BYTES_EQUAL(23, result[1].value);
}


// ENDIF


TEST(TemplateLanguage,
     parsingIfWithEndIf_returnsLhsWithEndIndexPointingToNextToken) {
    tl_tokenT *result = tl_parseFormat("{?&1>23}txt{;}next");
    BYTES_EQUAL(3, result[0].lhs.endIndex);
}


TEST(TemplateLanguage,
     parsingIfWithEndIf_returnsLhsWithElseIndexSameAsEndIndex) {
    tl_tokenT *result = tl_parseFormat("{?&1>23}txt{;}");
    BYTES_EQUAL(3, result[0].lhs.endIndex);
    BYTES_EQUAL(3, result[0].lhs.elseIndex);
}


TEST(TemplateLanguage,
     parsingIfWithEndIf_whereIfIsNot1stToken_returnsLhsWhereEndIndexIsNextToken) {
    tl_tokenT *result = tl_parseFormat("start{?&1>23}txt{;}");
    BYTES_EQUAL(4, result[1].lhs.elseIndex);
    BYTES_EQUAL(4, result[1].lhs.endIndex);
}


TEST(TemplateLanguage,
     parsingIfWithEndIf_withOtherTokenInside_returnsLhsWhereEndIndexIsNextToken) {
    tl_tokenT *result = tl_parseFormat("{?&1>23}{&3}{;}");
    BYTES_EQUAL(3, result[0].lhs.elseIndex);
    BYTES_EQUAL(3, result[0].lhs.endIndex);
}


TEST(TemplateLanguage,
     parsingTwoConsecutiveIfs_returnsValidResult) {
    tl_tokenT *result = tl_parseFormat("a{?&1<9}b{;}c{?&2>0}d{;}e");
    // 1st if:
    BYTES_EQUAL(tl_if_lhs, result[1].type);
    BYTES_EQUAL(4, result[1].lhs.elseIndex);
    BYTES_EQUAL(4, result[1].lhs.endIndex);
    // 2nd if:
    BYTES_EQUAL(tl_if_lhs, result[5].type);
    BYTES_EQUAL(8, result[5].lhs.elseIndex);
    BYTES_EQUAL(8, result[5].lhs.endIndex);
}


// ELSE


TEST(TemplateLanguage,
     parsingIfWithElse_returnsLhsWithElseIndexAndEndIndex) {
    tl_tokenT *result = tl_parseFormat("{?&1>0}a{:}b{;}");
    BYTES_EQUAL(3, result[0].lhs.elseIndex);
    BYTES_EQUAL(4, result[0].lhs.endIndex);
}


TEST(TemplateLanguage,
     parsingIfWithElse_withTokensInside_worksAsExpected) {
    tl_tokenT *result = tl_parseFormat("a{?&1>0}b{*}c{:}d{#}e{;}");
    BYTES_EQUAL(6, result[1].lhs.elseIndex);
    BYTES_EQUAL(9, result[1].lhs.endIndex);
}


TEST(TemplateLanguage,
     parsingTwoConsecutiveIfWithElse_returnsValidResult) {
    const char *str = "a{?&1<9}b{:}c{;}d{?&2>0}e{:}f{;}g";
    tl_tokenT *result = tl_parseFormat(str);
    // 1st if:
    BYTES_EQUAL(tl_if_lhs, result[1].type);
    BYTES_EQUAL(4, result[1].lhs.elseIndex);
    BYTES_EQUAL(5, result[1].lhs.endIndex);
    // 2nd if:
    BYTES_EQUAL(tl_if_lhs, result[6].type);
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

    bool isTokenEqual(tl_tokenT *token1,
                      tl_tokenT *token2) {
        const tl_tokenTypeT tokenType = token1->type;
        BYTES_EQUAL_TEXT(tokenType,
                         token2->type,
                         StringFromFormat("tokenType @ tokenNo = %d",
                                          tokenNo).asCharString());
        switch (tokenType) {
            case tl_text :
                return isTxtTokenEqual(token1, token2);
            case tl_value :
                return isValueTokenEqual(token1, token2);
            case tl_if_lhs :
                return isLhsTokenEqual(token1, token2);
            case tl_if_rhs :
                return isRhsTokenEqual(token1, token2);
            default :
                return false;
        }
    }

    bool isTxtTokenEqual(tl_tokenT *token1,
                         tl_tokenT *token2) {
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

    bool isValueTokenEqual(tl_tokenT *token1,
                           tl_tokenT *token2) {
        BYTES_EQUAL_TEXT(token1->valType,
                         token2->valType,
                         StringFromFormat("value.type @ tokenNo = %d",
                                          tokenNo).asCharString());
        BYTES_EQUAL_TEXT(token1->value,
                         token2->value,
                         StringFromFormat("value.number @ tokenNo = %d",
                                          tokenNo).asCharString());
        BYTES_EQUAL_TEXT(token1->properties.leading0,
                         token2->properties.leading0,
                         StringFromFormat("value.properties.leading0 @ tokenNo = %d",
                                          tokenNo).asCharString());
        BYTES_EQUAL_TEXT(token1->properties.offset1,
                         token2->properties.offset1,
                         StringFromFormat("value.properties.offset1 @ tokenNo = %d",
                                          tokenNo).asCharString());
        BYTES_EQUAL_TEXT(token1->properties.asHex,
                         token2->properties.asHex,
                         StringFromFormat("value.properties.asHex @ tokenNo = %d",
                                          tokenNo).asCharString());
        return false;
    }

    bool isLhsTokenEqual(tl_tokenT *token1,
                         tl_tokenT *token2) {
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

    bool isRhsTokenEqual(tl_tokenT *token1,
                         tl_tokenT *token2) {
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
    static tl_tokenT fmtTokens[10] = {
        [0] = {
            .type = tl_text,
            .text.start = 0,
            .text.length = 1,
        },
        [1] = {
            .type = tl_value,
            .valType = tl_value_column_id,
            .value = 1,
        },
        [2] = {
            .type = tl_text,
            .text.start = 5,
            .text.length = 1,
        },
        [3] = {
            .type = tl_if_lhs,
            .lhs.elseIndex = 6,
            .lhs.endIndex = 8,
            .valType = tl_value_column_id,
            .value = 2,
        },
        [4] = {
            .type = tl_if_rhs,
            .rhs.ifOperator = ou_equal,
            .valType = tl_value_literal,
            .value = 0,
        },
        [5] = {
            .type = tl_text,
            .text.start = 14,
            .text.length = 5,
        },
        [6] = {
            .type = tl_text,
            .text.start = 22,
            .text.length = 1,
        },
        [7] = {
            .type = tl_value,
            .valType = tl_value_column_id,
            .value = 2,
            .properties.leading0 = true,
        },
        [8] = {
            .type = tl_text,
            .text.start = 31,
            .text.length = 1,
        },
        [9] = {
            .type = tl_value,
            .valType = tl_value_column_id,
            .value = 3,
        },

    };
    tl_tokenT *result = tl_parseFormat(fmtString);
    for (tokenNo = 0; tokenNo < 10; tokenNo++) {
        isTokenEqual(&fmtTokens[tokenNo], &result[tokenNo]);
    }
}

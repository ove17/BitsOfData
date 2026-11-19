// TestRecordWriter.cpp

#include "cpputestUtils.h"

extern "C" {
    #include "EeHw.h"
    #include "BitsOfData.h"
    #include "SchemaForTesting.h"
    #include "RecordWriter.h"
}

static BDB_recordFieldPropertiesT *fieldProps = NULL;


TEST_GROUP(getRecordFieldProperties) {
    void setup() {
        eeClear();
        BDB_openDataBase(&dbaseDef, NULL);
    }

    void teardown() {
        BDB_closeDataBase();
    }
};


// fieldProps.numFields


TEST(getRecordFieldProperties,
     fieldProps_withoutEditableFields_returnsAnEmptyStruct) {
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{*}b{#}c{^1}d\0");
    BYTES_EQUAL(0, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_with1editableField_returnsNumFields1) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{&0}b\0");
    BYTES_EQUAL(1, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_with1columnId_returns2fields) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{&0}{&0}b\0");
    BYTES_EQUAL(2, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_withHiddenColumnId_returns1field) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{h&0}b\0");
    BYTES_EQUAL(1, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_withFixedColumnId_returns0fields) {
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{f&1}b\0");
    BYTES_EQUAL(0, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_withFixedIf_returns0fields_ifTrue) {
    BDB_setValue(0, 0, 1, 1);
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{?f&1}b{;}\0");
    BYTES_EQUAL(0, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_withIf_returns1field_ifTrue) {
    BDB_setValue(3, 0, 0, 1);
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{?&1}b{:}c{;}\0");
    BYTES_EQUAL(1, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_withColumnIdInTrueIfBranch_returns2fields) {
    BDB_setValue(0, 0, 1, 1);
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{?&1}{&2}{:}c{;}\0");
    BYTES_EQUAL(2, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_withIf_returns0fields_ifFalse) {
    BDB_setValue(0, 0, 1, 0);
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{?&1}b{:}c{;}\0");
    BYTES_EQUAL(0, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_withColumnIdInFalseIfBranch_returns1_ifFalse) {
    BDB_setValue(0, 0, 1, 0);
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{?&1}b{:}{&2}{;}\0");
    BYTES_EQUAL(1, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_withFixedIfRhs_returns0fields_ifTrue) {
    BDB_setValue(0, 0, 1, 0);
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{?f&1==0}{:}{&2}{;}\0");
    BYTES_EQUAL(0, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_withIfRhs_returns1field_ifTrue) {
    BDB_setValue(0, 0, 1, 2);
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{?&1==2}b{:}c{;}\0");
    BYTES_EQUAL(1, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_withColumnIdInTrueIfRhsBranch_returns2fields) {
    BDB_setValue(0, 0, 1, 2);
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{?&1==2}{&2}{:}c{;}\0");
    BYTES_EQUAL(2, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_withIfRhs_returns0fields_ifFalse) {
    BDB_setValue(0, 0, 1, 2);
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{?&1==3}b{:}c{;}\0");
    BYTES_EQUAL(0, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_withColumnIdInFalseIfRhsBranch_returns1_ifFalse) {
    BDB_setValue(0, 0, 1, 2);
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{?&1==1}b{:}{&2}{;}\0");
    BYTES_EQUAL(1, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_onHiddenColumn_returns1field) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{h&1}b\0");
    BYTES_EQUAL(1, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_onIfColumn_returns1field_ifTrue) {
    BDB_setValue(0, 0, 1, 1);
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{?&1}b{:}c{;}\0");
    BYTES_EQUAL(1, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     fieldProps_onIfColumnWithRhsCol_returns1field_ifTrue) {
    BDB_setValue(0, 0, 1, 1);
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{?&1>&2}b{:}c{;}\0");
    BYTES_EQUAL(1, fieldProps->numFields);
}


TEST(getRecordFieldProperties,
     getRecordFieldProps_onFixedColumn_returns0Fields) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{f&7}b\0");
    BYTES_EQUAL(0, fieldProps->numFields);
}


// fieldProps[].columnId


TEST(getRecordFieldProperties,
     getRecordFieldProps_withColumn0as1st_returnsCol0) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{&0}b\0");
    BYTES_EQUAL(0, fieldProps->fields[0].columnId);
}


TEST(getRecordFieldProperties,
     getRecordFieldProps_withColumn15as1st_returnsCol15) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{&15}b\0");
    BYTES_EQUAL(15, fieldProps->fields[0].columnId);
}


TEST(getRecordFieldProperties,
     getRecordFieldProps_withIf_returnsLhsCol) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{?&3>8}b{;}\0");
    BYTES_EQUAL(3, fieldProps->fields[0].columnId);
}


TEST(getRecordFieldProperties,
     getRecordFieldProps_onHiddenColumn_returnsColumnId) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{h&7}b\0");
    BYTES_EQUAL(7, fieldProps->fields[0].columnId);
}


// fieldProps[].editStep


TEST(getRecordFieldProperties,
     getRecordFieldProps_onColumnId_defaultEditStepIs1) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{&0}b\0");
    BYTES_EQUAL(1, fieldProps->fields[0].editStep);
}


TEST(getRecordFieldProperties,
     getRecordFieldProps_onIfColumn_defaultEditStepIs1) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{?&0}b{;}\0");
    BYTES_EQUAL(1, fieldProps->fields[0].editStep);
}


// columnId &i/1, &i/2


TEST(getRecordFieldProperties,
     fieldProps_withoutEditPer_on4digitColumn_returns1field) {
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{&1}b\0");
    BYTES_EQUAL(1, fieldProps->numFields);
    BYTES_EQUAL(1, fieldProps->fields[0].columnId);
    BYTES_EQUAL(1, fieldProps->fields[0].editStep);
}


// note: range table3, column0 is 0-99
TEST(getRecordFieldProperties,
     fieldProps_withEditPer1on2digitColumn_returns2fields) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{&0/1}b\0");
    BYTES_EQUAL(2, fieldProps->numFields);
    BYTES_EQUAL(0, fieldProps->fields[0].columnId);
    BYTES_EQUAL(0, fieldProps->fields[1].columnId);
    BYTES_EQUAL(10, fieldProps->fields[0].editStep);
    BYTES_EQUAL(1, fieldProps->fields[1].editStep);
}


// note: range table3, column1 is 0-200
TEST(getRecordFieldProperties,
     fieldProps_withEditPer1on3digitColumn_returns3fields) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{&1/1}b\0");
    BYTES_EQUAL(3, fieldProps->numFields);
    BYTES_EQUAL(1, fieldProps->fields[0].columnId);
    BYTES_EQUAL(1, fieldProps->fields[1].columnId);
    BYTES_EQUAL(1, fieldProps->fields[2].columnId);
    BYTES_EQUAL(100, fieldProps->fields[0].editStep);
    BYTES_EQUAL(10, fieldProps->fields[1].editStep);
    BYTES_EQUAL(1, fieldProps->fields[2].editStep);
}


// note: range table0, column1 is 0-4000
TEST(getRecordFieldProperties,
     fieldProps_withEditPer1on4digitColumn_returns4fields) {
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{&1/1}b\0");
    BYTES_EQUAL(4, fieldProps->numFields);
    BYTES_EQUAL(1, fieldProps->fields[0].columnId);
    BYTES_EQUAL(1, fieldProps->fields[1].columnId);
    BYTES_EQUAL(1, fieldProps->fields[2].columnId);
    BYTES_EQUAL(1, fieldProps->fields[3].columnId);
    LONGS_EQUAL(1000, fieldProps->fields[0].editStep);
    BYTES_EQUAL(100, fieldProps->fields[1].editStep);
    BYTES_EQUAL(10, fieldProps->fields[2].editStep);
    BYTES_EQUAL(1, fieldProps->fields[3].editStep);
}


// note: range table3, column1 is 0-200
TEST(getRecordFieldProperties,
     fieldProps_withEditPer2on3digitColumn_returns2fields) {
    fieldProps = rw_getRecordFieldProperties(3, 0, "a{&1/2}b\0");
    BYTES_EQUAL(2, fieldProps->numFields);
    BYTES_EQUAL(1, fieldProps->fields[0].columnId);
    BYTES_EQUAL(1, fieldProps->fields[1].columnId);
    BYTES_EQUAL(100, fieldProps->fields[0].editStep);
    BYTES_EQUAL(1, fieldProps->fields[1].editStep);
}


TEST(getRecordFieldProperties,
     fieldProps_withEditPer2on4digitColumn_returns2fields) {
    fieldProps = rw_getRecordFieldProperties(0, 0, "a{&1/2}b\0");
    BYTES_EQUAL(2, fieldProps->numFields);
    BYTES_EQUAL(1, fieldProps->fields[0].columnId);
    BYTES_EQUAL(1, fieldProps->fields[1].columnId);
    BYTES_EQUAL(100, fieldProps->fields[0].editStep);
    BYTES_EQUAL(1, fieldProps->fields[1].editStep);
}


// string


TEST(getRecordFieldProperties,
     fieldProps_ofString_returns1fieldForEachCharacter) {
    fieldProps = rw_getRecordFieldProperties(2, 0, " abc{&7}d\0");
    BYTES_EQUAL(4, fieldProps->numFields);
    BYTES_EQUAL(1, fieldProps->fields[0].columnId);
    BYTES_EQUAL(2, fieldProps->fields[1].columnId);
    BYTES_EQUAL(3, fieldProps->fields[2].columnId);
    BYTES_EQUAL(4, fieldProps->fields[3].columnId);
    BYTES_EQUAL(1, fieldProps->fields[0].editStep);
    BYTES_EQUAL(1, fieldProps->fields[1].editStep);
    BYTES_EQUAL(1, fieldProps->fields[2].editStep);
    BYTES_EQUAL(1, fieldProps->fields[3].editStep);
    BYTES_EQUAL(4, fieldProps->fields[0].cursorPos);
    BYTES_EQUAL(5, fieldProps->fields[1].cursorPos);
    BYTES_EQUAL(6, fieldProps->fields[2].cursorPos);
    BYTES_EQUAL(7, fieldProps->fields[3].cursorPos);
}


/*TODO:
 * getRecordFieldProperties
 *      cursorPos: decimal, enum, if, hidden
 *
 * move BDB_writeRecordWithFormat to rw_writeRecordWithFormat
 *
 * move dbEngine out of BitsOfData
 */



TEST_GROUP(writeRecordWithFormat) {
    void setup() {
        eeClear();
        BDB_openDataBase(&dbaseDef, NULL);
    }

    void teardown() {
        BDB_closeDataBase();
    }
};


TEST(writeRecordWithFormat,
     writeValueOfPercentageReturnsPercentage) {
    BDB_setValue(1, 0, 0, 1); // set recordType
    BDB_setValue(1, 0, 3, 10);
    rw_writeRecordWithFormat(1, 0, "a{&3%}b\n");
    STRNCMP_EQUAL("a 40b", BDB_getWriteBuffer(), 5);
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_writesPlainText) {
    rw_writeRecordWithFormat(0, 0, "text\n");
    STRNCMP_EQUAL("text", BDB_getWriteBuffer(), 4);
}


TEST(writeRecordWithFormat,
     writeRecordWithTrueIf_writesIfTrueText) {
    BDB_setValue(0, 0, 1, 1);
    rw_writeRecordWithFormat(0, 0, "{?&1}true{:}false{;} ");
    STRNCMP_EQUAL("true ", BDB_getWriteBuffer(), 5);
}


TEST(writeRecordWithFormat,
     writeRecordWithFalseIf_writesIfFalseText) {
    BDB_setValue(0, 0, 1, 0);
    rw_writeRecordWithFormat(0, 0, "{?&1}true{:}false{;} ");
    STRNCMP_EQUAL("false ", BDB_getWriteBuffer(), 6);
}


TEST(writeRecordWithFormat,
     writeRecordWithFalseIfCondition_writesIfFalseText) {
    BDB_setValue(0, 0, 1, 0);
    rw_writeRecordWithFormat(0, 0, "{?&1}true{:}false{;} ");
    STRNCMP_EQUAL("false ", BDB_getWriteBuffer(), 5);
}


TEST(writeRecordWithFormat,
     writeRecordWithTrueIfCondition_writesIfTrueText) {
    BDB_setValue(0, 0, 1, 2);
    rw_writeRecordWithFormat(0, 0, "{?&1}true{:}false{;} ");
    STRNCMP_EQUAL("true ", BDB_getWriteBuffer(), 5);
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_withoutElse_writesIfTrueTextWhenTrue) {
    BDB_setValue(0, 0, 1, 1);
    rw_writeRecordWithFormat(0, 0, "{?&1}true{;} ");
    STRNCMP_EQUAL("true ", BDB_getWriteBuffer(), 5);
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_withoutElse_writesNoTextWhenFalse) {
    BDB_setValue(0, 0, 1, 0);
    rw_writeRecordWithFormat(0, 0, "{?&1}true{;} ");
    STRNCMP_EQUAL("     ", BDB_getWriteBuffer(), 5);
}


TEST(writeRecordWithFormat,
     writeRecordWithNumRecordsWithoutLeading0_writesNumRecords) {
    rw_writeRecordWithFormat(1, 0, "{*}\n");
    STRNCMP_EQUAL(" 1", BDB_getWriteBuffer(), 2);
}


TEST(writeRecordWithFormat,
     writeRecordWithNumRecordsWithLeading0_writesNumRecords) {
    rw_writeRecordWithFormat(1, 0, "{o*}\n");
    STRNCMP_EQUAL("01", BDB_getWriteBuffer(), 2);
}


TEST(writeRecordWithFormat,
     writeRecordWithRecordId_withoutLeading0_writesRecordId) {
    BDB_insertRecordAfter(1, 0);
    BDB_insertRecordAfter(1, 0);
    rw_writeRecordWithFormat(1, 2, "{#}\n");
    STRNCMP_EQUAL(" 2", BDB_getWriteBuffer(), 2);
}


TEST(writeRecordWithFormat,
     writeRecordWithRecordId_withLeading0_writesRecordId) {
    rw_writeRecordWithFormat(1, 0, "{o#}\n");
    STRNCMP_EQUAL("00", BDB_getWriteBuffer(), 2);
}


TEST(writeRecordWithFormat,
     writeRecordWithColumnId_withoutLeading0_writesRecordValue) {
    BDB_setValue(0, 0, 1, 412);
    rw_writeRecordWithFormat(0, 0, "{&1}\n");
    STRNCMP_EQUAL(" 412 ", BDB_getWriteBuffer(), 4);
}


TEST(writeRecordWithFormat,
     writeRecordWithColumnId_withLeading0_writesRecordValue) {
    BDB_setValue(0, 0, 1, 412);
    rw_writeRecordWithFormat(0, 0, "{o&1}\n");
    STRNCMP_EQUAL("0412 ", BDB_getWriteBuffer(), 4);
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_padsWithTrailingSpacesUpToMaxStringBufferSize) {
    rw_writeRecordWithFormat(0, 0, "text\0");
    STRNCMP_EQUAL("text                ", BDB_getWriteBuffer(), 20);
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_writesDecimalWithAndWithoutLeading0) {
    BDB_setValue(1, 0, 2, 15);
    BDB_setValue(1, 0, 4, 75);
    rw_writeRecordWithFormat(1, 0, "{&4}-{o&4}-{&2}\0");
    STRNCMP_EQUAL(" 7.5-07.5-0.0015    ", BDB_getWriteBuffer(), 20);
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_withMaxVal99andOffset1_takes3digits) {
    rw_writeRecordWithFormat(3, 0, "a{&0+}b\0");
    STRNCMP_EQUAL("a 25b", BDB_getWriteBuffer(), 5);
}


// if literal


TEST(writeRecordWithFormat,
     writeRecordWithFormat_withIfEqLiteral_writesFirstOptionWhenTrue) {
    BDB_setValue(1, 0, 1, 0);
    rw_writeRecordWithFormat(1, 0, "|{?&1==0}n/a{:}S{o&1}{;}|\0");
    STRNCMP_EQUAL("|n/a|               ", BDB_getWriteBuffer(), 20);
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_withIfLiteral_writesSecondOptionWhenFalse) {
    BDB_setValue(1, 0, 1, 1);
    rw_writeRecordWithFormat(1, 0, "|{?&1==0}n/a{:}S{o&1}{;}|\0");
    STRNCMP_EQUAL("|S001|              ", BDB_getWriteBuffer(), 20);
}


// if other columnId


TEST(writeRecordWithFormat,
     writeRecordWithFormat_withIfOtherColumnIdWorks) {
    BDB_setValue(1, 0, 1, 3);
    BDB_setValue(1, 0, 2, 3);
    rw_writeRecordWithFormat(1, 0, "{?&1==&2}T{:}F{;}\0");
    STRNCMP_EQUAL("T", BDB_getWriteBuffer(), 1);
}


// if recordId


TEST(writeRecordWithFormat,
     writeRecordWithFormat_withRecordIdWorks) {
    BDB_insertRecordAfter(1, 0);
    BDB_insertRecordAfter(1, 0);
    BDB_setValue(1, 2, 1, 2);
    rw_writeRecordWithFormat(1, 2, "{?&1==#}T{:}F{;}\0");
    STRNCMP_EQUAL("T", BDB_getWriteBuffer(), 1);
}


// operators


TEST(writeRecordWithFormat,
     writeRecordWithFormat_withNEoperatorWorks) {
    BDB_setValue(1, 0, 1, 5);
    rw_writeRecordWithFormat(1, 0, "{?&1!=0}T{;}|{?&1!=5}F{;}\0");
    STRNCMP_EQUAL("T|                  ", BDB_getWriteBuffer(), 20);
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_withGEoperatorWorks) {
    BDB_setValue(1, 0, 1, 5);
    rw_writeRecordWithFormat(1, 0, "{?&1>=5}T{;}|{?&1>=6}F{;}\0");
    STRNCMP_EQUAL("T|                  ", BDB_getWriteBuffer(), 20);
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_withGToperatorWorks) {
    BDB_setValue(1, 0, 1, 5);
    rw_writeRecordWithFormat(1, 0, "{?&1>4}T{;}|{?&1>5}F{;}\0");
    STRNCMP_EQUAL("T|                   ", BDB_getWriteBuffer(), 20);
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_withLEoperatorWorks) {
    BDB_setValue(1, 0, 1, 5);
    rw_writeRecordWithFormat(1, 0, "{?&1<=5}T{;}|{?&1<=4}F{;}\0");
    STRNCMP_EQUAL("T|                  ", BDB_getWriteBuffer(), 20);
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_withLToperatorWorks) {
    BDB_setValue(1, 0, 1, 5);
    rw_writeRecordWithFormat(1, 0, "{?&1<6}T{;}|{?&1<5}F{;}\0");
    STRNCMP_EQUAL("T|                   ", BDB_getWriteBuffer(), 20);
}


// special values


TEST(writeRecordWithFormat,
     writeRecordWithFormat_recordIdPlusWorks) {
    BDB_insertRecordAfter(1, 0);
    BDB_insertRecordAfter(1, 0);
    rw_writeRecordWithFormat(1, 2, "{o#+}\0");
    STRNCMP_EQUAL("03                  ", BDB_getWriteBuffer(), 20);
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_asteriskShowsTotalNumberOfRecords) {
    BDB_insertRecordAfter(1, 0);
    BDB_insertRecordAfter(1, 0);
    rw_writeRecordWithFormat(1, 0, "{o*}\0");
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_asHexShowsHexaDecimalFormat) {
    BDB_setValue(0, 0, 1, 14);
    rw_writeRecordWithFormat(0, 0, "{&1x}\0");
    STRNCMP_EQUAL("00E ", BDB_getWriteBuffer(), 4);
}


TEST(writeRecordWithFormat,
     writeRecordWithFormat_hiddenColumnId_isInvisible) {
    rw_writeRecordWithFormat(0, 0, "a{h&1}b\0");
    STRNCMP_EQUAL("ab", BDB_getWriteBuffer(), 2);
}

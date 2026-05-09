// TestBitsOfData.cpp

#include "CppUTest/TestHarness.h"
#include "cpputestUtils.h"

extern "C" {
    #include "EeHw.h"
    #include "EeHwX86.h"
    #include "BitsOfData.h"
    #include "RecordStore.h"
    #include "SchemaForTesting.h"
}


TEST_GROUP(OpenDbase) {
    void setup() {
        eeClear();
    }

    void teardown() {
        BDB_closeDataBase();
    }
};


TEST(OpenDbase, openDataBaseWhenNoneExistsReturnsFalse) {
    CHECK_FALSE(BDB_openDataBase(&dbaseDef, NULL));
}


TEST(OpenDbase, openDataBaseWhenOneExistsReturnsTrue) {
    BDB_openDataBase(&dbaseDef, NULL);
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef, NULL));
}


// Use Dbase without getTxt functions


TEST_GROUP(UseDbase) {
    void setup() {
        eeClear();
        BDB_openDataBase(&dbaseDef, NULL);
    }

    void teardown() {
        BDB_closeDataBase();
    }
};


TEST(UseDbase, getNumRecordsReturns1onNewTable) {
    BYTES_EQUAL(1, BDB_getNumRecords(0));
}


TEST(UseDbase, getNumRecordsReturns1onNewVarRecordTable) {
    BYTES_EQUAL(1, BDB_getNumRecords(1));
}


TEST(UseDbase, getNumRecordsReturns2onNewVarRecordTableWithExtraRecord) {
    BDB_insertRecordAfter(1, 0);
    BYTES_EQUAL(2, BDB_getNumRecords(1));
}


TEST(UseDbase, getNumRealColumnsReturnsNumColumnsWithoutVirtual) {
    BYTES_EQUAL(4, BDB_getNumRealColumns(0, 0));
}


TEST(UseDbase, getNumRealColumnsReturnsNumColumnsWithoutVirtualForVariableRecordType) {
    BDB_setValue(1, 0, 0, 1); // set to 2nd recordType
    BYTES_EQUAL(3, BDB_getNumRealColumns(1, 0));
}


TEST(UseDbase, newTableHasrecordWithDefaultColValues) {
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
    LONGS_EQUAL(1234, BDB_getValue(0, 0, 1));
}


TEST(UseDbase, openDbaseStoresDefaultsOf1stRecord) {
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef, NULL));
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, setValueFailsIfValueIsBelowMin) {
    CHECK_FALSE(BDB_setValue(0, 0, 0, 2));
    BYTES_EQUAL(7, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, setValueSucceedsIfValueIsMin) {
    CHECK_TRUE(BDB_setValue(0, 0, 0, 3));
    BYTES_EQUAL(3, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, setValueFailsIfValueIsAboveMax) {
    CHECK_FALSE(BDB_setValue(0, 0, 0, 9));
    BYTES_EQUAL(7, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, setValueSucceedsIfValueIsMax) {
    CHECK_TRUE(BDB_setValue(0, 0, 0, 8));
    BYTES_EQUAL(8, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, setValueIsReturnedWithGetValue) {
    BDB_setValue(0, 0, 0, 3);
    BYTES_EQUAL(3, BDB_getValue(0, 0, 0));
    BDB_setValue(0, 0, 1, 3456);
    LONGS_EQUAL(3456, BDB_getValue(0, 0, 1));
}


TEST(UseDbase, changeValueUpWhenItIsAlreadyMaxFails) {
    BDB_setValue(0, 0, 0, 8);
    CHECK_FALSE(BDB_changeValue(0, 0, 0, 1));
    BYTES_EQUAL(8, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, changeValueUpBy1Succeeds) {
    BYTES_EQUAL(7, BDB_getValue(0, 0, 0));
    CHECK_TRUE(BDB_changeValue(0, 0, 0, 1));
    BYTES_EQUAL(8, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, changeValueDownWhenItIsAlreadyMinFails) {
    BDB_setValue(0, 0, 0, 3);
    CHECK_FALSE(BDB_changeValue(0, 0, 0, -1));
    BYTES_EQUAL(3, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, changeValueDownBy1Succeeds) {
    BDB_setValue(0, 0, 0, 4);
    CHECK_TRUE(BDB_changeValue(0, 0, 0, -1));
    BYTES_EQUAL(3, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, changeValueUpBy100IfItIsMaxFails) {
    BDB_setValue(0, 0, 1, 4000);
    CHECK_FALSE(BDB_changeValue(0, 0, 1, 100));
    BYTES_EQUAL(4000, BDB_getValue(0, 0, 1));
}


TEST(UseDbase, changeValueUpBy100Succeeds) {
    BDB_setValue(0, 0, 1, 3900);
    CHECK_TRUE(BDB_changeValue(0, 0, 1, 100));
    BYTES_EQUAL(4000, BDB_getValue(0, 0, 1));
}


TEST(UseDbase, changeValueUpBy100IfItIs50BelowMaxSucceedsAndAdds50) {
    BDB_setValue(0, 0, 1, 3950);
    CHECK_TRUE(BDB_changeValue(0, 0, 1, 100));
    BYTES_EQUAL(4000, BDB_getValue(0, 0, 1));
}


TEST(UseDbase, changeValueDownBy100IfItIsMinFails) {
    BDB_setValue(0, 0, 1, 0);
    CHECK_FALSE(BDB_changeValue(0, 0, 1, -100));
    BYTES_EQUAL(0, BDB_getValue(0, 0, 1));
}


TEST(UseDbase, changeValueDownBy100Succeeds) {
    BDB_setValue(0, 0, 1, 100);
    CHECK_TRUE(BDB_changeValue(0, 0, 1, -100));
    BYTES_EQUAL(0, BDB_getValue(0, 0, 1));
}


TEST(UseDbase, changeValueDownBy100IfItIs50AboveMinSucceedsAndSubtrackts50) {
    BDB_setValue(0, 0, 1, 50);
    CHECK_TRUE(BDB_changeValue(0, 0, 1, -100));
    BYTES_EQUAL(0, BDB_getValue(0, 0, 1));
}


TEST(UseDbase, retrievingDifferentRecordsInTheSameTableConsecutivelyWorks) {
    BDB_insertRecordAfter(0, 0);
    CHECK_TRUE(BDB_setValue(0, 0, 0, 4));
    CHECK_TRUE(BDB_setValue(0, 1, 0, 6));
    BYTES_EQUAL(4, BDB_getValue(0, 0, 0));
    BYTES_EQUAL(6, BDB_getValue(0, 1, 0));
}


// variable recordDefs


TEST(UseDbase, inVariableRecordDefTable_The1stColumnHoldsTheRecordDef) {
    LONGS_EQUAL(0, BDB_getValue(1, 0, 0)); // default = 0 !
}


TEST(UseDbase, inVariableRecordDefTable_SettingRecordTypeOutOfRangeReturnsFalse) {
    CHECK_FALSE(BDB_setValue(1,0,0,2));
    LONGS_EQUAL(0, BDB_getValue(1, 0, 0)); // unchanged
}


TEST(UseDbase, inVariableRecordDefTable_DefaultRecordTypeIsThe1st) {
    LONGS_EQUAL(150, BDB_getValue(1, 0, 1));
    LONGS_EQUAL(  0, BDB_getValue(1, 0, 2));
    LONGS_EQUAL(255, BDB_getValue(1, 0, 3));
}


TEST(UseDbase, inVariableRecordDefTable_ChangingRecordTypeSetsDefaults) {
    BDB_setValue(1, 0, 0, 1);
    LONGS_EQUAL(45, BDB_getValue(1, 0, 1));
}


TEST(UseDbase, inVariableRecordDefTable_changeValueWorksOnNormalColumns) {
    BDB_changeValue(1, 0, 1, 10);
    LONGS_EQUAL(160, BDB_getValue(1, 0, 1));
}


TEST(UseDbase, inVariableRecordDefTable_changeValueWorksOnRecordTypeColumn) {
    CHECK_TRUE(BDB_changeValue(1, 0, 0, 1));
    LONGS_EQUAL(1, BDB_getValue(1, 0, 0));
    LONGS_EQUAL(45, BDB_getValue(1, 0, 1));
}


TEST(UseDbase, inVariableRecordDefTable_changeValueFailsWhenOutOfRange) {
    BDB_setValue(1, 0, 0, 1);
    CHECK_FALSE(BDB_changeValue(1, 0, 0, 1));
    LONGS_EQUAL(1, BDB_getValue(1, 0, 0));
    LONGS_EQUAL(45, BDB_getValue(1, 0, 1));
}


TEST(UseDbase, inVariableRecordDefTable_changeValueSetsToMaxWhenItWouldExceed) {
    CHECK_TRUE(BDB_changeValue(1, 0, 0, 2));
    LONGS_EQUAL(1, BDB_getValue(1, 0, 0));
    LONGS_EQUAL(45, BDB_getValue(1, 0, 1));
}


TEST(UseDbase, inVariableRecordDefTableRetrievingRecordWithDifferentTypesConsecutivelyWorks) {
    BDB_insertRecordAfter(1, 0);
    CHECK_TRUE(BDB_setValue(1, 0, 0, 0)); // set recordType to 0
    CHECK_TRUE(BDB_setValue(1, 1, 0, 1)); // set recordType to 1
    BYTES_EQUAL( 45, BDB_getValue(1, 1, 1)); // default value
    BYTES_EQUAL(150, BDB_getValue(1, 0, 1)); // default value
}


// recordBuffer


TEST(UseDbase, gettingSameRecordConsecutivelyDoesNotRetreiveRecordAgain) {
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
    eeClear();
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, settingARecordDoesNotStoreIt) {
    CHECK_TRUE(BDB_setValue(0, 0, 0, 5));
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef, NULL));
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0)); // the default value
}


TEST(UseDbase, storeRecordStoresBuffer) {
    BDB_setValue(0, 0, 0, 6);
    BDB_storeRecord(0, 0);
    BDB_setValue(0, 0, 0, 5);
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef, NULL));
    LONGS_EQUAL(6, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, settingSameRecordConsecutivelyDoesNotStoreRecord) {
    BDB_setValue(0, 0, 0, 6);
    BDB_setValue(0, 0, 0, 5);
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef, NULL));
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0)); // the default value
}


TEST(UseDbase, accessingAnotherRecordInTheSameTable_StoresRecordBuffer) {
    BDB_insertRecordAfter(0, 0);
    CHECK_TRUE(BDB_setValue(0, 0, 0, 5));
    CHECK_TRUE(BDB_setValue(0, 1, 0, 6));
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef, NULL));
    LONGS_EQUAL(5, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, changeValueDoesNotStoreRecord) {
    CHECK_TRUE(BDB_changeValue(0, 0, 0, 1));
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef, NULL));
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, changeValueUsesBufferedValue) {
    CHECK_TRUE(BDB_changeValue(0, 0, 0, -1));
    eeClear();
    CHECK_TRUE(BDB_changeValue(0, 0, 0, -1));
    LONGS_EQUAL(5, BDB_getValue(0, 0, 0));
}


TEST(UseDbase, changeValueOnAnotherRecordStoresPreviousRecord) {
    BDB_insertRecordAfter(0, 0);
    CHECK_TRUE(BDB_changeValue(0, 0, 0, 1));
    LONGS_EQUAL(8, BDB_getValue(0, 0, 0));
    CHECK_TRUE(BDB_changeValue(0, 1, 1, 1));
    LONGS_EQUAL(1235, BDB_getValue(0, 1, 1));
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef, NULL));
    LONGS_EQUAL(8, BDB_getValue(0, 0, 0));  // stored
    LONGS_EQUAL(1234, BDB_getValue(0, 1, 1)); // not stored
}


TEST(UseDbase, accessingARecordInAnotherTable_DoesNotStoreRecordBuffer) {
    CHECK_TRUE(BDB_insertRecordAfter(1, 0));
    BDB_setValue(0, 0, 0, 6);
    BDB_setValue(1, 1, 1, 40);
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef, NULL));
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
    LONGS_EQUAL(150, BDB_getValue(1, 1, 1));
}


// NOTE: an actual change is not required, an edit without change suffices
TEST(UseDbase, changingToAnotherRecordDoesNotStoreIfValueWasNotEdited) {
    BDB_insertRecordAfter(0, 0);
    BDB_setValue(0, 0, 0, 6);
    BDB_setValue(0, 1, 0, 5);   // must store rec0=6
    LONGS_EQUAL(6, BDB_getValue(0, 0, 0)); // must store rec1=5
    uint8_t rec[5] = {0, 0, 0, 0};
    rs_setRawRecord(0, 0, rec);    // set rec0 to 0 (=3, min)
    LONGS_EQUAL(5, BDB_getValue(0, 1, 0)); // must not store rec0
    LONGS_EQUAL(3, BDB_getValue(0, 0, 0)); // rec0 is still 0
}


// set/get records


TEST(UseDbase, getRecordOnSingleRecDefReturnsAllNonVirtualColumns) {
    uint16_t record0[] = { 7, 1234, 0, 2 };
    CHECK_UINT16_ARRAY_EQUAL(record0, BDB_getRecord(0, 0), 4);
}


TEST(UseDbase, getRecordOnVariableRecDefReturnsAllNonVirtualColumns) {
    BDB_insertRecordAfter(1, 0);
    CHECK_TRUE(BDB_setValue(1, 0, 0, 0)); // set recordType to 0
    CHECK_TRUE(BDB_setValue(1, 1, 0, 1)); // set recordType to 1

    uint16_t rec1_0[] = { 0, 150, 0, 255};
    CHECK_UINT16_ARRAY_EQUAL(rec1_0, BDB_getRecord(1, 0), 4);
    uint16_t rec1_1[] = { 1, 45, 0, };
    CHECK_UINT16_ARRAY_EQUAL(rec1_1, BDB_getRecord(1, 1), 3);
}


TEST(UseDbase, setRecordToValidRecordSucceeds) {
    BDB_insertRecordAfter(1, 0);
    uint16_t rec1_0[] = { 0, 123, 75, 60, 500, 1};
    CHECK_TRUE(BDB_setRecord(1, 0, rec1_0));
    uint16_t rec1_1[] = { 1, 85, 0, };
    CHECK_TRUE(BDB_setRecord(1, 1, rec1_1));
    CHECK_UINT16_ARRAY_EQUAL(rec1_0, BDB_getRecord(1, 0), 4);
    CHECK_UINT16_ARRAY_EQUAL(rec1_1, BDB_getRecord(1, 1), 3);
}


TEST(UseDbase, setRecordToRecordWithValueGtMaxValueFails) {
    uint16_t record0[] = { 5, 4001, 0, 2 };
    CHECK_FALSE(BDB_setRecord(0, 0, record0));
    LONGS_EQUAL(1234, BDB_getValue(0, 0, 1)); // unchanged
}


TEST(UseDbase, setRecordToRecordWithValueLtMinValueFails) {
    uint16_t record0[] = { 0, 1000, 0, 2 };
    CHECK_FALSE(BDB_setRecord(0, 0, record0));
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0)); // unchanged
}


TEST(UseDbase, setRecordToRecordWithDecimalWithInvalidStepFails) {
    uint16_t record1[] = { 0, 123, 12, 512, 899};
    CHECK_FALSE(BDB_setRecord(1, 0, record1));
    LONGS_EQUAL(10, BDB_getValue(1, 0, 4)); // unchanged
}


TEST(UseDbase, setRecordToRecordWithInvalidReferenceFails) {
    uint16_t record0[] = { 4, 123, 8, 3 };
    CHECK_FALSE(BDB_setRecord(0, 0, record0));
    LONGS_EQUAL(0, BDB_getValue(1, 0, 2)); // unchanged
}


// adding records


TEST(UseDbase, insertRecordCreatesDefaultRecord) {
    BDB_insertRecordAfter(0, 0);
    LONGS_EQUAL(1234, BDB_getValue(0, 1, 1));
}


TEST(UseDbase, insertRecordAfterReturnsFalseIfMaxNumRecordsReached) {
    for (uint8_t rec = 0; rec < 9; rec++) {
        CHECK_TRUE(BDB_insertRecordAfter(0, 0));
    }
    BYTES_EQUAL(10, BDB_getNumRecords(0));
    CHECK_FALSE(BDB_insertRecordAfter(0, 0));
}


TEST(UseDbase, canRecordBeAddedReturnsTrueIfThereIsOnly1Record) {
    CHECK_TRUE(BDB_canRecordBeAdded(0));
}


TEST(UseDbase, canRecordBeAddedReturnsTrueIfThereAre1FewerThanMaxRecords) {
    for (uint8_t rec = 1; rec < 9; rec++) {
        CHECK_TRUE(BDB_insertRecordAfter(0, 0));
    }
    CHECK_TRUE(BDB_canRecordBeAdded(0));
}


TEST(UseDbase, canRecordBeAddedReturnsFalseeIfThereAreMaxRecords) {
    for (uint8_t rec = 1; rec < 10; rec++) {
        CHECK_TRUE(BDB_insertRecordAfter(0, 0));
    }
    CHECK_FALSE(BDB_canRecordBeAdded(0));
}


// deleting records


TEST(UseDbase, deleteRecordReturnsTrueIfRecordWasDeleted) {
    rs_appendRecord(0);
    CHECK_TRUE(BDB_deleteRecord(0, 0));
}


TEST(UseDbase, deleteRecordReturnsFalseIfThereIsOnly1Record) {
    CHECK_FALSE(BDB_deleteRecord(0, 0));
}


TEST(UseDbase, deleteRecordDeletesRecord) {
    rs_appendRecord(0);
    BDB_deleteRecord(0, 0);
    BYTES_EQUAL(1, BDB_getNumRecords(0));
}


TEST(UseDbase, canRecordBeDeleted_ReturnsFalseIfItWasTheLastRecord) {
    CHECK_FALSE(BDB_canRecordBeDeleted(0, 0));
}


TEST(UseDbase, canRecordBeDelete_dReturnsTrueIfItHasNoDependencies) {
    rs_appendRecord(0);
    CHECK_TRUE(BDB_canRecordBeDeleted(0, 1));
}


TEST(UseDbase, canRecordBeDeteted_ReturnsFalseIfAnotherRecordReferencesIt) {
    rs_appendRecord(0);
    rs_appendRecord(2);
    rs_appendRecord(2);
    CHECK_TRUE(BDB_setValue(0, 1, 2, 1)); // 0,1,2 points to recordId 1 (in table 2)
    CHECK_FALSE(BDB_canRecordBeDeleted(2, 1));  // so table 2, record 1 cannot be deleted
}


TEST(UseDbase, canRecordBeDeteted_ReturnsFalseIf_ItIsReferencedFromAVariableRecord) {
    rs_appendRecord(1);
    rs_appendRecord(2);
    rs_appendRecord(2);
    CHECK_TRUE(BDB_setValue(1, 1, 0, 1)); // set recordType to 1
    CHECK_TRUE(BDB_setValue(1, 1, 2, 2)); // 1,1,2 points to recordId 2 (in table 2)
    CHECK_FALSE(BDB_canRecordBeDeleted(2, 2));  // so table 2, record 2 cannot be deleted
}


// BDB_COLUMN_INTEGER


TEST(UseDbase, stringValueOf4DigitIntReturnsStringOfLength4) {
    BYTES_EQUAL(4, BDB_writeValue(0, 0, 1, 0));
    MEMCMP_EQUAL("1234", BDB_getWriteBuffer(), 4);
}


// where maxDigits == numDigits of .maxValue
TEST(UseDbase, stringValueOf1DigitIntReturnsStringOfMaxLength) {
    BDB_setValue(0, 0, 1, 9);
    BYTES_EQUAL(4, BDB_writeValue(0, 0, 1, 0));
    MEMCMP_EQUAL("   9", BDB_getWriteBuffer(), 4);
}


TEST(UseDbase, stringValueOfIntWithLeading0ReturnsStringWithLeading0s) {
    BDB_setValue(2, 0, 0, 14);
    BYTES_EQUAL(3, BDB_writeValue(2, 0, 0, 0));
    MEMCMP_EQUAL("014", BDB_getWriteBuffer(), 3);
}


// BDB_COLUMN_REFERENCE


TEST(UseDbase, setValueOfReferenceColumn_ToExistingRecord_InRefTableSucceeds) {
    CHECK_TRUE(BDB_setValue(0, 0, 2, 0));
}


TEST(UseDbase, setValueOfReferenceColumnToNonExistingRecordInRefTableFails) {
    CHECK_FALSE(BDB_setValue(0, 0, 2, 1));
}


TEST(UseDbase, changeValueOnReferenceColumnToExistingRecordInRefTableSucceeds) {
    rs_appendRecord(2);
    rs_appendRecord(2);
    CHECK_TRUE(BDB_changeValue(0, 0, 2, 2));
}


TEST(UseDbase, changeValueOnReferenceColumnToNonExistingRecordInRefTableFails) {
    rs_appendRecord(2);
    rs_appendRecord(2);
    CHECK_TRUE(BDB_changeValue(0, 0, 2, 2));
    CHECK_FALSE(BDB_changeValue(0, 0, 2, 1));
}


TEST(UseDbase, changeValueOnReferenceColumn_ToExistingRecordInRefTable_SetsValueToNumRecords) {
    rs_appendRecord(2);
    rs_appendRecord(2);
    CHECK_TRUE(BDB_changeValue(0, 0, 2, 5));
    BYTES_EQUAL(3, BDB_getNumRecords(2));
    BYTES_EQUAL(2, BDB_getValue(0,0,2));
}


TEST(UseDbase, deletingALowerRecordInTheRefTableAdjustssRecordReference) {
    rs_appendRecord(2); // table 2, record 1
    rs_appendRecord(2); // table 2, record 2
    CHECK_TRUE(BDB_setValue(0, 0, 2, 1)); // references table 2, record 1
    BDB_deleteRecord(2, 0); // 0,0,2 should now referece table 2, record 0
    BYTES_EQUAL(0, BDB_getValue(0, 0, 2));
}


TEST(UseDbase, insertingALowerRecordInTheRefTableAdjustsRecordReference) {
    rs_appendRecord(2); // table 2, record 1
    rs_appendRecord(2); // table 2, record 2
    CHECK_TRUE(BDB_setValue(0, 0, 2, 1)); // references table 2, record 1
    BDB_insertRecordAfter(2, 0); // 0,0,2 should now referece table 2, record 2
    BYTES_EQUAL(2, BDB_getValue(0, 0, 2));
}


// BDB_COLUMN_VIRTUAL


TEST(UseDbase, setValueFailsOnAVirtualColumn) {
    CHECK_FALSE(BDB_setValue(0, 0, 4, 0));
}


TEST(UseDbase, changeValueFailsOnAVirtualColumn) {
    CHECK_FALSE(BDB_changeValue(0, 0, 4, 1));
}


TEST(UseDbase, getValueOnAVirtualColumn_ReturnsValueOfTheReferencedTableColumn) {
    BDB_setValue(2, 0, 1, 8);
    BYTES_EQUAL(8, BDB_getValue(0, 0, 4));
}


// BDB_COLUMN_CHAR


TEST(UseDbase, getValueOnCharReturnsNumericValue) {
    BDB_setValue(2, 0, 1, 12);
    BYTES_EQUAL(12, BDB_getValue(2, 0, 1));
}


TEST(UseDbase, writeValueSetsCharFromCharset) {
    BDB_setValue(2, 0, 1, 12);
    BYTES_EQUAL(1, BDB_writeValue(2, 0, 1, 0));
    BYTES_EQUAL(charSet[12], BDB_getWriteBuffer()[0]);
}


// BDB_COLUMN_STRING


TEST(UseDbase, setValueOnStringColumnFails) {
    CHECK_FALSE(BDB_setValue(2, 0, 5, 0));
}


TEST(UseDbase, changeValueOnStringColumnFails) {
    CHECK_FALSE(BDB_changeValue(2, 0, 5, 1));
}


TEST(UseDbase, getValueOnStringColumnFails) {
    LONGS_EQUAL(0xFFFF, BDB_getValue(2, 0, 5));
}


TEST(UseDbase, writeValueOnAStringColumnWritesAllChars) {
    BDB_writeValue(2, 0, 5, 0);
    STRNCMP_EQUAL("    ", BDB_getWriteBuffer() , 4);
}


// BDB_COLUMN_DECIMAL


TEST(UseDbase, setValueToNonStepValueFails) {
    CHECK_FALSE(BDB_setValue(1, 0, 4, 238));
}


TEST(UseDbase, setValueToStepValueSucceeds) {
    CHECK_TRUE(BDB_setValue(1, 0, 4, 25));
}


TEST(UseDbase, changeValueBy1IncreasesValueByStep) {
    CHECK_TRUE(BDB_changeValue(1, 0, 4, 1));
    LONGS_EQUAL(15, BDB_getValue(1, 0, 4));
}


TEST(UseDbase, writeValue10_OnDecimalColumnWith1DecimalInsertsPointBeforeLastDigit) {
    BDB_writeValue(1, 0, 4, 0);
    STRNCMP_EQUAL(" 1.0", BDB_getWriteBuffer(), 4);
}


TEST(UseDbase, writeValue0_OnDecimalColumnWith4DecimalsInserts0pt000) {
    BDB_writeValue(1, 0, 2, 0);
    STRNCMP_EQUAL("0.0000\n", BDB_getWriteBuffer(), 6);
}


TEST(UseDbase, writeValue19_OnDecimalColumnWith4DecimalsInserts0tp00) {
    BDB_setValue(1, 0, 2, 19);
    BDB_writeValue(1, 0, 2, 0);
    STRNCMP_EQUAL("0.0019\n", BDB_getWriteBuffer(), 6);
}


// Use Dbase with getTxt functions


TEST_GROUP(UseDbaseWithTxt) {
    void setup() {
        eeClear();
    }

    void teardown() {
        BDB_closeDataBase();
    }
};


static uint8_t length(const char* txt) {
    for (uint8_t length = 0; ; length++) {
        if (txt[length] == '\0') return length + 1;
    }
}


static const char* texts[] = {"zero", "yes", "no"};


static uint8_t getTxt(const char** txtPtr, const uint8_t txtId) {
    if (txtPtr != NULL) { // in case of length-only call
        *txtPtr = texts[txtId];
    }
    return length(texts[txtId]);
}


// BDB_COLUMN_INT_ZEROTXT


TEST(UseDbaseWithTxt, writeValue0_onIntZerovalColumn_returnsText) {
    BDB_openDataBase(&dbaseDef, getTxt);
    BDB_setValue(0, 0, 3, 0);
    BDB_writeValue(0, 0, 3, 0);
    STRNCMP_EQUAL("no", BDB_getWriteBuffer(), 2);
}


TEST(UseDbaseWithTxt, writeValue1_onIntZerovalColumn_returns1) {
    BDB_openDataBase(&dbaseDef, getTxt);
    BDB_setValue(0, 0, 3, 1);
    BDB_writeValue(0, 0, 3, 0);
    STRNCMP_EQUAL(" 1", BDB_getWriteBuffer(), 2);
}


// BDB_COLUMN_TXT_LIST


TEST(UseDbaseWithTxt, writeValue_onTxtListColumn_returnsTextFromFirstId) {
    BDB_openDataBase(&dbaseDef, getTxt);
    BDB_setValue(1, 0, 5, 2);
    BDB_writeValue(1, 0, 5, 0);
    STRNCMP_EQUAL("zero", BDB_getWriteBuffer(), 4); // returns texts[0], although value = 2 !
}


TEST(UseDbaseWithTxt, writeValue_onTxtListColumn_fillsUpToLengthOfLongestTxtInList) {
    BDB_openDataBase(&dbaseDef, getTxt);
    BDB_setValue(1, 0, 5, 1);
    BDB_writeValue(1, 0, 5, 0);
    STRNCMP_EQUAL("no  \0", BDB_getWriteBuffer(), 5);
}



/* TODO:
 *
 * remaining columnType:
 *  BDB_COLUMN_SYMBOL_LIST
 *
 * remaining writeColumnValue implementations for columnTypes
 *
 * writeRecord
 *
 * writeColumnValue -> start pos - end pos
 *
 * refactor: move API to bottom and all static functions to top
 * refactor: use getColumnDef, getRecordDef everywhere possible
 */

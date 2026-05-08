// TestBitsOfData.cpp

/*
 * TDD approach:
 * 1)   implement explicit data model tests
 * 2)   implement pure API tests
 * 3)   rewrite explicit data model tests, using API functions where possible
 * 4)   remove tests from 1) that cannot fail independently from 2) or 3)
 * 5)   mark all remaining tests from 1) as "data model dependent"
 *
 */

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
    CHECK_FALSE(BDB_openDataBase(&dbaseDef));
}


TEST(OpenDbase, openDataBaseWhenOneExistsReturnsTrue) {
    BDB_openDataBase(&dbaseDef);
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef));
}


TEST(OpenDbase, getNumRecordsReturns1onNewTable) {
    BDB_openDataBase(&dbaseDef);
    BYTES_EQUAL(1, BDB_getNumRecords(0));
}


TEST(OpenDbase, getNumRecordsReturns1onNewVarRecordTable) {
    BDB_openDataBase(&dbaseDef);
    BYTES_EQUAL(1, BDB_getNumRecords(1));
}


TEST(OpenDbase, getNumRecordsReturns2onNewVarRecordTableWithExtraRecord) {
    BDB_openDataBase(&dbaseDef);
    BDB_insertRecordAfter(1, 0);
    BYTES_EQUAL(2, BDB_getNumRecords(1));
}


TEST(OpenDbase, getNumRealColumnsReturnsNumColumnsWithoutVirtual) {
    BDB_openDataBase(&dbaseDef);
    BYTES_EQUAL(4, BDB_getNumRealColumns(0, 0));
}


TEST(OpenDbase, getNumRealColumnsReturnsNumColumnsWithoutVirtualForVariableRecordType) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(1, 0, 0, 1); // set to 2nd recordType
    BYTES_EQUAL(3, BDB_getNumRealColumns(1, 0));
}


TEST(OpenDbase, newTableHasrecordWithDefaultColValues) {
    BDB_openDataBase(&dbaseDef);
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
    LONGS_EQUAL(1234, BDB_getValue(0, 0, 1));
}


TEST(OpenDbase, openDbaseStoresDefaultsOf1stRecord) {
    CHECK_FALSE(BDB_openDataBase(&dbaseDef));
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef));
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, setValueFailsIfValueIsBelowMin) {
    BDB_openDataBase(&dbaseDef);
    CHECK_FALSE(BDB_setValue(0, 0, 0, 2));
    BYTES_EQUAL(7, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, setValueSucceedsIfValueIsMin) {
    BDB_openDataBase(&dbaseDef);
    CHECK_TRUE(BDB_setValue(0, 0, 0, 3));
    BYTES_EQUAL(3, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, setValueFailsIfValueIsAboveMax) {
    BDB_openDataBase(&dbaseDef);
    CHECK_FALSE(BDB_setValue(0, 0, 0, 9));
    BYTES_EQUAL(7, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, setValueSucceedsIfValueIsMax) {
    BDB_openDataBase(&dbaseDef);
    CHECK_TRUE(BDB_setValue(0, 0, 0, 8));
    BYTES_EQUAL(8, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, setValueIsReturnedWithGetValue) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(0, 0, 0, 3);
    BYTES_EQUAL(3, BDB_getValue(0, 0, 0));
    BDB_setValue(0, 0, 1, 3456);
    LONGS_EQUAL(3456, BDB_getValue(0, 0, 1));
}


TEST(OpenDbase, changeValueUpWhenItIsAlreadyMaxFails) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(0, 0, 0, 8);
    CHECK_FALSE(BDB_changeValue(0, 0, 0, 1));
    BYTES_EQUAL(8, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, changeValueUpBy1Succeeds) {
    BDB_openDataBase(&dbaseDef);
    BYTES_EQUAL(7, BDB_getValue(0, 0, 0));
    CHECK_TRUE(BDB_changeValue(0, 0, 0, 1));
    BYTES_EQUAL(8, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, changeValueDownWhenItIsAlreadyMinFails) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(0, 0, 0, 3);
    CHECK_FALSE(BDB_changeValue(0, 0, 0, -1));
    BYTES_EQUAL(3, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, changeValueDownBy1Succeeds) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(0, 0, 0, 4);
    CHECK_TRUE(BDB_changeValue(0, 0, 0, -1));
    BYTES_EQUAL(3, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, changeValueUpBy100IfItIsMaxFails) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(0, 0, 1, 4000);
    CHECK_FALSE(BDB_changeValue(0, 0, 1, 100));
    BYTES_EQUAL(4000, BDB_getValue(0, 0, 1));
}


TEST(OpenDbase, changeValueUpBy100Succeeds) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(0, 0, 1, 3900);
    CHECK_TRUE(BDB_changeValue(0, 0, 1, 100));
    BYTES_EQUAL(4000, BDB_getValue(0, 0, 1));
}


TEST(OpenDbase, changeValueUpBy100IfItIs50BelowMaxSucceedsAndAdds50) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(0, 0, 1, 3950);
    CHECK_TRUE(BDB_changeValue(0, 0, 1, 100));
    BYTES_EQUAL(4000, BDB_getValue(0, 0, 1));
}


TEST(OpenDbase, changeValueDownBy100IfItIsMinFails) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(0, 0, 1, 0);
    CHECK_FALSE(BDB_changeValue(0, 0, 1, -100));
    BYTES_EQUAL(0, BDB_getValue(0, 0, 1));
}


TEST(OpenDbase, changeValueDownBy100Succeeds) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(0, 0, 1, 100);
    CHECK_TRUE(BDB_changeValue(0, 0, 1, -100));
    BYTES_EQUAL(0, BDB_getValue(0, 0, 1));
}


TEST(OpenDbase, changeValueDownBy100IfItIs50AboveMinSucceedsAndSubtrackts50) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(0, 0, 1, 50);
    CHECK_TRUE(BDB_changeValue(0, 0, 1, -100));
    BYTES_EQUAL(0, BDB_getValue(0, 0, 1));
}


TEST(OpenDbase, retrievingDifferentRecordsInTheSameTableConsecutivelyWorks) {
    BDB_openDataBase(&dbaseDef);
    BDB_insertRecordAfter(0, 0);
    CHECK_TRUE(BDB_setValue(0, 0, 0, 4));
    CHECK_TRUE(BDB_setValue(0, 1, 0, 6));
    BYTES_EQUAL(4, BDB_getValue(0, 0, 0));
    BYTES_EQUAL(6, BDB_getValue(0, 1, 0));
}


// variable recordDefs


TEST(OpenDbase, inVariableRecordDefTable_The1stColumnHoldsTheRecordDef) {
    BDB_openDataBase(&dbaseDef);
    LONGS_EQUAL(0, BDB_getValue(1, 0, 0)); // default = 0 !
}


TEST(OpenDbase, inVariableRecordDefTable_SettingRecordTypeOutOfRangeReturnsFalse) {
    BDB_openDataBase(&dbaseDef);
    CHECK_FALSE(BDB_setValue(1,0,0,2));
    LONGS_EQUAL(0, BDB_getValue(1, 0, 0)); // unchanged
}


TEST(OpenDbase, inVariableRecordDefTable_DefaultRecordTypeIsThe1st) {
    BDB_openDataBase(&dbaseDef);
    LONGS_EQUAL(150, BDB_getValue(1, 0, 1));
    LONGS_EQUAL(  0, BDB_getValue(1, 0, 2));
    LONGS_EQUAL(255, BDB_getValue(1, 0, 3));
}


TEST(OpenDbase, inVariableRecordDefTable_ChangingRecordTypeSetsDefaults) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(1, 0, 0, 1);
    LONGS_EQUAL(45, BDB_getValue(1, 0, 1));
}


TEST(OpenDbase, inVariableRecordDefTable_changeValueWorksOnNormalColumns) {
    BDB_openDataBase(&dbaseDef);
    BDB_changeValue(1, 0, 1, 10);
    LONGS_EQUAL(160, BDB_getValue(1, 0, 1));
}


TEST(OpenDbase, inVariableRecordDefTable_changeValueWorksOnRecordTypeColumn) {
    BDB_openDataBase(&dbaseDef);
    CHECK_TRUE(BDB_changeValue(1, 0, 0, 1));
    LONGS_EQUAL(1, BDB_getValue(1, 0, 0));
    LONGS_EQUAL(45, BDB_getValue(1, 0, 1));
}


TEST(OpenDbase, inVariableRecordDefTable_changeValueFailsWhenOutOfRange) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(1, 0, 0, 1);
    CHECK_FALSE(BDB_changeValue(1, 0, 0, 1));
    LONGS_EQUAL(1, BDB_getValue(1, 0, 0));
    LONGS_EQUAL(45, BDB_getValue(1, 0, 1));
}


TEST(OpenDbase, inVariableRecordDefTable_changeValueSetsToMaxWhenItWouldExceed) {
    BDB_openDataBase(&dbaseDef);
    CHECK_TRUE(BDB_changeValue(1, 0, 0, 2));
    LONGS_EQUAL(1, BDB_getValue(1, 0, 0));
    LONGS_EQUAL(45, BDB_getValue(1, 0, 1));
}


TEST(OpenDbase, inVariableRecordDefTableRetrievingRecordWithDifferentTypesConsecutivelyWorks) {
    BDB_openDataBase(&dbaseDef);
    BDB_insertRecordAfter(1, 0);
    CHECK_TRUE(BDB_setValue(1, 0, 0, 0)); // set recordType to 0
    CHECK_TRUE(BDB_setValue(1, 1, 0, 1)); // set recordType to 1
    BYTES_EQUAL( 45, BDB_getValue(1, 1, 1)); // default value
    BYTES_EQUAL(150, BDB_getValue(1, 0, 1)); // default value
}


// recordBuffer


TEST(OpenDbase, gettingSameRecordConsecutivelyDoesNotRetreiveRecordAgain) {
    BDB_openDataBase(&dbaseDef);
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
    eeClear();
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, settingARecordDoesNotStoreIt) {
    BDB_openDataBase(&dbaseDef);
    CHECK_TRUE(BDB_setValue(0, 0, 0, 5));
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef));
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0)); // the default value
}


TEST(OpenDbase, storeRecordStoresBuffer) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(0, 0, 0, 6);
    BDB_storeRecord(0, 0);
    BDB_setValue(0, 0, 0, 5);
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef));
    LONGS_EQUAL(6, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, settingSameRecordConsecutivelyDoesNotStoreRecord) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(0, 0, 0, 6);
    BDB_setValue(0, 0, 0, 5);
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef));
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0)); // the default value
}


TEST(OpenDbase, accessingAnotherRecordInTheSameTable_StoresRecordBuffer) {
    BDB_openDataBase(&dbaseDef);
    BDB_insertRecordAfter(0, 0);
    CHECK_TRUE(BDB_setValue(0, 0, 0, 5));
    CHECK_TRUE(BDB_setValue(0, 1, 0, 6));
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef));
    LONGS_EQUAL(5, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, changeValueDoesNotStoreRecord) {
    BDB_openDataBase(&dbaseDef);
    CHECK_TRUE(BDB_changeValue(0, 0, 0, 1));
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef));
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, changeValueUsesBufferedValue) {
    BDB_openDataBase(&dbaseDef);
    CHECK_TRUE(BDB_changeValue(0, 0, 0, -1));
    eeClear();
    CHECK_TRUE(BDB_changeValue(0, 0, 0, -1));
    LONGS_EQUAL(5, BDB_getValue(0, 0, 0));
}


TEST(OpenDbase, changeValueOnAnotherRecordStoresPreviousRecord) {
    CHECK_FALSE(BDB_openDataBase(&dbaseDef));
    BDB_insertRecordAfter(0, 0);
    CHECK_TRUE(BDB_changeValue(0, 0, 0, 1));
    LONGS_EQUAL(8, BDB_getValue(0, 0, 0));
    CHECK_TRUE(BDB_changeValue(0, 1, 1, 1));
    LONGS_EQUAL(1235, BDB_getValue(0, 1, 1));
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef));
    LONGS_EQUAL(8, BDB_getValue(0, 0, 0));  // stored
    LONGS_EQUAL(1234, BDB_getValue(0, 1, 1)); // not stored
}


TEST(OpenDbase, accessingARecordInAnotherTable_DoesNotStoreRecordBuffer) {
    CHECK_FALSE(BDB_openDataBase(&dbaseDef));
    CHECK_TRUE(BDB_insertRecordAfter(1, 0));
    BDB_setValue(0, 0, 0, 6);
    BDB_setValue(1, 1, 1, 40);
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef));
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0));
    LONGS_EQUAL(150, BDB_getValue(1, 1, 1));
}


// NOTE: an actual change is not required, an edit without change suffices
TEST(OpenDbase, changingToAnotherRecordDoesNotStoreIfValueWasNotEdited) {
    BDB_openDataBase(&dbaseDef);
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


TEST(OpenDbase, getRecordOnSingleRecDefReturnsAllNonVirtualColumns) {
    BDB_openDataBase(&dbaseDef);
    uint16_t record0[] = { 7, 1234, 0, 2 };
    CHECK_UINT16_ARRAY_EQUAL(record0, BDB_getRecord(0, 0), 4);
}


TEST(OpenDbase, getRecordOnVariableRecDefReturnsAllNonVirtualColumns) {
    BDB_openDataBase(&dbaseDef);
    BDB_insertRecordAfter(1, 0);
    CHECK_TRUE(BDB_setValue(1, 0, 0, 0)); // set recordType to 0
    CHECK_TRUE(BDB_setValue(1, 1, 0, 1)); // set recordType to 1

    uint16_t rec1_0[] = { 0, 150, 0, 255};
    CHECK_UINT16_ARRAY_EQUAL(rec1_0, BDB_getRecord(1, 0), 4);
    uint16_t rec1_1[] = { 1, 45, 0, };
    CHECK_UINT16_ARRAY_EQUAL(rec1_1, BDB_getRecord(1, 1), 3);
}


TEST(OpenDbase, setRecordToValidRecordSucceeds) {
    BDB_openDataBase(&dbaseDef);
    BDB_insertRecordAfter(1, 0);
    uint16_t rec1_0[] = { 0, 123, 75, 60, 500};
    CHECK_TRUE(BDB_setRecord(1, 0, rec1_0));
    uint16_t rec1_1[] = { 1, 85, 0, };
    CHECK_TRUE(BDB_setRecord(1, 1, rec1_1));
    CHECK_UINT16_ARRAY_EQUAL(rec1_0, BDB_getRecord(1, 0), 4);
    CHECK_UINT16_ARRAY_EQUAL(rec1_1, BDB_getRecord(1, 1), 3);
}


TEST(OpenDbase, setRecordToRecordWithValueGtMaxValueFails) {
    BDB_openDataBase(&dbaseDef);
    uint16_t record0[] = { 5, 4001, 0, 2 };
    CHECK_FALSE(BDB_setRecord(0, 0, record0));
    LONGS_EQUAL(1234, BDB_getValue(0, 0, 1)); // unchanged
}


TEST(OpenDbase, setRecordToRecordWithValueLtMinValueFails) {
    BDB_openDataBase(&dbaseDef);
    uint16_t record0[] = { 0, 1000, 0, 2 };
    CHECK_FALSE(BDB_setRecord(0, 0, record0));
    LONGS_EQUAL(7, BDB_getValue(0, 0, 0)); // unchanged
}


TEST(OpenDbase, setRecordToRecordWithDecimalWithInvalidStepFails) {
    BDB_openDataBase(&dbaseDef);
    uint16_t record1[] = { 0, 123, 12, 512, 899};
    CHECK_FALSE(BDB_setRecord(1, 0, record1));
    LONGS_EQUAL(10, BDB_getValue(1, 0, 4)); // unchanged
}


TEST(OpenDbase, setRecordToRecordWithInvalidReferenceFails) {
    BDB_openDataBase(&dbaseDef);
    uint16_t record0[] = { 4, 123, 8, 3 };
    CHECK_FALSE(BDB_setRecord(0, 0, record0));
    LONGS_EQUAL(0, BDB_getValue(1, 0, 2)); // unchanged
}


// adding records


TEST(OpenDbase, insertRecordCreatesDefaultRecord) {
    BDB_openDataBase(&dbaseDef);
    BDB_insertRecordAfter(0, 0);
    LONGS_EQUAL(1234, BDB_getValue(0, 1, 1));
}


TEST(OpenDbase, insertRecordAfterReturnsFalseIfMaxNumRecordsReached) {
    BDB_openDataBase(&dbaseDef);
    for (uint8_t rec = 0; rec < 9; rec++) {
        CHECK_TRUE(BDB_insertRecordAfter(0, 0));
    }
    BYTES_EQUAL(10, BDB_getNumRecords(0));
    CHECK_FALSE(BDB_insertRecordAfter(0, 0));
}


TEST(OpenDbase, canRecordBeAddedReturnsTrueIfThereIsOnly1Record) {
    BDB_openDataBase(&dbaseDef);
    CHECK_TRUE(BDB_canRecordBeAdded(0));
}


TEST(OpenDbase, canRecordBeAddedReturnsTrueIfThereAre1FewerThanMaxRecords) {
    BDB_openDataBase(&dbaseDef);
    for (uint8_t rec = 1; rec < 9; rec++) {
        CHECK_TRUE(BDB_insertRecordAfter(0, 0));
    }
    CHECK_TRUE(BDB_canRecordBeAdded(0));
}


TEST(OpenDbase, canRecordBeAddedReturnsFalseeIfThereAreMaxRecords) {
    BDB_openDataBase(&dbaseDef);
    for (uint8_t rec = 1; rec < 10; rec++) {
        CHECK_TRUE(BDB_insertRecordAfter(0, 0));
    }
    CHECK_FALSE(BDB_canRecordBeAdded(0));
}


// deleting records


TEST(OpenDbase, deleteRecordReturnsTrueIfRecordWasDeleted) {
    BDB_openDataBase(&dbaseDef);
    rs_appendRecord(0);
    CHECK_TRUE(BDB_deleteRecord(0, 0));
}


TEST(OpenDbase, deleteRecordReturnsFalseIfThereIsOnly1Record) {
    BDB_openDataBase(&dbaseDef);
    CHECK_FALSE(BDB_deleteRecord(0, 0));
}


TEST(OpenDbase, deleteRecordDeletesRecord) {
    BDB_openDataBase(&dbaseDef);
    rs_appendRecord(0);
    BDB_deleteRecord(0, 0);
    BYTES_EQUAL(1, BDB_getNumRecords(0));
}


TEST(OpenDbase, canRecordBeDeleted_ReturnsFalseIfItWasTheLastRecord) {
    BDB_openDataBase(&dbaseDef);
    CHECK_FALSE(BDB_canRecordBeDeleted(0, 0));
}


TEST(OpenDbase, canRecordBeDelete_dReturnsTrueIfItHasNoDependencies) {
    BDB_openDataBase(&dbaseDef);
    rs_appendRecord(0);
    CHECK_TRUE(BDB_canRecordBeDeleted(0, 1));
}


TEST(OpenDbase, canRecordBeDeteted_ReturnsFalseIfAnotherRecordReferencesIt) {
    BDB_openDataBase(&dbaseDef);
    rs_appendRecord(0);
    rs_appendRecord(2);
    rs_appendRecord(2);
    CHECK_TRUE(BDB_setValue(0, 1, 2, 1)); // 0,1,2 points to recordId 1 (in table 2)
    CHECK_FALSE(BDB_canRecordBeDeleted(2, 1));  // so table 2, record 1 cannot be deleted
}


TEST(OpenDbase, canRecordBeDeteted_ReturnsFalseIf_ItIsReferencedFromAVariableRecord) {
    BDB_openDataBase(&dbaseDef);
    rs_appendRecord(1);
    rs_appendRecord(2);
    rs_appendRecord(2);
    CHECK_TRUE(BDB_setValue(1, 1, 0, 1)); // set recordType to 1
    CHECK_TRUE(BDB_setValue(1, 1, 2, 2)); // 1,1,2 points to recordId 2 (in table 2)
    CHECK_FALSE(BDB_canRecordBeDeleted(2, 2));  // so table 2, record 2 cannot be deleted
}


// BDB_COLUMN_INTEGER


TEST(OpenDbase, stringValueOf4DigitIntReturnsStringOfLength4) {
    BDB_openDataBase(&dbaseDef);
    BYTES_EQUAL(4, BDB_writeValue(0, 0, 1, 0));
    MEMCMP_EQUAL("1234", BDB_getWriteBuffer(), 4);
}


// where maxDigits == numDigits of .maxValue
TEST(OpenDbase, stringValueOf1DigitIntReturnsStringOfMaxLength) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(0, 0, 1, 9);
    BYTES_EQUAL(4, BDB_writeValue(0, 0, 1, 0));
    MEMCMP_EQUAL("   9", BDB_getWriteBuffer(), 4);
}


TEST(OpenDbase, stringValueOfIntWithLeading0ReturnsStringWithLeading0s) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(2, 0, 0, 14);
    BYTES_EQUAL(3, BDB_writeValue(2, 0, 0, 0));
    MEMCMP_EQUAL("014", BDB_getWriteBuffer(), 3);
}


// BDB_COLUMN_REFERENCE


TEST(OpenDbase, setValueOfReferenceColumn_ToExistingRecord_InRefTableSucceeds) {
    BDB_openDataBase(&dbaseDef);
    CHECK_TRUE(BDB_setValue(0, 0, 2, 0));
}


TEST(OpenDbase, setValueOfReferenceColumnToNonExistingRecordInRefTableFails) {
    BDB_openDataBase(&dbaseDef);
    CHECK_FALSE(BDB_setValue(0, 0, 2, 1));
}


TEST(OpenDbase, changeValueOnReferenceColumnToExistingRecordInRefTableSucceeds) {
    BDB_openDataBase(&dbaseDef);
    rs_appendRecord(2);
    rs_appendRecord(2);
    CHECK_TRUE(BDB_changeValue(0, 0, 2, 2));
}


TEST(OpenDbase, changeValueOnReferenceColumnToNonExistingRecordInRefTableFails) {
    BDB_openDataBase(&dbaseDef);
    rs_appendRecord(2);
    rs_appendRecord(2);
    CHECK_TRUE(BDB_changeValue(0, 0, 2, 2));
    CHECK_FALSE(BDB_changeValue(0, 0, 2, 1));
}


TEST(OpenDbase, changeValueOnReferenceColumn_ToExistingRecordInRefTable_SetsValueToNumRecords) {
    BDB_openDataBase(&dbaseDef);
    rs_appendRecord(2);
    rs_appendRecord(2);
    CHECK_TRUE(BDB_changeValue(0, 0, 2, 5));
    BYTES_EQUAL(3, BDB_getNumRecords(2));
    BYTES_EQUAL(2, BDB_getValue(0,0,2));
}


TEST(OpenDbase, deletingALowerRecordInTheRefTableAdjustssRecordReference) {
    BDB_openDataBase(&dbaseDef);
    rs_appendRecord(2); // table 2, record 1
    rs_appendRecord(2); // table 2, record 2
    CHECK_TRUE(BDB_setValue(0, 0, 2, 1)); // references table 2, record 1
    BDB_deleteRecord(2, 0); // 0,0,2 should now referece table 2, record 0
    BYTES_EQUAL(0, BDB_getValue(0, 0, 2));
}


TEST(OpenDbase, insertingALowerRecordInTheRefTableAdjustsRecordReference) {
    BDB_openDataBase(&dbaseDef);
    rs_appendRecord(2); // table 2, record 1
    rs_appendRecord(2); // table 2, record 2
    CHECK_TRUE(BDB_setValue(0, 0, 2, 1)); // references table 2, record 1
    BDB_insertRecordAfter(2, 0); // 0,0,2 should now referece table 2, record 2
    BYTES_EQUAL(2, BDB_getValue(0, 0, 2));
}


// BDB_COLUMN_VIRTUAL


TEST(OpenDbase, setValueFailsOnAVirtualColumn) {
    BDB_openDataBase(&dbaseDef);
    CHECK_FALSE(BDB_setValue(0, 0, 3, 0));
}


TEST(OpenDbase, changeValueFailsOnAVirtualColumn) {
    BDB_openDataBase(&dbaseDef);
    CHECK_FALSE(BDB_changeValue(0, 0, 4, 1));
}


TEST(OpenDbase, getValueOnAVirtualColumn_ReturnsValueOfTheReferencedTableColumn) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(2, 0, 1, 8);
    BYTES_EQUAL(8, BDB_getValue(0, 0, 4));
}


// BDB_COLUMN_CHAR


TEST(OpenDbase, getValueOnCharReturnsNumericValue) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(2, 0, 1, 12);
    BYTES_EQUAL(12, BDB_getValue(2, 0, 1));
}


TEST(OpenDbase, writeValueSetsCharFromCharset) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(2, 0, 1, 12);
    BYTES_EQUAL(1, BDB_writeValue(2, 0, 1, 0));
    BYTES_EQUAL(charSet[12], BDB_getWriteBuffer()[0]);
}


// BDB_COLUMN_STRING


TEST(OpenDbase, setValueOnStringColumnFails) {
    BDB_openDataBase(&dbaseDef);
    CHECK_FALSE(BDB_setValue(2, 0, 5, 0));
}


TEST(OpenDbase, changeValueOnStringColumnFails) {
    BDB_openDataBase(&dbaseDef);
    CHECK_FALSE(BDB_changeValue(2, 0, 5, 1));
}


TEST(OpenDbase, getValueOnStringColumnFails) {
    BDB_openDataBase(&dbaseDef);
    LONGS_EQUAL(0xFFFF, BDB_getValue(2, 0, 5));
}


TEST(OpenDbase, writeValueOnAStringColumnWritesAllChars) {
    BDB_openDataBase(&dbaseDef);
    BDB_writeValue(2, 0, 5, 0);
    STRNCMP_EQUAL("    ", BDB_getWriteBuffer() , 4);
}


// BDB_COLUMN_DECIMAL


TEST(OpenDbase, setValueToNonStepValueFails) {
    BDB_openDataBase(&dbaseDef);
    CHECK_FALSE(BDB_setValue(1, 0, 4, 238));
}


TEST(OpenDbase, setValueToStepValueSucceeds) {
    BDB_openDataBase(&dbaseDef);
    CHECK_TRUE(BDB_setValue(1, 0, 4, 25));
}


TEST(OpenDbase, changeValueBy1IncreasesValueByStep) {
    BDB_openDataBase(&dbaseDef);
    CHECK_TRUE(BDB_changeValue(1, 0, 4, 1));
    LONGS_EQUAL(15, BDB_getValue(1, 0, 4));
}


TEST(OpenDbase, writeValue10_OnDecimalColumnWith1DecimalInsertsPointBeforeLastDigit) {
    BDB_openDataBase(&dbaseDef);
    BDB_writeValue(1, 0, 4, 0);
    STRNCMP_EQUAL(" 1.0", BDB_getWriteBuffer(), 4);
}


TEST(OpenDbase, writeValue0_OnDecimalColumnWith4DecimalsInserts0pt000) {
    BDB_openDataBase(&dbaseDef);
    BDB_writeValue(1, 0, 2, 0);
    STRNCMP_EQUAL("0.0000\n", BDB_getWriteBuffer(), 6);
}


TEST(OpenDbase, writeValue19_OnDecimalColumnWith4DecimalsInserts0tp00) {
    BDB_openDataBase(&dbaseDef);
    BDB_setValue(1, 0, 2, 19);
    BDB_writeValue(1, 0, 2, 0);
    STRNCMP_EQUAL("0.0019\n", BDB_getWriteBuffer(), 6);
}


/* TODO:
 *
 * remaining columnTypes:
 *  BDB_COLUMN_INT_ZEROVAL
 *  BDB_COLUMN_STRING_LIST
 *  BDB_COLUMN_SYMBOL_LIST
 *  BDB_COLUMN_STRING_LISTS
 *
 * remaining writeColumnValue implementations for columnTypes
 *
 * writeRecord
 *
 * writeColumnValue -> start pos - end pos
 *
 * refactor: move API to bottom and all static functions to top
 */

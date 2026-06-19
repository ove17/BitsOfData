/* TestBitsOfData.cpp
 *
 * TODO:
 *  check if (all) tests also work if recId != 0 ?
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
    const uint8_t recordType = 1;
    BDB_setValue(1, 0, 0, recordType);
    BYTES_EQUAL(4, BDB_getNumRealColumns(1, 0));
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


TEST(UseDbase, setValueOnVariableRecDefChangesBuffer) {
    BDB_insertRecordAfter(1, 0);
    CHECK_TRUE(BDB_setValue(1, 0, 0, 0)); // set recordType to 0
    CHECK_TRUE(BDB_setValue(1, 1, 0, 1)); // set recordType to 1
    CHECK_TRUE(BDB_setValue(1, 0, 1, 4)); // min value is below that of recordType 1
    LONGS_EQUAL(4, BDB_getValue(1, 0, 1));
}


// parent/child functions


TEST(UseDbase, getParentTableOnAChildReturnsTableId) {
    BYTES_EQUAL(2, BDB_getParentTable(0));
    BYTES_EQUAL(2, BDB_getParentTable(1));
}


TEST(UseDbase, getParentRecordOnAnEmptyDbaseReturns0onTheFirstChildTable) {
    BYTES_EQUAL(0, BDB_getParentRecord(3));
}


TEST(UseDbase, getParentRecordOnAnEmptyDbaseReturns0xFFonAnotherChildTable) {
    BYTES_EQUAL(0xFF, BDB_getParentRecord(4));
}


TEST(UseDbase, setValueOnAChildColumnReturnsFalse) {
    CHECK_FALSE(BDB_setValue(2, 0, 5, 0));
}


TEST(UseDbase, changeValueOnAChildColumnReturnsFalse) {
    CHECK_FALSE(BDB_changeValue(2, 0, 5, 1));
}


TEST(UseDbase, insertRecordGeneratesValueForChildTableRecord) {
    BDB_insertRecordAfter(2, 0);
    BYTES_EQUAL(0, BDB_getParentRecord(3));
    BYTES_EQUAL(1, BDB_getParentRecord(4));
}


TEST(UseDbase, deleteRecordDestroysValueForChildTableRecord) {
    CHECK_TRUE(BDB_insertRecordAfter(2, 0));
    BYTES_EQUAL(2, BDB_getNumRecords(2));
    BDB_setValue(0, 0, 2, 1); // allow 2, 0 to be deleted
    BDB_setValue(2, 1, 6, 1); // allow 2, 0 to be deleted
    CHECK_TRUE(BDB_deleteRecord(2, 0)); // delete 1st record
    BYTES_EQUAL(0, BDB_getParentRecord(4)); // table 1 is used by record 0
    BYTES_EQUAL(0xFF, BDB_getParentRecord(3)); // table 0 is not in use
}


TEST(UseDbase, repeatedInsertAndDeleteRecordsWorkForChildTableRecord) {
    for (uint8_t i = 0; i < 10 ; i++) {
        CHECK_TRUE(BDB_insertRecordAfter(2, 0));
        BDB_setValue(0, 0, 2, 1); // allow 2, 0 to be deleted
        BDB_setValue(2, 1, 6, 1); // allow 2, 0 to be deleted
        CHECK_TRUE(BDB_deleteRecord(2, 0)); // delete 1st record
    }
    BYTES_EQUAL(0, BDB_getParentRecord(4)); // table 1 is used by record 0
    BYTES_EQUAL(0xFF, BDB_getParentRecord(3)); // table 0 is not in use
}


TEST(UseDbase, openingExistingDbaseGeneratesChildTableRecord) {
    BDB_insertRecordAfter(2, 0);
    BDB_closeDataBase();
    CHECK_TRUE(BDB_openDataBase(&dbaseDef, NULL));
    BYTES_EQUAL(0, BDB_getParentRecord(3));
    BYTES_EQUAL(1, BDB_getParentRecord(4));
}


TEST(UseDbase, importTableSetsChildTableRecord) {
    static const uint16_t table2Data[] = {
        8, 3, 5, 40, 30, 6, 0,
        8, 3, 5, 40, 30, 4, 0
    };
    uint8_t numRecords = 2;
    BDB_importTable(2, table2Data, numRecords);
    BYTES_EQUAL(1, BDB_getParentRecord(4));
    BYTES_EQUAL(0, BDB_getParentRecord(6));
}


// changeValue


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


TEST(UseDbase, changeValueOnVariableRecDefChangesBuffer) {
    BDB_insertRecordAfter(1, 0);
    CHECK_TRUE(BDB_setValue(1, 0, 0, 0)); // set recordType to 0
    CHECK_TRUE(BDB_setValue(1, 1, 0, 1)); // set recordType to 1
    CHECK_TRUE(BDB_changeValue(1, 0, 1, 1)); // 150 + 1
    LONGS_EQUAL(151, BDB_getValue(1, 0, 1));
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
    BDB_syncTable(0);
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


// NOTE: an actual change is not required, an edit without change triggers a store
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
    uint16_t rec1_1[] = { 1, 85, 0, 2};
    CHECK_TRUE(BDB_setRecord(1, 1, rec1_1));
    CHECK_UINT16_ARRAY_EQUAL(rec1_1, BDB_getRecord(1, 1), 3);
    CHECK_UINT16_ARRAY_EQUAL(rec1_0, BDB_getRecord(1, 0), 4);
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


TEST(UseDbase, insertRecordAfterDoesNotAffectPreviousRecord) {
    static const uint16_t record0[] = {0,   0,   0,    0,   5, 0};
    BDB_setRecord(1, 0, record0);
    CHECK_TRUE(BDB_insertRecordAfter(1, 0));
    CHECK_UINT16_ARRAY_EQUAL(record0, BDB_getRecord(1, 0), 6);
}


TEST(UseDbase, insertRecordAfterDoesNotAffectNextRecord) {
    BDB_insertRecordAfter(1, 0);
    static const uint16_t record0[] = {0,   0,   0,    0,   5, 0};
    static const uint16_t record1[] = {0, 300, 100, 1024, 995, 2};
    BDB_setRecord(1, 0, record0);
    BDB_setRecord(1, 1, record1);
    BDB_insertRecordAfter(1, 0);
    CHECK_UINT16_ARRAY_EQUAL(record1, BDB_getRecord(1, 2), 6);
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


TEST(UseDbase, canRecordBeDeleted_ReturnsTrueIfItIsSelfReferencing) {
    CHECK_TRUE(BDB_insertRecordAfter(2, 0));
    BDB_setValue(0, 0, 2, 1); // allow 2, 0 to be deleted
    BDB_setValue(2, 1, 6, 1); // allow 2, 0 to be deleted
    CHECK_TRUE(BDB_canRecordBeDeleted(2, 0));
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


// importing table


TEST(UseDbase, importTableWithValidDataReturnsNumRecords) {
    static const uint16_t table1Data[] = {
        0,   0,   0,    0,   5, 0,
        0, 300, 100, 1024, 995, 2
    };
    uint8_t numRecords = 2;
    BYTES_EQUAL(numRecords, BDB_importTable(1, table1Data, numRecords));
    BYTES_EQUAL(numRecords, BDB_getNumRecords(1));
}


TEST(UseDbase, importTableWithValidDataSetsRecords) {
    static const uint16_t table1Data[] = {
        0,   0,   0,    0,   5, 0,
        0, 123,  50,  499,  30, 1,
        0, 300, 100, 1024, 500, 2
    };
    uint8_t numRecords = 3;
    uint8_t numColumns = 6;
    BDB_importTable(1, table1Data, numRecords);
    for (uint8_t rec = 0; rec < numRecords; rec++) {
        CHECK_UINT16_ARRAY_EQUAL(table1Data + rec * numColumns, BDB_getRecord(1, rec), 6);
        for (uint8_t col = 0; col < numColumns; col++) {
            LONGS_EQUAL(table1Data[numColumns * rec + col], BDB_getValue(1, rec, col));
        }
    }
}


TEST(UseDbase, importTableWith2recordsOverwrites3records) {
    BDB_insertRecordAfter(1, 0);
    BDB_insertRecordAfter(1, 0);
    uint16_t record0[] = {0, 1,  50,  499,  30, 1};
    uint16_t record1[] = {0, 2,  50,  499,  30, 1};
    uint16_t record2[] = {0, 3,  50,  499,  30, 1};
    BDB_setRecord(1, 0, record0);
    BDB_setRecord(1, 1, record1);
    BDB_setRecord(1, 2, record2);
    BYTES_EQUAL(3, BDB_getNumRecords(1));
    static const uint16_t table1Data[] = {
        0,   0,   0,    0,   5, 0,
        0, 300, 100, 1024, 995, 2
    };
    uint8_t numRecords = 2;
    BYTES_EQUAL(numRecords, BDB_importTable(1, table1Data, numRecords));
    BYTES_EQUAL(numRecords, BDB_getNumRecords(1));
    BYTES_EQUAL(2, BDB_getNumRecords(1));
    BYTES_EQUAL(0, BDB_getValue(1, 0, 1));
    BYTES_EQUAL(300, BDB_getValue(1, 1, 1));
}


TEST(UseDbase, importTableReturnsRecordIdOfDataUnderMinValue) {
    static const uint16_t table1Data[] = {
        0,   0,   0,    0,   5, 0,
        0,   0,   0,    0,   4, 0,
        0, 300, 100, 1024, 995, 2
    };
    uint8_t numRecords = 3;
    BYTES_EQUAL(1, BDB_importTable(1, table1Data, numRecords));
}


TEST(UseDbase, importTableReturnsRecordIdOfDataOverMaxValue) {
    static const uint16_t table1Data[] = {
        0,   0,   0,    0,   5, 0,
        0,   0, 101,    0,   5, 0,
        0, 300, 100, 1024, 995, 2
    };
    uint8_t numRecords = 3;
    BYTES_EQUAL(1, BDB_importTable(1, table1Data, numRecords));
}


TEST(UseDbase, importTableReturnsRecordIdOfDataWithWrongStep) {
    static const uint16_t table1Data[] = {
        0,   0,   0,    0,   5, 0,
        0,   0,   0,    0, 178, 0,
        0, 300, 100, 1024, 995, 2
    };
    uint8_t numRecords = 3;
    BYTES_EQUAL(1, BDB_importTable(1, table1Data, numRecords));
}


TEST(UseDbase, importTableReturnsRecordIdOfDataThatReferencesNonExistingRecord) {
    rs_appendRecord(2);
    rs_appendRecord(2);
    static const uint16_t table0Data[] = {
        3,    0,   0,  0,
        3,    0,   3,  0,
        8, 4000,   2, 10,
    };
    uint8_t numRecords = 3;
    BYTES_EQUAL(1, BDB_importTable(0, table0Data, numRecords));
}


// BDB_COLUMN_INTEGER


TEST(UseDbase, writeValueOf4DigitIntReturnsStringOfLength4) {
    BYTES_EQUAL(4, BDB_writeValue(0, 0, 1, 0));
    MEMCMP_EQUAL("1234", BDB_getWriteBuffer(), 4);
}


// where maxDigits == numDigits of .maxValue
TEST(UseDbase, writeValueOf1DigitIntReturnsStringOfMaxLength) {
    BDB_setValue(1, 0, 3, 9);
    BYTES_EQUAL(4, BDB_writeValue(1, 0, 3, 0));
    MEMCMP_EQUAL("   9", BDB_getWriteBuffer(), 4);
}


// BDB_COLUMN_PERCENTAGE


TEST(UseDbase, writeValueOfPercentageReturnsPercentage) {
    BDB_setValue(1, 0, 0, 1);
    BYTES_EQUAL(10, BDB_getValue(1, 0, 3));
    BYTES_EQUAL(3, BDB_writeValue(1, 0, 3, 0));
    STRNCMP_EQUAL(" 40", BDB_getWriteBuffer(), 3);
}


// BDB_COLUMN_INT_STEP


TEST(UseDbase, setValueOfIntStepToInvalidValueFails) {
    CHECK_FALSE(BDB_setValue(2, 0, 0, 15));
}


TEST(UseDbase, changeValueBy1IncreasesIntStepValueByStep) {
    BDB_setValue(2, 0, 0, 40);
    CHECK_TRUE(BDB_changeValue(2, 0, 0, 1));
    LONGS_EQUAL(48, BDB_getValue(2, 0, 0));
}


TEST(UseDbase, writeValueOfIntStepReturnsStringWithoutLeading0) {
    BDB_setValue(2, 0, 0, 64);
    BYTES_EQUAL(3, BDB_writeValue(2, 0, 0, 0));
    MEMCMP_EQUAL(" 64", BDB_getWriteBuffer(), 3);
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
// Make sure no record references record 0:
    BDB_setValue(2, 0, 6, 1);
    BDB_setValue(2, 1, 6, 1);
    BDB_setValue(2, 2, 6, 1);
    CHECK_TRUE(BDB_setValue(0, 0, 2, 1)); // references table 2, record 1
    CHECK_TRUE(BDB_deleteRecord(2, 0)); // should now reference table 2, record 0
    BYTES_EQUAL(0, BDB_getValue(0, 0, 2));
}


TEST(UseDbase, insertingALowerRecordInTheRefTableAdjustsRecordReference) {
    rs_appendRecord(2); // table 2, record 1
    rs_appendRecord(2); // table 2, record 2
    CHECK_TRUE(BDB_setValue(0, 0, 2, 1)); // references table 2, record 1
    BDB_insertRecordAfter(2, 0); // 0,0,2 should now reference table 2, record 2
    BYTES_EQUAL(2, BDB_getValue(0, 0, 2));
}


TEST(UseDbase, writeValueOfRefColumnWritesReferredString) {
    uint16_t rec2[] = { 184, 12, 16, 0, 28, 3, 0};
    CHECK_TRUE(BDB_setRecord(2, 0, rec2));
    CHECK_TRUE(BDB_setValue(0, 0, 2, 0)); // references table 2, record 0
    BDB_writeValue(0, 0, 2, 0);
    STRNCMP_EQUAL("AE Q", BDB_getWriteBuffer() , 4);
}


TEST(UseDbase, changeValueOnRefColumnIsLimitedToActualNumRecords) {
    BDB_insertRecordAfter(2, 0);
    BDB_insertRecordAfter(2, 0);
    CHECK_TRUE(BDB_setValue(0, 0, 2, 0));
    CHECK_TRUE(BDB_changeValue(0, 0, 2, 1));
    CHECK_TRUE(BDB_changeValue(0, 0, 2, 1));
    CHECK_FALSE(BDB_changeValue(0, 0, 2, 1));
}


// BDB_COLUMN_SELF_REFERENCE



TEST(UseDbase, setValueOfSelfReferenceColumn_ToExistingRecord_InRefTableSucceeds) {
    CHECK_TRUE(BDB_setValue(2, 0, 6, 0));
}


TEST(UseDbase, setValueOfSelfReferenceColumnToNonExistingRecordInRefTableFails) {
    CHECK_FALSE(BDB_setValue(2, 0, 6, 1));
}


TEST(UseDbase, changeValueOnSelfReferenceColumnToExistingRecordInRefTableSucceeds) {
    rs_appendRecord(2);
    rs_appendRecord(2);
    CHECK_TRUE(BDB_changeValue(2, 0, 6, 2));
}


TEST(UseDbase, changeValueOnSelfReferenceColumnToNonExistingRecordInRefTableFails) {
    rs_appendRecord(2);
    rs_appendRecord(2);
    CHECK_TRUE(BDB_changeValue(2, 0, 6, 2));
    CHECK_FALSE(BDB_changeValue(2, 0, 6, 1));
}


TEST(UseDbase, deletingALowerRecordInTheSelfRefTableAdjustssRecordReference) {
    rs_appendRecord(2); // table 2, record 1
    rs_appendRecord(2); // table 2, record 2
// make sure no record references T2 record 0:
    CHECK_TRUE(BDB_setValue(2, 0, 6, 1));
    CHECK_TRUE(BDB_setValue(2, 1, 6, 1));
    CHECK_TRUE(BDB_setValue(2, 2, 6, 1));
    CHECK_TRUE(BDB_setValue(0, 0, 2, 1));
    CHECK_TRUE(BDB_deleteRecord(2, 0));
    BYTES_EQUAL(0, BDB_getValue(2, 0, 6)); // should now reference table 2, record 0
}


TEST(UseDbase, insertingALowerRecordInTheSelfRefTableAdjustsRecordReference) {
    rs_appendRecord(2); // table 2, record 1
    rs_appendRecord(2); // table 2, record 2
    CHECK_TRUE(BDB_setValue(2, 0, 6, 1)); // references table 2, record 1
    BDB_insertRecordAfter(2, 0); // should now reference table 2, record 2
    BYTES_EQUAL(2, BDB_getValue(2, 0, 6));
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


TEST(UseDbase, writeValueOnAVirtualColumn_WritesValueOfTheReferencedTableColumn) {
    BDB_setValue(1, 0, 0, 1); // change record definition
    BDB_setValue(2, 0, 1, 13); // value of referenced table, column
    BYTES_EQUAL(1, BDB_writeValue(1, 0, 4, 0));
    BYTES_EQUAL(charSet[13], BDB_getWriteBuffer()[0]);
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
    CHECK_FALSE(BDB_setValue(2, 0, 7, 0));
}


TEST(UseDbase, changeValueOnStringColumnFails) {
    CHECK_FALSE(BDB_changeValue(2, 0, 7, 1));
}


TEST(UseDbase, getValueOnStringColumnFails) {
    LONGS_EQUAL(0xFFFF, BDB_getValue(2, 0, 7));
}


TEST(UseDbase, writeValueOnAStringColumnWritesAllChars) {
    uint16_t rec2[] = { 184, 12, 16, 0, 28, 3, 0};
    CHECK_TRUE(BDB_setRecord(2, 0, rec2));
    BDB_writeValue(2, 0, 7, 0);
    STRNCMP_EQUAL("AE Q", BDB_getWriteBuffer() , 4);
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


TEST(UseDbase, writeValueOfTableColumnReturnsNumberOfRecords) {
    BDB_insertRecordAfter(0, 0);
    BDB_insertRecordAfter(0, 0);
    BDB_setValue(2, 0, 5, 0); // table 0
    BDB_writeValue(2, 0, 5, 0);
    STRNCMP_EQUAL(" 3", BDB_getWriteBuffer(), 1);
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
        if (txt[length] == '\0') return length;
    }
}


static const char* texts[] = {"yes", "zero", "no", " {&0} {&1}{&2}{&3}{&4}-{&7}", " {^7} "};


static uint8_t getTxt(const char** txtPtr, const uint8_t txtId) {
    if (txtPtr != NULL) { // in case of length-only call
        *txtPtr = texts[txtId];
    }
    return length(texts[txtId]);
}


// BDB_COLUMN_TXT_LIST


TEST(UseDbaseWithTxt, writeValue_onTxtListColumn_returnsTextFromFirstId) {
    BDB_openDataBase(&dbaseDef, getTxt);
    BDB_setValue(1, 0, 5, 0);
    BDB_writeValue(1, 0, 5, 0);
    STRNCMP_EQUAL("zero", BDB_getWriteBuffer(), 4); // returns texts[1], although value = 0 !
}


TEST(UseDbaseWithTxt, writeValue_onTxtListColumn_fillsUpToLengthOfLongestTxtInList) {
    BDB_openDataBase(&dbaseDef, getTxt);
    BDB_setValue(1, 0, 5, 1);
    BDB_writeValue(1, 0, 5, 0);
    STRNCMP_EQUAL("no  \0", BDB_getWriteBuffer(), 5);
}


// writeHeader


TEST(UseDbaseWithTxt, writeheaderOnChildCanDisplayParentColumn) {
    // NOTE: format is texts[4]
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec2[] = { 184, 13, 12, 14, 15, 3, 0 };
    CHECK_TRUE(BDB_setRecord(2, 0, rec2));
    BDB_writeHeader(0, 0);
    STRNCMP_EQUAL(" BACD ", BDB_getWriteBuffer(), 6);
}


// writeRecord


TEST(UseDbaseWithTxt, writeRecordReturnsStringForEntireRecord) {
    // NOTE: format is texts[3]
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec2[] = { 184, 12, 13, 14, 15, 3, 0 };
    CHECK_TRUE(BDB_setRecord(2, 0, rec2));
    BDB_writeRecord(2, 0);
    STRNCMP_EQUAL(" 184 ABCD-ABCD      ", BDB_getWriteBuffer(), 20);
}


TEST(UseDbaseWithTxt, writeRecordWithFormatWorksWithAndWithoutLeading0) {
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec1[] = { 0, 12, 15, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 0, rec1));
    BDB_writeRecordWithFormat(1, 0, "{&1}-{o&1}-{&3}\0");
    STRNCMP_EQUAL(" 12-012- 753        ", BDB_getWriteBuffer(), 20);
}


TEST(UseDbaseWithTxt, writeRecordWithFormatWorksOnCol0WithAndWithoutLeading0) {
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec2[] = { 16, 12, 13, 14, 15, 3, 0 };
    CHECK_TRUE(BDB_setRecord(2, 0, rec2));
    BDB_writeRecordWithFormat(2, 0, "{&0}-{o&0}-{&0}\0");
    STRNCMP_EQUAL(" 16-016- 16         ", BDB_getWriteBuffer(), 20);
}


TEST(UseDbaseWithTxt, writeRecordWithFormatWorksOnDecimalWithAndWithoutLeading0) {
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec1[] = { 0, 12, 15, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 0, rec1));
    BDB_writeRecordWithFormat(1, 0, "{&4}-{o&4}-{&2}\0");
    STRNCMP_EQUAL(" 7.5-07.5-0.0015    ", BDB_getWriteBuffer(), 20);
}


TEST(UseDbaseWithTxt, writeRecordWithIfWritesFirstOptionWhenTrue) {
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec1[] = { 0, 0, 15, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 0, rec1));
    BDB_writeRecordWithFormat(1, 0, "|{?&1==$0}n/a{:}S{o&1}{;}|\0");
    STRNCMP_EQUAL("|n/a|               ", BDB_getWriteBuffer(), 20);
}


TEST(UseDbaseWithTxt, writeRecordWithIfWritesSecondOptionWhenFalse) {
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec1[] = { 0, 5, 15, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 0, rec1));
    BDB_writeRecordWithFormat(1, 0, "|{?&1==$0}n/a{:}S{o&1}{;}|\0");
    STRNCMP_EQUAL("|S005|              ", BDB_getWriteBuffer(), 20);
}


TEST(UseDbaseWithTxt, writeRecordWithIfAndGEoperatorWorks) {
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec1[] = { 0, 5, 15, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 0, rec1));
    BDB_writeRecordWithFormat(1, 0, "{?&1>=$5}{o&1}{:}F{;}\0");
    STRNCMP_EQUAL("005                 ", BDB_getWriteBuffer(), 20);
}


TEST(UseDbaseWithTxt, writeRecordWithIfAndGToperatorWorks) {
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec1[] = { 0, 5, 15, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 0, rec1));
    BDB_writeRecordWithFormat(1, 0, "{?&1>$5}{o&1}{:}F{;}\0");
    STRNCMP_EQUAL("F                   ", BDB_getWriteBuffer(), 20);
}


TEST(UseDbaseWithTxt, writeRecordWithRecordIdWorks) {
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec1[] = { 0, 5, 15, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 0, rec1));
    BDB_writeRecordWithFormat(1, 0, "{#}\0");
    STRNCMP_EQUAL(" 0                  ", BDB_getWriteBuffer(), 20);
}


TEST(UseDbaseWithTxt, writeRecordWithRecordIdAndLeading0Works) {
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec1[] = { 0, 5, 15, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 0, rec1));
    BDB_writeRecordWithFormat(1, 0, "{o#}\0");
    STRNCMP_EQUAL("00                  ", BDB_getWriteBuffer(), 20);
}


TEST(UseDbaseWithTxt, writeRecordWithRecordIdPlusWorks) {
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec1[] = { 0, 5, 15, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 0, rec1));
    BDB_writeRecordWithFormat(1, 0, "{o#+}\0");
    STRNCMP_EQUAL("01                  ", BDB_getWriteBuffer(), 20);
}


TEST(UseDbaseWithTxt, writeRecordWithIfAndSecondColumnLEWorks) {
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec1[] = { 0, 5, 5, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 0, rec1));
    BDB_writeRecordWithFormat(1, 0, "{?&1>&2}>{:}<={;}\0");
    STRNCMP_EQUAL("<=", BDB_getWriteBuffer(), 2);
}


TEST(UseDbaseWithTxt, writeRecordWithIfAndSecondColumnGTWorks) {
    BDB_openDataBase(&dbaseDef, getTxt);
    uint16_t rec1[] = { 0, 6, 5, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 0, rec1));
    BDB_writeRecordWithFormat(1, 0, "{?&1>&2}>{:}<={;}\0");
    STRNCMP_EQUAL("> ", BDB_getWriteBuffer(), 2);
}


TEST(UseDbaseWithTxt, writeRecordWithIfAndRecordIdGTWorks) {
    BDB_openDataBase(&dbaseDef, getTxt);
    BDB_insertRecordAfter(1, 0);
    BDB_insertRecordAfter(1, 0);
    uint16_t rec1[] = { 0, 3, 5, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 2, rec1));
    BDB_writeRecordWithFormat(1, 2, "{?&1>#}>{:}<={;}\0");
    STRNCMP_EQUAL("> ", BDB_getWriteBuffer(), 2);
}


TEST(UseDbaseWithTxt, writeRecordWithIfAndRecordIdLEWorks) {
    BDB_openDataBase(&dbaseDef, getTxt);
    BDB_insertRecordAfter(1, 0);
    BDB_insertRecordAfter(1, 0);
    uint16_t rec1[] = { 0, 2, 5, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 2, rec1));
    BDB_writeRecordWithFormat(1, 2, "{?&1>#}>{:}<={;}\0");
    STRNCMP_EQUAL("<=", BDB_getWriteBuffer(), 2);
}


TEST(UseDbaseWithTxt, writeRecordWithIfAndRecordIdLTplusWorks) {
    BDB_openDataBase(&dbaseDef, getTxt);
    BDB_insertRecordAfter(1, 0);
    BDB_insertRecordAfter(1, 0);
    uint16_t rec1[] = { 0, 2, 5, 753, 75, 0 };
    CHECK_TRUE(BDB_setRecord(1, 2, rec1));
    BDB_writeRecordWithFormat(1, 2, "{?&1<#+}<{:}>={;}\0");
    STRNCMP_EQUAL("< ", BDB_getWriteBuffer(), 2);
}


TEST(UseDbaseWithTxt, writeRecordWithAsteriskShowsTotalNumberOfRecords) {
    BDB_openDataBase(&dbaseDef, getTxt);
    BDB_insertRecordAfter(1, 0);
    BDB_insertRecordAfter(1, 0);
    BDB_writeRecordWithFormat(1, 0, "{#+}/{o*}\0");
    STRNCMP_EQUAL(" 1/03", BDB_getWriteBuffer(), 5);
}

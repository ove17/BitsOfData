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

extern "C" {
    #include "EeHw.h"
    #include "EeHwX86.h"
    #include "BitsOfData.h"
    #include "RecordStore.h"
}


TEST_GROUP(OpenDbase) {
    void setup() {
        eeClear();
    }

    void teardown() {
        BDB_closeDataBase();
    }
};

static const uint8_t maxNumRecordsInTable2 = 15;

static const BDB_recordT recordDef = {
    .numColumns = 3,
    .columns = {
        {.maxValue = 8, .defaultVal = 7, .minValue = 3},
        {.maxValue = 4000, .defaultVal = 1234},
        {.colType = BDB_COLUMN_REFERENCE, .refTable = 2, .maxValue = maxNumRecordsInTable2 - 1,},
    },
};
static const BDB_recordT recordDefs0[] = {recordDef};
static const BDB_tableT table0 = {
    .maxNumRecords = 10,
    .numRecordDefs = 1,
    .recordDefs = recordDefs0
};

enum {
    RECORD_TYPE_0,
    RECORD_TYPE_1,
    NUM_RECORD_TYPES
};


static const BDB_columnT recordTypeColumn = {
    .colType = BDB_COLUMN_RECORD_TYPE,
    .minValue = 0,
    .maxValue = NUM_RECORD_TYPES - 1,
    .defaultVal = 0
};

static const BDB_recordT recordDef0 = {
    .numColumns = 4,
    .columns = {
        recordTypeColumn,
        {.maxValue = 300, .defaultVal = 150},
        {.maxValue = 100 },
        {.maxValue = 1024, .defaultVal = 255},
    },
};
static const BDB_recordT recordDef1 = {
    .numColumns = 3,
    .columns = {
        recordTypeColumn,
        {.maxValue = 85, .defaultVal = 45, .minValue = 5},
        {.colType = BDB_COLUMN_REFERENCE, .refTable = 2, .maxValue = maxNumRecordsInTable2 - 1,},
    },
};
static const BDB_recordT recordDefs1[] = {recordDef0, recordDef1};

static const BDB_tableT table1 = {
    .maxNumRecords = 20,
    .numRecordDefs = NUM_RECORD_TYPES,
    .recordDefs = recordDefs1
};


static const BDB_recordT recordDef2 = {
    .numColumns = 1,
    .columns = {
        {.maxValue = 255,},
    },
};
static const BDB_recordT recordDefs2[] = {recordDef2};
static const BDB_tableT table2 = {
    .maxNumRecords = maxNumRecordsInTable2,
    .numRecordDefs = 1,
    .recordDefs = recordDefs2
};


static const BDB_tableT tables[] = { table0, table1, table2};
static const BDB_dbaseDefT dbaseDef = {
    .numTables = 3,
    .tables = tables
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


TEST(OpenDbase, newTableHas1recordWithDefaultColValues) {
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


// NOTE: an actual change is not required, just an edit
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



/* TODO:
 *
 * 6 charColumn
 * 6 stringColumn
 * 7 virtualColumn
 *
 * 8 (sPrintValue)
 * 9 sPrintRecord
 * 10 all recordDefs
 *
 * import/export table
 * import/export record
 *      setRecord ipv setRawRecord ?
 *
 */

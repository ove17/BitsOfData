// TestRecordTables.cpp

/*
 * TODO: overweeg stricte bewaking eeAddress in STM mbv assert()
 *
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
    #include <assert.h>
    #include "EeHw.h"
    #include "EeHwX86.h"
    #include "RecordStore.h"
}


static const uint8_t numParamsPerTable = 4;
static const uint8_t numBytesPerTable = 6;


/* Each line holds for 1 table:
 * startRecordList, startDataArea, maxRecordSize, maxNumRecords
 * NOTE: start addresses need to be offset, based on number of tables
 */
static uint16_t validTableCatalog[] = {
       0,    4,   3,  4,  // recordIndexSize = 4,  recordDataSize = 12
      16,   24,   5,  8,  // recordIndexSize = 8,  recordDataSize = 40
      64,   65,  18,  1,  // recordIndexSize = 1,  recordDataSize = 18
      83,   94,  69, 11,  // recordIndexSize = 11, recordDataSize = 759
     853,  860,   9,  7,  // recordIndexSize = 7,  recordDataSize = 63
     923                  // virtual
};


static void createMockTableCatalog(const uint8_t numTables) {
    assert(numTables <= 5);
    uint16_t address = 0;
    eeWriteUint8(address, numTables);
    address++;
    uint16_t dataOffset = numBytesPerTable * numTables + 1;
    eeWriteUint8(dataOffset, 0); // end of recordTableList
    for (uint8_t table=0; table < numTables; table++) {
        uint8_t dataIndex = numParamsPerTable * table;
        dataOffset++;
        uint16_t recordIndexAddress = dataOffset + validTableCatalog[dataIndex];
        // start tableCatalog:
        eeWriteUint16(address, recordIndexAddress);
        // tableCatalog closing byte:
        eeWriteUint8(dataOffset + validTableCatalog[dataIndex + 1], 0);
        dataOffset++;
        uint16_t recordDataAddress = dataOffset + validTableCatalog[dataIndex + 1];
        // start recordData:
        eeWriteUint16(address+2, recordDataAddress);
        // recordData closing byte:
        eeWriteUint8(dataOffset + validTableCatalog[dataIndex+numParamsPerTable], 0);
        // recordSize:
        eeWriteUint8(address+4, (uint8_t)validTableCatalog[dataIndex + 2]);
        // maxNumRecords:
        eeWriteUint8(address+5, (uint8_t)validTableCatalog[dataIndex + 3]);
        address += numBytesPerTable;
    }
}



TEST_GROUP(OpenRecordStore) {
    void setup() {
        eeClear();
    }

    void teardown() {
        rs_closeTableCatalog();
    }
};


TEST(OpenRecordStore, openTableCatalogReturnsFalseIfItDoesNotExistYet) {
    CHECK_FALSE(rs_tryToOpenRecordStore(2));
}


TEST(OpenRecordStore, openTableCatalogOnAValidListReturnsTrue) {
    createMockTableCatalog(5);
    CHECK_TRUE(rs_tryToOpenRecordStore(5));
}


// NOTE: implementation details exposed, but keep
TEST(OpenRecordStore, openTableCatalogReturnsFalseIfNumTablesIsInvalid) {
    createMockTableCatalog(5);
    eeWriteUint8(0, 4); // numTables != 5
    CHECK_FALSE(rs_tryToOpenRecordStore(5));
}


// NOTE: implementation details exposed, but keep
TEST(OpenRecordStore, openTableCatalogReturnsFalseIfRecordIndexAddressIsInvalid) {
    createMockTableCatalog(5);
    eeWriteUint8(1, 31); // recordIndexAddress[0] != 32
    CHECK_FALSE(rs_tryToOpenRecordStore(5));
}


// NOTE: implementation details exposed, but keep
TEST(OpenRecordStore, openTableCatalogReturnsFalseIfRecordDataAddressIsInvalid) {
    createMockTableCatalog(5);
    eeWriteUint8(9, 58); // RecordDataAddress[1] != 59
    CHECK_FALSE(rs_tryToOpenRecordStore(5));
}


// NOTE: implementation details exposed, but keep
TEST(OpenRecordStore, openTableCatalogReturnsFalseIfRecordIndexEndByteIsNot0) {
    createMockTableCatalog(5);
    eeWriteUint8(99, 1); // endRecordIndex[2] != 0
    CHECK_FALSE(rs_tryToOpenRecordStore(5));
}


// NOTE: implementation details exposed, but keep
TEST(OpenRecordStore, openTableCatalogReturnsFalseIfRecordDataEndByteIsNot0) {
    createMockTableCatalog(5);
    eeWriteUint8(964, 1); // endRecordData[5] != 0
    CHECK_FALSE(rs_tryToOpenRecordStore(5));
}


// NOTE: implementation details exposed, but keep
TEST(OpenRecordStore, openTableCatalogReturnsFalseIfTableCatalogEndByteIsNot0) {
    createMockTableCatalog(5);
    eeWriteUint8(31, 1);
    CHECK_FALSE(rs_tryToOpenRecordStore(5));
}


TEST(OpenRecordStore, openTableCatalogUsesExistingRecordsToDetermineNumRecords) {
    rs_tryToOpenRecordStore(1);
//dumpFakeEe(1, 10, numBytesPerTable);
    rs_createTable(5, 2);
    rs_commitTables();
//dumpFakeEe(1, 10, numBytesPerTable);
    rs_appendRecord(0); // 1st
    rs_appendRecord(0); // 2nd
    rs_appendRecord(0); // 3rd
    BYTES_EQUAL(3, rs_getNumRecords(0));
    rs_closeTableCatalog(); // "reboot"
    rs_tryToOpenRecordStore(1);
    BYTES_EQUAL(3, rs_getNumRecords(0));
}


// NOTE: implementation details exposed, but keep
TEST(OpenRecordStore, deleteTableCatalogCausesOpenRecordStoreToClearEe) {
    rs_tryToOpenRecordStore(1);
    rs_createTable(5, 2);
    rs_commitTables();
    rs_appendRecord(0); // 1st
    rs_appendRecord(0); // 2nd
    rs_appendRecord(0); // 3rd
    BYTES_EQUAL(3, rs_getNumRecords(0));
//dumpFakeEe(1, 7, numBytesPerTable);
    rs_deleteTableCatalog();
    LONGS_EQUAL(0, eeReadUint8(0));
    LONGS_EQUAL(8, eeReadUint8(1));
    rs_closeTableCatalog(); // "reboot"
    rs_tryToOpenRecordStore(1);
    LONGS_EQUAL(0xFF, eeReadUint8(0));
    LONGS_EQUAL(0xFF, eeReadUint8(1));
}




TEST_GROUP(CreateTables) {
    void setup() {
        eeClear();
    }

    void teardown() {
        rs_closeTableCatalog();
    }
};


TEST(CreateTables, createTableOnAnEmptyTableCatalogReturns0) {
    rs_tryToOpenRecordStore(3);
    BYTES_EQUAL(0, rs_createTable(5, 3));
}


TEST(CreateTables, createTableOnAnExistingTableCatalogReturnsItsTableId) {
    rs_tryToOpenRecordStore(3);
    rs_createTable(20, 5);
    BYTES_EQUAL(1, rs_createTable(1, 100));
    BYTES_EQUAL(2, rs_createTable(5, 3));
}


TEST(CreateTables, createTableThatFillsStorageSpaceCompletelySucceeds) {
    rs_tryToOpenRecordStore(2);
    BYTES_EQUAL(0, rs_createTable(32, 250)); // takes 8048 bytes, so 144 left
    BYTES_EQUAL(1, rs_createTable(1, 141)); // takes 144 bytes extra
//    dumpFakeEe(0, 256, 32);
}


TEST(CreateTables, createTableWhenThereIsNoStorageSpaceLeftFailsAssertion) {
    rs_tryToOpenRecordStore(2);
    rs_createTable(32, 250); // takes 8048 bytes, so 144 left
    disableAssertAddress();
    rs_createTable(1, 142); // takes 145 bytes extra - 1 too many
    CHECK_FALSE(wasAddressValid());
    enableAssertAddress();
}




TEST_GROUP(Records) {
    void setup() {
        eeClear();
        rs_tryToOpenRecordStore(1);
        rs_createTable(maxNumRecords, recordSize);
        rs_commitTables();
    }

    void teardown() {
        rs_closeTableCatalog();
    }

    void reboot() {
        rs_closeTableCatalog();
        rs_tryToOpenRecordStore(1);
    }

    void createRecords(const uint8_t table, const uint8_t numRecords) {
        for (uint8_t i=0; i < numRecords; i++) {
            rs_appendRecord(table);
        }
    }

    const uint8_t maxNumRecords = 10;
    const uint8_t recordSize = 2;
    const uint16_t recordIndexAddress = 1 + numBytesPerTable + 1;
    const uint16_t recordDataAddress = recordIndexAddress + maxNumRecords + 1;
};


// GET NUM RECORDS


TEST(Records, newlyCreatedTableHasNoRecords) {
    BYTES_EQUAL(0, rs_getNumRecords(0));
}


TEST(Records, getNumRecordsOnATableWith1RecordsReturns1) {
    rs_appendRecord(0);
    BYTES_EQUAL(1, rs_getNumRecords(0));
}


TEST(Records, getNumRecordsOnATableWithMaxNumRecordsReturnsMaxNumRecords) {
    createRecords(0, maxNumRecords);
    BYTES_EQUAL(maxNumRecords, rs_getNumRecords(0));
}


// APPEND RECORD


TEST(Records, appendRecordOnEmptyTableReturnsRecordIndex0) {
    BYTES_EQUAL(0, rs_appendRecord(0));
}


TEST(Records, appendRecordOnTableWith1RecordReturnsRecordIndex1) {
    rs_appendRecord(0);
    BYTES_EQUAL(1, rs_appendRecord(0));
}


TEST(Records, appendRecordOnATableBeforeMaxNumRecordsReturnsItsIndex) {
    createRecords(0, maxNumRecords-1);
    BYTES_EQUAL(maxNumRecords-1, rs_getNumRecords(0));
}


TEST(Records, appendRecordWhenTheMaxNumberOfRecordsWasReachedReturns0xFF) {
    createRecords(0, maxNumRecords);
    BYTES_EQUAL(0xFF, rs_appendRecord(0));
    BYTES_EQUAL(maxNumRecords, rs_getNumRecords(0));
}


TEST(Records, appendRecordCreatesANewRecordAfterTheLastOneAndReturnsItsIndex) {
    BYTES_EQUAL(0, rs_appendRecord(0));
    BYTES_EQUAL(1, rs_appendRecord(0));
    BYTES_EQUAL(2, rs_appendRecord(0));
}


// INSERT RECORD AFTER


TEST(Records, insertRecordAfterThe1stRecordReturns1) {
    createRecords(0, 2);
    BYTES_EQUAL(1, rs_insertRecordAfter(0, 0));
}


TEST(Records, insertRecordAfterThe4rdAndLastRecordReturns4) {
    createRecords(0, 4);
    BYTES_EQUAL(4, rs_insertRecordAfter(0, 3));
}


TEST(Records, insertRecordAfterWhenTheMaxNumberOfRecordsWasReachedReturns0xFF) {
    createRecords(0, maxNumRecords);
    BYTES_EQUAL(0xFF, rs_insertRecordAfter(0, 6));
}


TEST(Records, insertRecordAfterWhenThereAreMaxMinus1RecordsReturnsRecordId) {
    createRecords(0, maxNumRecords-1);
    BYTES_EQUAL(7, rs_insertRecordAfter(0, 6));
}


TEST(Records, insertRecordAfterIncreasesTheNumberOfRecordsByOne) {
    createRecords(0, maxNumRecords-1);
    rs_insertRecordAfter(0, 2);
    BYTES_EQUAL(maxNumRecords, rs_getNumRecords(0));
}


TEST(Records, insertRecordAfter_On2ndRecord_InsertsRecordIndex) {
    createRecords(0, 2);        // recordIndex: 0, 1
    uint8_t rec[3] = {1,0,0};   rs_setRawRecord(0, 0, rec);
    rec[0] = 2;                 rs_setRawRecord(0, 1, rec);
//dumpFakeEe(1, 7, numBytesPerTable);
    BYTES_EQUAL(1, rs_insertRecordAfter(0, 0)); // recordIndex: 0, 2, 1
//dumpFakeEe(1, 7, numBytesPerTable);
    BYTES_EQUAL(   1, *rs_getRawRecord(0, 0));
    BYTES_EQUAL(0xFF, *rs_getRawRecord(0, 1)); // the inserted record
    BYTES_EQUAL(   2, *rs_getRawRecord(0, 2));
}


// DELETE RECORD


TEST(Records, deleteRecordOnTheLastRecordReturnsTrue) {
    createRecords(0, 5);
    CHECK_TRUE(rs_deleteRecord(0, 4));
}


TEST(Records, deleteRecordOnTheLastRecordReducesNumRecordsBy1) {
    createRecords(0, 5);
    rs_deleteRecord(0, 4);
    BYTES_EQUAL(4, rs_getNumRecords(0));
}


TEST(Records, deleteRecordOnTheMiddleRecordShiftsLastRecord) {
    createRecords(0, 3);
    uint8_t rec[3] = {1,0,0};   rs_setRawRecord(0, 0, rec);
    rec[0] = 2;                 rs_setRawRecord(0, 1, rec);
    rec[0] = 3;                 rs_setRawRecord(0, 2, rec);
    CHECK_TRUE(rs_deleteRecord(0, 1));
    BYTES_EQUAL(1, *rs_getRawRecord(0, 0));
    BYTES_EQUAL(3, *rs_getRawRecord(0, 1));
}


TEST(Records, deleteRecordOnFullRecordIndexShiftsRecordIndex) {
    createRecords(0, maxNumRecords);
    uint8_t rec[3] = {1,0,0};
    rs_setRawRecord(0, 0, rec);
    for (uint8_t i = 1; i < maxNumRecords; i++) {
        rec[0] = i+1;
        rs_setRawRecord(0, i, rec);
    }
    CHECK_TRUE(rs_deleteRecord(0, 5));
    BYTES_EQUAL( 1, *rs_getRawRecord(0, 0));
    BYTES_EQUAL( 5, *rs_getRawRecord(0, 4));
    BYTES_EQUAL( 7, *rs_getRawRecord(0, 5));
    BYTES_EQUAL(10, *rs_getRawRecord(0, 8));
}


TEST(Records, deleteRecordOnTheFirstRecordShiftsAllRecords) {
    createRecords(0, maxNumRecords);
    uint8_t rec[3] = {1,0,0};
    rs_setRawRecord(0, 0, rec);
    for (uint8_t i = 1; i < maxNumRecords; i++) {
        rec[0] = i+1;
        rs_setRawRecord(0, i, rec);
    }
    CHECK_TRUE(rs_deleteRecord(0, 0));
    BYTES_EQUAL( 2, *rs_getRawRecord(0, 0));
    BYTES_EQUAL( 3, *rs_getRawRecord(0, 1));
    BYTES_EQUAL( 9, *rs_getRawRecord(0, 7));
    BYTES_EQUAL(10, *rs_getRawRecord(0, 8));
}


TEST(Records, deleteRecordIsPersistentOnReboot) {
    createRecords(0, maxNumRecords);
    CHECK_TRUE(rs_deleteRecord(0, 9));
    CHECK_TRUE(rs_deleteRecord(0, 3));
    CHECK_TRUE(rs_deleteRecord(0, 0));
    BYTES_EQUAL(7, rs_getNumRecords(0));
    reboot();
    BYTES_EQUAL(7, rs_getNumRecords(0));
}


TEST(Records, deleteAllRecordsLeaves0Records) {
    createRecords(0, 5);
    rs_deleteAllRecords(0);
    BYTES_EQUAL(0, rs_getNumRecords(0));
}


TEST(Records, deleteAllRecordsReturnsTheNumberOfDeletedRecords) {
    createRecords(0, maxNumRecords);
    BYTES_EQUAL(maxNumRecords, rs_deleteAllRecords(0));
}


TEST(Records, deleteAllRecordsMakesSpaceForNewRecords) {
    createRecords(0, 3);
    rs_deleteAllRecords(0);
    createRecords(0, maxNumRecords-1);
    BYTES_EQUAL(9, rs_appendRecord(0));
}



static const uint8_t numTables = 6;
static const uint8_t numRecords[] = {99,   8,  1, 147, 16, 250};
static const uint8_t recordSize[] = {30, 250, 18,  11,  1,   4};

static uint32_t seed = 12345;

static uint8_t nextByte() {
    seed = seed * 1103515245 + 12345;
    return (uint8_t)(seed >> 16);
}


TEST_GROUP(RandomDataWrite) {
    void setup() {
        eeClear();
        rs_tryToOpenRecordStore(6);
        for (uint8_t table = 0; table < numTables; table++) {
            rs_createTable(numRecords[table], recordSize[table]);
        }
        rs_commitTables();
        for (uint8_t table = 0; table < numTables; table++) {
            for (uint8_t record = 0; record < numRecords[table]; record++) {
                rs_appendRecord(table);
            }
        }
    }

    void teardown() {
        rs_closeTableCatalog();
    }
};


/* NOTE: this tests reading and writing of all records in a multi-table dbase
 *          that fills 100% of EE, so it implicitly tests the validity of the
 *          EE data model.
 */
TEST(RandomDataWrite, getRecordRetrievesExactlyWhatSetRecordWroteOnFullEeWrite) {
    uint32_t seed0 = (uint32_t)time(NULL);
    seed = seed0;
    for (uint8_t table = 0; table < numTables; table++) {
        uint8_t rawRecord[255];
        uint8_t recSize = recordSize[table];
        for (uint8_t record = 0; record < numRecords[table]; record++) {
            for (uint8_t i = 0; i < recSize; i++) {
                rawRecord[i] = nextByte();
            }
            rs_setRawRecord(table, record, rawRecord);
        }
    }
    char msg[64];
    seed = seed0;
    for (uint8_t table = 0; table < numTables; table++) {
        uint8_t rawRecord[255];
        uint8_t recSize = recordSize[table];
        for (uint8_t record = 0; record < numRecords[table]; record++) {
            for (uint8_t i = 0; i < recSize; i++) {
                rawRecord[i] = nextByte();
            }
            snprintf(msg, 64, "table=%u record=%u seed=%u", table, record, seed0);
            MEMCMP_EQUAL_TEXT(rawRecord, rs_getRawRecord(table, record), recSize, msg);
        }
    }
}

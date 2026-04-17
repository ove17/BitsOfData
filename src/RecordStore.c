/* RecordStore.c
 *
 * This module manages tables of fixed-size byte records stored in EEPROM.
 *
 * IMPLEMENTATION OVERVIEW:
 *
 * The TableCatalog in EE contains one entry (tableRow) per table. Each entry
 *  stores:
 *   - EEPROM address of the recordIndex
 *   - EEPROM address of the recordDataArea
 *   - fixed record size
 *   - maximum number of records
 *
 * A RAM copy of the TableCatalog is maintained, enriched with derived runtime
 *  state:
 *   - current number of active records per table
 *   - a pointer to the recordDataBuffer
 *   - a pointer to the recordInUseBitmap;
 *
 * RECORD INDEX:
 *
 * Each table maintains a recordIndex which is a compact list of record offsets
 * into the recordDataArea.
 *      - The recordIndex is terminated by 0xFF. All entries after the first
 *          0xFF are considered unused.
 *      - This structure allows fast lookup, insertion, and deletion of records
 *          by updating only the index.
 *
 * RECORD DATA AREA:
 *
 * The recordDataArea contains raw record storage.
 * Records are accessed indirectly via the recordIndex.
 */

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "RecordStore.h"
#include "EeHw.h"


// absolute maximum values:
#define MAX_NUM_TABLES 250
#define MAX_NUM_RECORDS 250
#define MAX_RECORD_SIZE 250

#define TABLE_CATALOG_ROW_SIZE (2 * sizeof(eeAddress_t) + 2 * sizeof(uint8_t))
#define ADDR_NUM_TABLES 0
#define ADDR_TABLE_CATALOG 1
#define RECORD_INDEX_OFFSET 0
#define RECORD_DATA_OFFSET 2
#define RECORD_SIZE_OFFSET 4
#define MAX_NUM_RECORDS_OFFSET 5

#define NO_RECORD_INDEX 0xFF
#define MAX_NUM_RECORDS_REACHED 0xFF
#define NO_FREE_RECORD_FOUND 0xFF


typedef struct {
    eeAddress_t recordIndexAddress;
    eeAddress_t recordDataAddress;
    uint8_t recordSize;
    uint8_t maxNumRecords;
    uint8_t numRecords;
    uint8_t* recordDataBuffer;
    uint8_t* recordInUseBitmap;
} tableDescriptorT;


static uint8_t NumTables = 0;
static tableDescriptorT* TableCatalog = NULL;


static void allocateTableCatalog(void);
static void allocateRecordDataBuffers(void);
static void allocateRecordInUseBitmap(void);
static void generateRecordInUseBitmap(void);
static void loadTableCatalog(void);
static bool isTableCatalogValid(void);
static void createTable(const uint8_t table,
                        const uint8_t maxNumRecords,
                        const uint8_t recordSize);
static void storeTableCatalog(void);
static uint8_t getNumRecordsFromIndex(tableDescriptorT* tableRow);
static bool recordIndexIsNotUsed(tableDescriptorT* tableRow,
                                 const uint8_t record);
static void storeNumTables(const uint8_t numTables);
static uint8_t loadNumTables(void);
static bool isTableSeparatorByte(const eeAddress_t eeAddress);

static eeAddress_t getDataAddress(tableDescriptorT* tableRow, const uint8_t record);
static uint16_t getDataAreaSize(tableDescriptorT* tableRow);

static uint8_t insertRecordAt(tableDescriptorT* tableRow,
                              const uint8_t index);
static uint8_t getRecordIndex(tableDescriptorT* tableRow,
                              const uint8_t record);
static void setRecordIndex(tableDescriptorT* tableRow,
                           const uint8_t record,
                           const uint8_t indexValue);
static void shiftRecordIndexUp(tableDescriptorT* tableRow,
                               const uint8_t record);
static void shiftRecordIndexDown(tableDescriptorT* tableRow,
                                 const uint8_t record);

static uint8_t getNextFreeRecord(tableDescriptorT* tableRow);
static void markRecordInUse(tableDescriptorT* tableRow,
                            const uint8_t record);
static void markRecordFree(tableDescriptorT* tableRow,
                           const uint8_t record);
static bool recordIsFree(tableDescriptorT* tableRow,
                         const uint8_t record);

static void assertTableExists(const uint8_t table);
static void assertRecordExists(const uint8_t table,
                               const uint8_t record);

//TODO: move to generic library
static uint8_t getByteIndex(const uint8_t record);
static uint8_t getBitMask(const uint8_t record);


bool rs_tryToOpenRecordStore(const uint8_t numTables) {
    assert(numTables > 0 && numTables <= MAX_NUM_TABLES);
    eeInit();
    NumTables = numTables;
    allocateTableCatalog();
    loadTableCatalog();
    if (isTableCatalogValid()) {
        allocateRecordDataBuffers();
        allocateRecordInUseBitmap();
        generateRecordInUseBitmap();
        return true; // TableCatalog is valid, start using it
    } else {
        eeClear(); // TableCatalog is not valid, so delete it from EE
        return false; // caller must use createTable to add tables
                      // and finaliseTableCatalog to start using it
    }
}


// NOTE: no error checking, system must have plenty of RAM
static void allocateTableCatalog(void) {
    TableCatalog = calloc(NumTables * TABLE_CATALOG_ROW_SIZE, sizeof(tableDescriptorT));
}


// Loads the tableCatalog from EE into RAM
static void loadTableCatalog(void) {
    if (loadNumTables() != NumTables) return;
    eeAddress_t address = ADDR_TABLE_CATALOG;
    tableDescriptorT* tableRow;
    for (uint8_t table = 0; table < NumTables; table++) {
        tableRow = &TableCatalog[table];
        tableRow->recordIndexAddress = eeReadUint16(address + RECORD_INDEX_OFFSET);
        tableRow->recordDataAddress =  eeReadUint16(address + RECORD_DATA_OFFSET);
        tableRow->recordSize =         eeReadUint8( address + RECORD_SIZE_OFFSET);
        tableRow->maxNumRecords =      eeReadUint8( address + MAX_NUM_RECORDS_OFFSET);
        tableRow->numRecords = getNumRecordsFromIndex(tableRow);
        address += TABLE_CATALOG_ROW_SIZE;
    }
}


/*
 * Returns true if the tableCatalog in RAM is valid, false if not.
 *
 * EEPROM serialization format of the TableCatalog.
 *
 * This format is intentionally stable and considered a low-level
 * persistent contract. Higher layers must not depend on it directly.
 *
 * Data storage format:
 *
 *  byte                                value
 *  ==============                      =========
 *  0                                   numTables (nT for short)
 *  1..nT*6                             tableList
 *  nT*6+1                              0 (end of tableList)
 *  nT*6+2                              table 1 recordIndex
 *  nT*6+2+mNR_t1                       0 (end of recordIndex_t1)
 *  nT*6+2+mNR_t1+1                     table 1 recordData
 *  nT*6+2+mNR_t1+1+mNR_t1*rS_t1        0 (end of recordData_t1)
 *  nT*6+2+mNR_t1+1+mNR_t1*rS_t1+1      table 2 recordIndex
 *   ...
 *  nT*6+2+sum(mNR)+nT+sum(mNR*rS)+nT   0 (end of recordData_tn)
 *
 * tableList entry consists of 6 (=TABLE_CATALOG_ROW_SIZE) bytes:
 *  byte0&1  recordIndexAddress
 *  byte2&3  recordDataAddress
 *  byte4    recordSize (rS for short)
 *  byte5    maxNumRecords (mNR for short)
 *
 */
static bool isTableCatalogValid(void) {
    if (loadNumTables() != NumTables) return false;
    eeAddress_t address = ADDR_TABLE_CATALOG + NumTables * TABLE_CATALOG_ROW_SIZE + 1;
    tableDescriptorT* tableRow;
    for (uint8_t table = 0; table < NumTables; table++) {
        tableRow = &TableCatalog[table];

        // check recordIndex start address:
        if (tableRow->recordIndexAddress != address) return false;
        // check recordIndex start byte:
        if (!isTableSeparatorByte(address-1)) return false;

        address += tableRow->maxNumRecords + 1;

        // check recordData start address:
        if ( tableRow->recordDataAddress != address) return false;
        // check recordData start byte:
        if (!isTableSeparatorByte(address-1)) return false;

        address += getDataAreaSize(tableRow) + 1;
    }
    // check last recordData closing byte:
    return isTableSeparatorByte(address-1);
}


static uint8_t getNumRecordsFromIndex(tableDescriptorT* tableRow) {
    for (uint8_t record = 0; record < tableRow->maxNumRecords; record++) {
        if (recordIndexIsNotUsed(tableRow, record)) {
            return record;
        }
    }
    return tableRow->maxNumRecords;
}


static bool recordIndexIsNotUsed(tableDescriptorT* tableRow,
                                 const uint8_t record) {
    eeAddress_t eeAddress = tableRow->recordIndexAddress + record;
    return eeReadUint8(eeAddress) == NO_RECORD_INDEX;
}


// only for testing - should never be used in production
void rs_closeTableCatalog(void) {
    if (TableCatalog) {
        for (uint8_t table=0; table < NumTables; table++) {
            if (TableCatalog[table].recordDataBuffer) {
                free(TableCatalog[table].recordDataBuffer);
            }
            TableCatalog[table].recordDataBuffer = NULL;

            if (TableCatalog[table].recordInUseBitmap) {
                free(TableCatalog[table].recordInUseBitmap);
            }
            TableCatalog[table].recordInUseBitmap = NULL;
        }
        free(TableCatalog);
    }
    TableCatalog = NULL;
    NumTables = 0;
}


// causes clearing of Ee on next openTableCatalog, e.g. for updating to next sw version
void rs_deleteTableCatalog(void) {
    storeNumTables(0);
}


// creates a table in the TableCatalog
uint8_t rs_createTable(const uint8_t maxNumRecords,
                       const uint8_t recordSize) {
    assert(maxNumRecords > 0 && maxNumRecords <= MAX_NUM_RECORDS);
    assert(recordSize > 0 && recordSize <= MAX_RECORD_SIZE);
    for (uint8_t table = 0; table < NumTables; table++) {
        if (TableCatalog[table].maxNumRecords == 0) {
            createTable(table, maxNumRecords, recordSize);
            return table;
        }
    }
    assert( 0 && "too many tables");
}


void rs_commitTables(void) {
    storeTableCatalog();
    allocateRecordDataBuffers();
    allocateRecordInUseBitmap();
}


// returns false if there was not enough EE space for the table
static void createTable(const uint8_t table,
                        const uint8_t maxNumRecords,
                        const uint8_t recordSize) {
    assertTableExists(table);
    eeAddress_t recordIndexAddress;
    tableDescriptorT* tableRow = &TableCatalog[table];
    if (table > 0) {
        tableDescriptorT* previousRow = &TableCatalog[table-1];
        recordIndexAddress = previousRow->recordDataAddress
                                            + getDataAreaSize(previousRow) + 1;
    } else {
        recordIndexAddress = NumTables * TABLE_CATALOG_ROW_SIZE + 2;
    }
    eeAddress_t recordDataAddress = recordIndexAddress + maxNumRecords + 1;
    eeAddress_t endByteAddress = recordDataAddress
                                    + (uint16_t)(recordSize * maxNumRecords);
    assertEeAddressExists(endByteAddress); // NOTE: use disableAssert() for testing
    tableRow->recordIndexAddress = recordIndexAddress;
    tableRow->recordDataAddress = recordDataAddress;
    tableRow->recordSize = recordSize;
    tableRow->maxNumRecords = maxNumRecords;
    tableRow->numRecords = 0;
}


static void storeTableCatalog(void) {
    storeNumTables(NumTables);
    eeAddress_t address = ADDR_TABLE_CATALOG;
    tableDescriptorT* tableRow = NULL;
    for (uint8_t table = 0; table < NumTables; table++) {
        tableRow = &TableCatalog[table];
        // recordTableList row:
        eeWriteUint16(address + RECORD_INDEX_OFFSET,    tableRow->recordIndexAddress);
        eeWriteUint16(address + RECORD_DATA_OFFSET,     tableRow->recordDataAddress);
        eeWriteUint8( address + RECORD_SIZE_OFFSET,     tableRow->recordSize);
        eeWriteUint8( address + MAX_NUM_RECORDS_OFFSET, tableRow->maxNumRecords);
        // recordIndex start byte:
        eeWriteUint8(tableRow->recordIndexAddress - 1, 0);
        // recordData start byte:
        eeWriteUint8(tableRow->recordDataAddress - 1, 0);
        address += TABLE_CATALOG_ROW_SIZE;
    }
    // final recordData end byte:
    assert(tableRow != NULL);

    address = tableRow->recordDataAddress + getDataAreaSize(tableRow);
    eeWriteUint8(address, 0);
}


static uint16_t getDataAreaSize(tableDescriptorT* tableRow) {
    return tableRow->recordSize * tableRow->maxNumRecords;
}


// NOTE: no error checking, system must have plenty of RAM
static void allocateRecordDataBuffers(void) {
    tableDescriptorT* tableRow;
    for (uint8_t table=0; table < NumTables; table++) {
        tableRow = &TableCatalog[table];
        tableRow->recordDataBuffer = calloc(tableRow->recordSize, sizeof(uint8_t));
    }
}


// NOTE: no error checking, system must have plenty of RAM
static void allocateRecordInUseBitmap(void) {
    tableDescriptorT* tableRow;
    for (uint8_t table=0; table < NumTables; table++) {
        tableRow = &TableCatalog[table];
        uint8_t bitmapSize = (tableRow->maxNumRecords + 7) / 8;
        tableRow->recordInUseBitmap = calloc(bitmapSize, sizeof(uint8_t));
    }
}


static void generateRecordInUseBitmap(void) {
    tableDescriptorT* tableRow;
    for (uint8_t table = 0; table < NumTables; table++) {
        tableRow = &TableCatalog[table];
        for (uint8_t record = 0; record < tableRow->numRecords; record++) {
            uint8_t offset = getRecordIndex(tableRow, record);
            markRecordInUse(tableRow, offset);
        }
    }
}


static void storeNumTables(const uint8_t numTables) {
    eeWriteUint8(ADDR_NUM_TABLES, numTables);
}


static uint8_t loadNumTables(void) {
    return eeReadUint8(ADDR_NUM_TABLES);
}


static bool isTableSeparatorByte(const eeAddress_t eeAddress) {
    return eeReadUint8(eeAddress) == 0;
}


uint8_t rs_getNumRecords(const uint8_t table) {
    assert(table < NumTables);
    return TableCatalog[table].numRecords;
}


void rs_setRecord(const uint8_t table,
                  const uint8_t record,
                  uint8_t* rawRecord) {
    assertRecordExists(table, record);
    tableDescriptorT* tableRow = &TableCatalog[table];
    eeAddress_t eeAddress = getDataAddress(tableRow, record);
    eeWriteUint8Array(eeAddress, rawRecord, tableRow->recordSize);
}


// Returns the pointer to the module-internal buffer array of this table.
// The pointer remains valid for the program lifetime and must not be freed.
uint8_t* rs_getRecord(const uint8_t table,
                      const uint8_t record) {
    assertRecordExists(table, record);
    tableDescriptorT* tableRow = &TableCatalog[table];
    eeAddress_t eeAddress = getDataAddress(tableRow, record);
    eeReadUint8Array(eeAddress, tableRow->recordDataBuffer, tableRow->recordSize);
    return tableRow->recordDataBuffer;
}


static eeAddress_t getDataAddress(tableDescriptorT* tableRow, const uint8_t record) {
    uint8_t offset = getRecordIndex(tableRow, record);
    return tableRow->recordDataAddress + offset * tableRow->recordSize;
}


bool rs_deleteRecord(const uint8_t table,
                     const uint8_t record) {
    assertRecordExists(table, record);
    tableDescriptorT* tableRow = &TableCatalog[table];
    markRecordFree(tableRow, record);
    shiftRecordIndexDown(tableRow, record);
    tableRow->numRecords--;
    return true;
}


// removes all records and returns the number of deleted records
uint8_t rs_deleteAllRecordsIn(const uint8_t table) {
    assertTableExists(table);
    tableDescriptorT* tableRow = &TableCatalog[table];
    for (uint8_t record = 0; record < tableRow->maxNumRecords; record++) {
        setRecordIndex(tableRow, record, NO_RECORD_INDEX);
        markRecordFree(tableRow, record);
    }
    uint8_t numDeletedRecords = tableRow->numRecords;
    tableRow->numRecords = 0;
    return numDeletedRecords;
}


// returns the recordIndex of the created record
uint8_t rs_appendRecord(const uint8_t table) {
    assertTableExists(table);
    tableDescriptorT* tableRow = &TableCatalog[table];
    return insertRecordAt(tableRow, tableRow->numRecords);
}


// returns the recordIndex of the created record
uint8_t rs_insertRecordAfter(const uint8_t table,
                             const uint8_t record) {
    assertRecordExists(table, record);
    tableDescriptorT* tableRow = &TableCatalog[table];
    return insertRecordAt(tableRow, record + 1);
}


static uint8_t insertRecordAt(tableDescriptorT* tableRow,
                              const uint8_t index) {
    if (tableRow->numRecords >= tableRow->maxNumRecords) {
        return MAX_NUM_RECORDS_REACHED;
    }
    uint8_t freeRecord = getNextFreeRecord(tableRow);
    // numRecords < maxNumRecords, so there MUST be a free record:
    assert(freeRecord != NO_FREE_RECORD_FOUND);
    shiftRecordIndexUp(tableRow, index-1); //NOTE: index=0 is not a problem
    setRecordIndex(tableRow, index, freeRecord);
    markRecordInUse(tableRow, freeRecord);

    tableRow->numRecords++;
    return index;
}


static uint8_t getRecordIndex(tableDescriptorT* tableRow,
                              const uint8_t record) {
    eeAddress_t eeAddress = tableRow->recordIndexAddress + record;
    return eeReadUint8(eeAddress);
}


static void setRecordIndex(tableDescriptorT* tableRow,
                           const uint8_t record,
                           const uint8_t indexValue) {
        eeAddress_t eeAddress = tableRow->recordIndexAddress + record;
        eeWriteUint8(eeAddress, indexValue);
}


// make space in recordIndex for inserting a new record
// NOTE: if record == 255 (-1), nothing happens!
static void shiftRecordIndexUp(tableDescriptorT* tableRow,
                               const uint8_t record) {
    for (uint8_t rec = tableRow->numRecords - 1; rec > record; rec--) {
        eeAddress_t eeAddress = tableRow->recordIndexAddress + rec;
        uint8_t index = eeReadUint8(eeAddress);
        eeWriteUint8(eeAddress+1, index);
    }
}


// close gap in recordIndex for deleting a record
static void shiftRecordIndexDown(tableDescriptorT* tableRow,
                                 const uint8_t record) {
    for (uint8_t rec = record; rec < tableRow->numRecords; rec++) {
        eeAddress_t eeAddress = tableRow->recordIndexAddress + rec;
        uint8_t index = eeReadUint8(eeAddress+1);
        if (rec >= tableRow->maxNumRecords - 1) { // FIXME: not covered by test
            index = NO_RECORD_INDEX;
        }
        eeWriteUint8(eeAddress, index);
    }
}


static uint8_t getNextFreeRecord(tableDescriptorT* tableRow) {
    for (uint8_t rec = 0; rec < tableRow->maxNumRecords; rec++) {
        if (recordIsFree(tableRow, rec)) {
            return rec;
        }
    }
    return NO_FREE_RECORD_FOUND;
}


static void markRecordInUse(tableDescriptorT* tableRow,
                            const uint8_t record) {
    uint8_t byteIndex = getByteIndex(record);
    uint8_t bitMask   = getBitMask(record);
    tableRow->recordInUseBitmap[byteIndex] |= bitMask;
}


static void markRecordFree(tableDescriptorT* tableRow,
                           const uint8_t record) {
    uint8_t byteIndex = getByteIndex(record);
    uint8_t bitMask   = getBitMask(record);
    tableRow->recordInUseBitmap[byteIndex] &= ~bitMask;
}


static bool recordIsFree(tableDescriptorT* tableRow,
                         const uint8_t record) {
    uint8_t byteIndex = getByteIndex(record);
    uint8_t bitMask   = getBitMask(record);
    return (tableRow->recordInUseBitmap[byteIndex] & bitMask) == 0;
}


//TODO: move to generic library
static uint8_t getByteIndex(const uint8_t record) {
    return record >> 3;      // record / 8
}


//TODO: move to generic library
static uint8_t getBitMask(const uint8_t record) {
    return (uint8_t)(1u << (record & 0x07)); // record % 8
}


static void assertTableExists(const uint8_t table) {
    assert(table < NumTables);
}


static void assertRecordExists(const uint8_t table,
                               const uint8_t record) {
    assertTableExists(table);
    assert(record < TableCatalog[table].numRecords);
}

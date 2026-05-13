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
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include "RecordStore.h"
#include "EeHw.h"
#include "BitUtils.h"


// absolute maximum values:
#define MAX_NUM_TABLES 250
#define MAX_NUM_RECORDS 250
#define MAX_RECORD_SIZE 250

// storage addresses and offsets:
#define ADDR_NUM_TABLES 0
#define ADDR_TABLE_CATALOG 1
#define RECORD_INDEX_OFFSET     0
#define RECORD_DATA_OFFSET      (1 * sizeof(eeAddress_t))
#define RECORD_SIZE_OFFSET      (2 * sizeof(eeAddress_t))
#define MAX_NUM_RECORDS_OFFSET  (2 * sizeof(eeAddress_t) + 1)
#define TABLE_CATALOG_ROW_SIZE  (2 * sizeof(eeAddress_t) + 2)

// FIXME: should these be public?
#define NO_RECORD_INDEX 0xFF
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



static uint8_t getRecordIndex(const tableDescriptorT* tableRow,
                              const uint8_t recordId) {
    eeAddress_t eeAddress = tableRow->recordIndexAddress + recordId;
    return eeReadUint8(eeAddress);
}


static void setRecordIndex(const tableDescriptorT* tableRow,
                           const uint8_t recordId,
                           const uint8_t indexValue) {
        eeAddress_t eeAddress = tableRow->recordIndexAddress + recordId;
        eeWriteUint8(eeAddress, indexValue);
}


static bool recordIndexAddress(const tableDescriptorT* tableRow,
                                 const uint8_t recordId) {
    eeAddress_t eeAddress = tableRow->recordIndexAddress + recordId;
    return eeReadUint8(eeAddress) == NO_RECORD_INDEX;
}


static uint8_t getNumRecordsFromIndex(const tableDescriptorT* tableRow) {
    for (uint8_t recordId = 0; recordId < tableRow->maxNumRecords; recordId++) {
        if (recordIndexAddress(tableRow, recordId)) {
            return recordId;
        }
    }
    return tableRow->maxNumRecords;
}


static void markRecordInUse(const tableDescriptorT* tableRow,
                            const uint8_t recordId) {
    uint8_t byteIndex = bu_getByteIndex(recordId);
    uint8_t bitMask   = bu_getSingleBitMask(recordId);
    tableRow->recordInUseBitmap[byteIndex] |= bitMask;
}


static void markRecordFree(const tableDescriptorT* tableRow,
                           const uint8_t recordId) {
    uint8_t byteIndex = bu_getByteIndex(recordId);
    uint8_t bitMask   = bu_getSingleBitMask(recordId);
    tableRow->recordInUseBitmap[byteIndex] &= ~bitMask;
}


static bool recordIsFree(const tableDescriptorT* tableRow,
                         const uint8_t recordId) {
    uint8_t byteIndex = bu_getByteIndex(recordId);
    uint8_t bitMask   = bu_getSingleBitMask(recordId);
    return (tableRow->recordInUseBitmap[byteIndex] & bitMask) == 0;
}


static uint16_t getDataAreaSize(const tableDescriptorT* tableRow) {
    return tableRow->recordSize * tableRow->maxNumRecords;
}


uint8_t rs_getNumRecords(const uint8_t tableId) {
    assert(tableId < NumTables);
    return TableCatalog[tableId].numRecords;
}


static void assertTableExists(const uint8_t tableId) {
    assert(tableId < NumTables);
}


static void assertRecordExists(const uint8_t tableId,
                               const uint8_t recordId) {
    assertTableExists(tableId);
    assert(recordId < TableCatalog[tableId].numRecords);
}


// NOTE: no error checking, system must have plenty of RAM
static void allocateTableCatalog(void) {
    TableCatalog = calloc(NumTables * TABLE_CATALOG_ROW_SIZE, sizeof(tableDescriptorT));
    assert(TableCatalog != NULL);
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


// Loads the tableIdCatalog from EE into RAM
static void loadTableCatalog(void) {
    if (loadNumTables() != NumTables) return;
    eeAddress_t address = ADDR_TABLE_CATALOG;
    tableDescriptorT* tableRow;
    for (uint8_t tableId = 0; tableId < NumTables; tableId++) {
        tableRow = &TableCatalog[tableId];
        tableRow->recordIndexAddress = eeReadUint16(address + RECORD_INDEX_OFFSET);
        tableRow->recordDataAddress  = eeReadUint16(address + RECORD_DATA_OFFSET);
        tableRow->recordSize         = eeReadUint8( address + RECORD_SIZE_OFFSET);
        tableRow->maxNumRecords      = eeReadUint8( address + MAX_NUM_RECORDS_OFFSET);
        tableRow->numRecords         = getNumRecordsFromIndex(tableRow);
        address += TABLE_CATALOG_ROW_SIZE;
    }
}


// NOTE: no error checking, system must have plenty of RAM
static void allocateRecordDataBuffers(void) {
    tableDescriptorT* tableRow;
    for (uint8_t tableId = 0; tableId < NumTables; tableId++) {
        tableRow = &TableCatalog[tableId];
        tableRow->recordDataBuffer = calloc(tableRow->recordSize, sizeof(uint8_t));
        assert(tableRow->recordDataBuffer != NULL);
    }
}


// NOTE: no error checking, system must have plenty of RAM
static void allocateRecordInUseBitmap(void) {
    tableDescriptorT* tableRow;
    for (uint8_t tableId = 0; tableId < NumTables; tableId++) {
        tableRow = &TableCatalog[tableId];
        uint8_t bitmapSize = bu_getNumBytes(tableRow->maxNumRecords);
        tableRow->recordInUseBitmap = calloc(bitmapSize, sizeof(uint8_t));
        assert(tableRow->recordInUseBitmap);
    }
}


static void generateRecordInUseBitmap(void) {
    tableDescriptorT* tableRow;
    for (uint8_t tableId = 0; tableId < NumTables; tableId++) {
        tableRow = &TableCatalog[tableId];
        for (uint8_t recordId = 0; recordId < tableRow->numRecords; recordId++) {
            uint8_t offset = getRecordIndex(tableRow, recordId);
            markRecordInUse(tableRow, offset);
        }
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
    for (uint8_t tableId = 0; tableId < NumTables; tableId++) {
        tableRow = &TableCatalog[tableId];

        // check recordIndex start address:
        if (tableRow->recordIndexAddress != address) return false;
        // check recordIndex start byte:
        if (!isTableSeparatorByte(address-1)) return false;

        address += tableRow->maxNumRecords + 1;

        // check recordData start address:
        if ( tableRow->recordDataAddress != address) return false;
        // check recordData start byte:
        if (!isTableSeparatorByte(address - 1)) return false;

        address += getDataAreaSize(tableRow) + 1;
    }
    // check last recordData closing byte:
    return isTableSeparatorByte(address - 1);
}


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
        return true;
    } else {
        eeClear();
        return false;
    }
}


// only for testing - should never be used in production
void rs_closeRecordStore(void) {
    if (TableCatalog) {
        for (uint8_t tableId = 0; tableId < NumTables; tableId++) {
            if (TableCatalog[tableId].recordDataBuffer) {
                free(TableCatalog[tableId].recordDataBuffer);
            }
            TableCatalog[tableId].recordDataBuffer = NULL;

            if (TableCatalog[tableId].recordInUseBitmap) {
                free(TableCatalog[tableId].recordInUseBitmap);
            }
            TableCatalog[tableId].recordInUseBitmap = NULL;
        }
        free(TableCatalog);
    }
    TableCatalog = NULL;
    NumTables = 0;
}


void rs_deleteTableCatalog(void) {
    storeNumTables(0);
}


static void createTable(const uint8_t tableId,
                        const uint8_t maxNumRecords,
                        const uint8_t recordSize) {
    assertTableExists(tableId);
    eeAddress_t recordIndexAddress;
    tableDescriptorT* tableRow = &TableCatalog[tableId];
    if (tableId > 0) {
        tableDescriptorT* previousRow = &TableCatalog[tableId-1];
        recordIndexAddress = previousRow->recordDataAddress
                             + getDataAreaSize(previousRow) + 1;
    } else {
        recordIndexAddress = NumTables * TABLE_CATALOG_ROW_SIZE + 2;
    }
    eeAddress_t recordDataAddress = recordIndexAddress + maxNumRecords + 1;
    eeAddress_t endByteAddress = recordDataAddress
                                 + (uint16_t)(recordSize * maxNumRecords);
    assertEeAddressExists(endByteAddress); // NOTE: use disableAssert() for testing end byte algorthm
    tableRow->recordIndexAddress = recordIndexAddress;
    tableRow->recordDataAddress = recordDataAddress;
    tableRow->recordSize = recordSize;
    tableRow->maxNumRecords = maxNumRecords;
    tableRow->numRecords = 0;
}


uint8_t rs_createTable(const uint8_t maxNumRecords,
                       const uint8_t recordSize) {
    assert(maxNumRecords > 0 && maxNumRecords <= MAX_NUM_RECORDS);
    assert(recordSize > 0 && recordSize <= MAX_RECORD_SIZE);

    for (uint8_t tableId = 0; tableId < NumTables; tableId++) {
        if (TableCatalog[tableId].maxNumRecords == 0) {
            createTable(tableId, maxNumRecords, recordSize);
            return tableId;
        }
    }
    assert(0 && "too many tableIds");
}


static void storeTableCatalog(void) {
    storeNumTables(NumTables);
    eeAddress_t address = ADDR_TABLE_CATALOG;
    tableDescriptorT* tableRow = NULL;
    for (uint8_t tableId = 0; tableId < NumTables; tableId++) {
        tableRow = &TableCatalog[tableId];
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


void rs_commitTables(void) {
    storeTableCatalog();
    allocateRecordDataBuffers();
    allocateRecordInUseBitmap();
}


static eeAddress_t getDataAddress(tableDescriptorT* tableRow,
                                  const uint8_t recordId) {
    uint8_t offset = getRecordIndex(tableRow, recordId);
    return tableRow->recordDataAddress + offset * tableRow->recordSize;
}


void rs_setRawRecord(const uint8_t tableId,
                     const uint8_t recordId,
                     uint8_t* rawRecord) {
    assertRecordExists(tableId, recordId);
    tableDescriptorT* tableRow = &TableCatalog[tableId];
    eeAddress_t eeAddress = getDataAddress(tableRow, recordId);
    eeWriteUint8Array(eeAddress, rawRecord, tableRow->recordSize);
}


const uint8_t* rs_getRawRecord(const uint8_t tableId,
                               const uint8_t recordId) {
    assertRecordExists(tableId, recordId);
    tableDescriptorT* tableRow = &TableCatalog[tableId];
    eeAddress_t eeAddress = getDataAddress(tableRow, recordId);
    eeReadUint8Array(eeAddress, tableRow->recordDataBuffer, tableRow->recordSize);
    return tableRow->recordDataBuffer;
}


// close gap in recordIndex for deleting a record
static void shiftRecordIndexDown(tableDescriptorT* tableRow,
                                 const uint8_t recordId) {
    for (uint8_t rec = recordId; rec < tableRow->numRecords; rec++) {
        eeAddress_t eeAddress = tableRow->recordIndexAddress + rec;
        uint8_t index = eeReadUint8(eeAddress + 1);
        if (rec >= tableRow->maxNumRecords - 1) { // FIXME: not covered by test
            index = NO_RECORD_INDEX;
        }
        eeWriteUint8(eeAddress, index);
    }
}


void rs_deleteRecord(const uint8_t tableId,
                     const uint8_t recordId) {
    assertRecordExists(tableId, recordId);
    tableDescriptorT* tableRow = &TableCatalog[tableId];
    markRecordFree(tableRow, recordId);
    shiftRecordIndexDown(tableRow, recordId);
    tableRow->numRecords--;
}


uint8_t rs_deleteAllRecords(const uint8_t tableId) {
    assertTableExists(tableId);
    tableDescriptorT* tableRow = &TableCatalog[tableId];
    for (uint8_t recordId = 0; recordId < tableRow->maxNumRecords; recordId++) {
        setRecordIndex(tableRow, recordId, NO_RECORD_INDEX);
        markRecordFree(tableRow, recordId);
    }
    uint8_t numDeletedRecords = tableRow->numRecords;
    tableRow->numRecords = 0;
    return numDeletedRecords;
}


// make space in recordIndex for inserting a new record
// NOTE: if record == 255 (-1), nothing happens!
static void shiftRecordIndexUp(tableDescriptorT* tableRow,
                               const uint8_t record) {
    for (uint8_t rec = tableRow->numRecords - 1; rec > record; rec--) {
        eeAddress_t eeAddress = tableRow->recordIndexAddress + rec;
        uint8_t index = eeReadUint8(eeAddress);
        eeWriteUint8(eeAddress + 1, index);
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


static uint8_t insertRecordAt(tableDescriptorT* tableRow,
                              const uint8_t index) {
    if (tableRow->numRecords >= tableRow->maxNumRecords) {
        return MAX_NUM_RECORDS_REACHED;
    }
    uint8_t freeRecord = getNextFreeRecord(tableRow);
    // numRecords < maxNumRecords, so there MUST be a free record:
    assert(freeRecord != NO_FREE_RECORD_FOUND);
    shiftRecordIndexUp(tableRow, index - 1); //NOTE: index=0 is not a problem
    setRecordIndex(tableRow, index, freeRecord);
    markRecordInUse(tableRow, freeRecord);

    tableRow->numRecords++;
    return index;
}


uint8_t rs_appendRecord(const uint8_t tableId) {
    assertTableExists(tableId);
    tableDescriptorT* tableRow = &TableCatalog[tableId];
    return insertRecordAt(tableRow, tableRow->numRecords);
}


uint8_t rs_insertRecordAfter(const uint8_t tableId,
                             const uint8_t record) {
    assertRecordExists(tableId, record);
    tableDescriptorT* tableRow = &TableCatalog[tableId];
    return insertRecordAt(tableRow, record + 1);
}

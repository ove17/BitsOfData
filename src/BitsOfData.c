/*
 * BitsOfData.c
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "BitsOfData.h"
#include "RecordStore.h"
#include "RecordCodec.h"

#include <stdio.h>

#define NO_RECORD_ID 0xFF

typedef struct {
    uint8_t recordId;
    bool isModified;
    uint16_t* columns;
} recordBufferT;

static recordBufferT* RecordBuffers = NULL;
static uint8_t* RawRecordBuffer = NULL;
static const BDB_dbaseDefT* DbaseDef = NULL;

static void checkDbaseDef();
static void allocateRecordBuffers();
static void createTable(const uint8_t tableId);
static uint8_t getMaxRecordSize(const uint8_t tableId);
static void createFirstRecordInEveryTable();

static void setRecordToDefaultValues(const uint8_t table,
                                     const uint8_t record);
static void setVariableRecordToDefaultValues(const uint8_t tableId,
                                             const uint8_t recordId,
                                             const BDB_recordT* recordDef);
static void setColumnsToDefaultValue(const uint8_t tableId,
                                     const uint8_t recordId,
                                     const uint8_t startColumnId,
                                     const BDB_recordT* recordDef);
static void setColumnToDefaultValue(const uint8_t tableId,
                                    const uint8_t columnId,
                                    const BDB_recordT* recordDef);

static void storeRecordBuffer(const uint8_t tableId,
                              const uint8_t recordId);
static void fillRecordBuffer(const uint8_t tableId,
                             const uint8_t recordId);

static BDB_recordT getRecordDef(const uint8_t tableId,
                                const BDB_tableT* tableDef);

static int16_t limitValue(const int16_t minValue,
                           const int16_t value,
                           const int16_t maxValue);


// returns true if a database was found and opened, false if one was created
bool BDB_openDataBase(const BDB_dbaseDefT* dbaseDef) {
    DbaseDef = dbaseDef;
    checkDbaseDef();
    allocateRecordBuffers();
    uint8_t numTables = DbaseDef->numTables;
    if (!rs_tryToOpenRecordStore(numTables)) {
        for (uint8_t tableId = 0; tableId < numTables; tableId++) {
            createTable(tableId);
        }
        rs_commitTables();
        createFirstRecordInEveryTable();
        return false;
    }
    return true;
}


static void checkDbaseDef() {
    uint8_t numTables = DbaseDef->numTables;
    for (uint8_t tableId = 0; tableId < numTables; tableId++) {
        BDB_tableT table = DbaseDef->tables[tableId];
        uint8_t numRecDefs = table.numRecordDefs;
        if (numRecDefs > 1) {
            for (uint8_t recDef = 0; recDef < numRecDefs; recDef++) {
                BDB_columnT recordDef = table.recordDefs[recDef].columns[0];
                assert(recordDef.colType == BDB_COLUMN_RECORD_TYPE);
                assert(recordDef.maxValue == numRecDefs - 1);
            }
        }
    }
}


static uint8_t getMaxNumColumns(const uint8_t tableId) {
    uint8_t numTables = DbaseDef->numTables;
    uint8_t maxNumColumns = 0;
    for (uint8_t tableId = 0; tableId < numTables; tableId++) {
        BDB_tableT table = DbaseDef->tables[tableId];
        uint8_t numRecDefs = table.numRecordDefs;
        for (uint8_t recDef = 0; recDef < numRecDefs; recDef++) {
            BDB_recordT recordDef = table.recordDefs[recDef];
            uint8_t numColumns = recordDef.numColumns;
            if (numColumns > maxNumColumns) {
                maxNumColumns = numColumns;
            }
        }
    }
//FIXME testing recordCodec requires MAX_NUM_COLUMNS == 6 :
//    assert(maxNumColumns == MAX_NUM_COLUMNS);
    return maxNumColumns;
}


static uint8_t getMaxRawRecordSize() {
    uint8_t numTables = DbaseDef->numTables;
    uint8_t maxRawRecordSize = 0;
    for (uint8_t tableId = 0; tableId < numTables; tableId++) {
        uint8_t recordSize = getMaxRecordSize(tableId);
        if (recordSize > maxRawRecordSize) {
            maxRawRecordSize = recordSize;
        }
    }
    return maxRawRecordSize;
}


static void allocateRecordBuffers() {
    uint8_t numTables = DbaseDef->numTables;
    RecordBuffers = calloc(numTables, sizeof(recordBufferT));
    for (uint8_t table = 0; table < numTables; table++) {
        uint8_t numColumns = getMaxNumColumns(table);
        recordBufferT* recordBuffer = &RecordBuffers[table];
        recordBuffer->recordId = NO_RECORD_ID;
        recordBuffer->isModified = false;
        recordBuffer->columns = calloc(numColumns, sizeof(uint16_t));
    }
    uint8_t maxRawRecordSize = getMaxRawRecordSize();
    RawRecordBuffer = calloc(maxRawRecordSize, sizeof(uint8_t));
}


static void freeRecordBuffers() {
    if (RecordBuffers != NULL) {
        uint8_t numTables = DbaseDef->numTables;
        for (uint8_t table = 0; table < numTables; table++) {
            free(RecordBuffers[table].columns);
        }
        free(RecordBuffers);
        RecordBuffers = NULL;
    }
    if (RawRecordBuffer != NULL) {
        free(RawRecordBuffer);
        RawRecordBuffer = NULL;
    }
}


static void createTable(const uint8_t tableId) {
    BDB_tableT tableDef = DbaseDef->tables[tableId];
    uint8_t numRecords = tableDef.maxNumRecords;
    uint8_t maxRecordSize = getMaxRecordSize(tableId);
    rs_createTable(numRecords, maxRecordSize);
}


// returns the packed size of a record in bytes
// if it has a variable record definition: the biggest is returned
static uint8_t getMaxRecordSize(const uint8_t tableId) {
    BDB_tableT tableDef = DbaseDef->tables[tableId];
    uint8_t numRecordDefs = tableDef.numRecordDefs;
    uint8_t maxRecordSize = 0;
    for (uint8_t recordDefId = 0; recordDefId < numRecordDefs; recordDefId++) {
        BDB_recordT recordDef = tableDef.recordDefs[recordDefId];
        uint8_t recordSize = rc_getRecordSize(&recordDef);
        if (recordSize > maxRecordSize) {
            maxRecordSize = recordSize;
        }
    }
    return maxRecordSize;
}


static void createFirstRecordInEveryTable() {
    uint8_t numTables = DbaseDef->numTables;
    for (uint8_t tableId = 0; tableId < numTables; tableId++) {
        uint8_t recordId = rs_appendRecord(tableId); // mandatory 1st record
        assert(recordId == 0);
        setRecordToDefaultValues(tableId, recordId);
    }
}


static void setRecordToDefaultValues(const uint8_t tableId,
                                     const uint8_t recordId) {

    BDB_tableT tableDef = DbaseDef->tables[tableId];
    BDB_recordT recordDef = tableDef.recordDefs[0]; // default is always 1st def
    setColumnsToDefaultValue(tableId, recordId, 0, &recordDef);
}


static void setVariableRecordToDefaultValues(const uint8_t tableId,
                                             const uint8_t recordId,
                                             const BDB_recordT* recordDef) {
    setColumnsToDefaultValue(tableId, recordId, 1, recordDef);
}


static void setColumnsToDefaultValue(const uint8_t tableId,
                                     const uint8_t recordId,
                                     const uint8_t startColumnId,
                                     const BDB_recordT* recordDef) {
    RecordBuffers[tableId].recordId = recordId;
    uint8_t numColumns = recordDef->numColumns;
    for (uint8_t col = startColumnId; col < numColumns; col++) {
        setColumnToDefaultValue(tableId, col, recordDef);
    }
    storeRecordBuffer(tableId, recordId);
}


static void setColumnToDefaultValue(const uint8_t tableId,
                                    const uint8_t columnId,
                                    const BDB_recordT* recordDef) {
    BDB_columnT columnDef = recordDef->columns[columnId];
    assert(columnDef.defaultVal >= columnDef.minValue);
    assert(columnDef.defaultVal <= columnDef.maxValue);
    RecordBuffers[tableId].columns[columnId] = columnDef.defaultVal - columnDef.minValue;
}


void BDB_closeDataBase() {
    rs_closeRecordStore();
    freeRecordBuffers();
    DbaseDef = NULL;
}


uint8_t BDB_getNumRecords(const uint8_t tableId) {
    return rs_getNumRecords(tableId);
}


uint16_t BDB_getValue(const uint8_t tableId,
                      const uint8_t recordId,
                      const uint8_t columnId) {
    fillRecordBuffer(tableId, recordId);
    BDB_recordT recordDef = getRecordDef(tableId, &(DbaseDef->tables[tableId]));
    BDB_columnT columnDef = recordDef.columns[columnId];
    return RecordBuffers[tableId].columns[columnId] + columnDef.minValue;
}


// returns false if the value is out of range
bool BDB_setValue (const uint8_t tableId,
                   const uint8_t recordId,
                   const uint8_t columnId,
                   const uint16_t value) {
    fillRecordBuffer(tableId, recordId);
    BDB_tableT tableDef = DbaseDef->tables[tableId];
    BDB_recordT recordDef = getRecordDef(tableId, &tableDef);
    BDB_columnT columnDef = recordDef.columns[columnId];
    if (value < columnDef.minValue || value > columnDef.maxValue ) {
        return false;
    }

    if (columnId == 0 && tableDef.numRecordDefs > 1) {
        // change record type:
        RecordBuffers[tableId].columns[0] = value;
        recordDef = tableDef.recordDefs[value];
        setVariableRecordToDefaultValues(tableId, recordId, &recordDef);
    } else {
        RecordBuffers[tableId].columns[columnId] = value - columnDef.minValue;
    }
    RecordBuffers[tableId].isModified = true;
    return true;
}


bool BDB_changeValue(const uint8_t tableId,
                     const uint8_t recordId,
                     const uint8_t columnId,
                     const int16_t delta) {
    int16_t value = BDB_getValue(tableId, recordId, columnId); // also fills RecordBuffer
    BDB_tableT tableDef = DbaseDef->tables[tableId];
    BDB_recordT recordDef = getRecordDef(tableId, &tableDef);
    BDB_columnT columnDef = recordDef.columns[columnId];

    int16_t newValue = limitValue(columnDef.minValue, value + delta, columnDef.maxValue);
    if (newValue == value) {
        return false;
    }
    RecordBuffers[tableId].columns[columnId] = (uint16_t)(newValue - columnDef.minValue);
    if (columnId == 0 && tableDef.numRecordDefs > 1) {
        // change record type:
        recordDef = tableDef.recordDefs[newValue];
        setVariableRecordToDefaultValues(tableId, recordId, &recordDef);
    }
    RecordBuffers[tableId].isModified = true;
    return true;
}


void BDB_storeRecord(const uint8_t tableId,
                     const uint8_t recordId) {
    storeRecordBuffer(tableId, recordId);
}


static void storeRecordBuffer(const uint8_t tableId,
                              const uint8_t recordId) {
    recordBufferT* recordBuffer = &RecordBuffers[tableId];
    BDB_recordT recordDef = getRecordDef(tableId, &(DbaseDef->tables[tableId]));
    rc_encodeRecord(recordBuffer->columns, RawRecordBuffer, &recordDef);
    rs_setRawRecord(tableId, recordId, RawRecordBuffer);
}


static void fillRecordBuffer(const uint8_t tableId,
                             const uint8_t recordId) {
    recordBufferT* recordBuffer = &RecordBuffers[tableId];
    uint8_t* bufferedRecord = &recordBuffer->recordId;
    if (*bufferedRecord != recordId) {
        if (recordBuffer->isModified && *bufferedRecord != NO_RECORD_ID) {
            storeRecordBuffer(tableId, *bufferedRecord);
        }
        *bufferedRecord = recordId;
// get new record:
        BDB_recordT recordDef = getRecordDef(tableId, &(DbaseDef->tables[tableId]));
        uint8_t* rawRecord = rs_getRawRecord(tableId, recordId);
        rc_decodeRecord(rawRecord, recordBuffer->columns, &recordDef);
        recordBuffer->isModified = false;
    }
}


// NOTE: caller must ensure RecordBuffer holds correct data
static BDB_recordT getRecordDef(const uint8_t tableId,
                                const BDB_tableT* tableDef) {
    uint8_t numRecordDefs = tableDef->numRecordDefs;
    uint8_t recordType = 0;
    if (numRecordDefs > 1) {
        recordType = (uint8_t)RecordBuffers[tableId].columns[0];
    }
    return tableDef->recordDefs[recordType];
}


// TODO put in generic library
static int16_t limitValue(const int16_t minValue,
                           const int16_t value,
                           const int16_t maxValue) {
    if (value >= maxValue) {
        return maxValue;
    }
    if (value <= minValue) {
        return minValue;
    }
    return value;
}


// Returns false if max. no of records was reached
bool BDB_insertRecordAfter(const int8_t tableId,
                           const uint8_t recordId) {
    uint8_t newRecordId = rs_insertRecordAfter(tableId, recordId);
    if (newRecordId == MAX_NUM_RECORDS_REACHED) {
        return false;
    }
    setRecordToDefaultValues(tableId, recordId + 1);
    return true;
}

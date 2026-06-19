/*
 * BitsOfData.c
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include "BitsOfData.h"
#include "BitsOfDataTypes.h"
#include "RecordStore.h"
#include "RecordCodec.h"
#include "WriteColumns.h"
#include "MathUtils.h"
#include "TxtUtils.h"

#define NO_RECORD_ID 0xFF
#define NOT_SET 0xFF
#define INVALID_VALUE 0xFFFF

typedef struct {
    uint8_t tableId;
    uint8_t recordId;
    uint8_t columnId;
} TrcT;


typedef struct {
    bool leading0;
    bool offset1;
} PropT;


typedef struct {
    uint8_t recordId;
    bool isModified;
    uint16_t* columns;
} recordBufferT;

#ifndef NDEBUG
#include <stdio.h>
extern void assertDbaseDefIsValid(const BDB_dbaseDefT* dbaseDef);
#endif

static const BDB_dbaseDefT* DbaseDef = NULL;
static recordBufferT* RecordBuffers = NULL;
static uint8_t* RawRecordBuffer = NULL;
static uint8_t* ParentRecordList = NULL;
static uint8_t NumChildColumns = 0;
static uint8_t ParentTableId = NOT_SET;
static uint8_t ParentChildColumnId = NOT_SET;
static BDB_txtHandlerFunction GetTxtPtr = NULL;


static void storeRecordBuffer(const uint8_t tableId) {
    recordBufferT* recordBuffer = &RecordBuffers[tableId];
    const uint8_t recordId = recordBuffer->recordId;
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    rc_encodeRecord(recordBuffer->columns, RawRecordBuffer, tableDef);
    rs_setRawRecord(tableId, recordId, RawRecordBuffer);
}


void BDB_syncTable(const uint8_t tableId) {
    storeRecordBuffer(tableId);
}


void BDB_syncDbase(void) {
    uint8_t numTables = DbaseDef->numTables;
    for (uint8_t tableId = 0; tableId < numTables; tableId++) {
        storeRecordBuffer(tableId);
    }
}


static void writeRecordBufferIfDirty(const uint8_t tableId) {
    recordBufferT* recordBuffer = &RecordBuffers[tableId];
    if (recordBuffer->isModified) {
        storeRecordBuffer(tableId);
    }
}


static void fillRecordBuffer(const uint8_t tableId,
                             const uint8_t recordId) {
    recordBufferT* recordBuffer = &RecordBuffers[tableId];
    if (recordBuffer->recordId != recordId) {
        writeRecordBufferIfDirty(tableId);
        recordBuffer->recordId = recordId;
        const uint8_t* rawRecord = rs_getRawRecord(tableId, recordId);
        const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
        rc_decodeRecord(rawRecord, recordBuffer->columns, tableDef);
        recordBuffer->isModified = false;
    }
}


static const BDB_recordT* getRecordDefFromBuffer(const uint8_t tableId,
                                                 const uint8_t recordId) {
    fillRecordBuffer(tableId, recordId);
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    const uint8_t numRecordDefs = tableDef->numRecordDefs;
    uint8_t recordType = 0;
    if (numRecordDefs > 1) {
        recordType = (uint8_t)RecordBuffers[tableId].columns[0];
    }
    return &tableDef->recordDefs[recordType];
}


static const BDB_recordT* getRecordDefFromData(const uint8_t tableId,
                                               const uint16_t* data) {
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    const uint8_t numRecordDefs = tableDef->numRecordDefs;
    uint8_t recordType = (uint8_t)((numRecordDefs > 1) ? (uint8_t)data[0] : 0);
    return &tableDef->recordDefs[recordType];
}


static const BDB_columnT* getColumnDef(const BDB_recordT* recordDef,
                                       const uint8_t columnId) {
    return &recordDef->columns[columnId];
}


static uint8_t getMaxNumColumns(const uint8_t tableId) {
    uint8_t maxNumColumns = 0;
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    const uint8_t numRecDefs = tableDef->numRecordDefs;
    for (uint8_t recDefId = 0; recDefId < numRecDefs; recDefId++) {
        const BDB_recordT* recordDef = &tableDef->recordDefs[recDefId];
        const uint8_t numColumns = recordDef->numColumns;
        if (numColumns > maxNumColumns) {
            maxNumColumns = numColumns;
        }
    }
    return maxNumColumns;
}


// returns the (maximum) record (storage) size in bytes
static uint8_t getMaxRawRecordSize(void) {
    uint8_t maxRawRecordSize = 0;
    const uint8_t numTables = DbaseDef->numTables;
    for (uint8_t tableId = 0; tableId < numTables; tableId++) {
        const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
        const uint8_t recordSize = rc_getMaxRecordSize(tableDef);
        if (recordSize > maxRawRecordSize) {
            maxRawRecordSize = recordSize;
        }
    }
    return maxRawRecordSize;
}


static void allocateRecordBuffers(void) {
    uint8_t numTables = DbaseDef->numTables;
    RecordBuffers = calloc(numTables, sizeof(recordBufferT));
    assert(RecordBuffers != NULL);
    for (uint8_t table = 0; table < numTables; table++) {
        uint8_t numColumns = getMaxNumColumns(table);
        recordBufferT* recordBuffer = &RecordBuffers[table];
        recordBuffer->recordId = NO_RECORD_ID;
        recordBuffer->isModified = false;
        recordBuffer->columns = calloc(numColumns, sizeof(uint16_t));
        assert(recordBuffer->columns != NULL);
    }
    uint8_t maxRawRecordSize = getMaxRawRecordSize();
    RawRecordBuffer = calloc(maxRawRecordSize, sizeof(uint8_t));
    assert(RawRecordBuffer != NULL);
}


static void clearParentRecordList(const uint8_t listSize) {
    for (uint8_t i = 0; i < listSize; i++) {
        ParentRecordList[i] = NO_RECORD_ID;
    }
}


static void allocateParentRecordList(void) {
    const uint8_t numTables = DbaseDef->numTables;
    for (uint8_t tableId = 0; tableId < numTables; tableId++) {
        const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
        const uint8_t numRecordDefs = tableDef->numRecordDefs;
        if (numRecordDefs > 1) {
            continue; // record with variable type cannot have children
        }
        const uint8_t numColumns = getMaxNumColumns(tableId);
        for (uint8_t columnId = 0; columnId < numColumns; columnId++) {
            const BDB_columnT* columnDef = &tableDef->recordDefs[0].columns[columnId];
            if (columnDef->colType == BDB_COLUMN_CHILD_TABLE) {
                assert (NumChildColumns < BDB_MAX_NUM_CHILD_COLUMNS);
                ParentTableId = tableId;
                ParentChildColumnId = columnId;
                NumChildColumns++;
            }
        }
    }
    if (NumChildColumns == 1) {
        const BDB_recordT* recordDef = &DbaseDef->tables[ParentTableId].recordDefs[0];
        const BDB_columnT* columnDef = &recordDef->columns[ParentChildColumnId];
        const uint8_t listSize = (uint8_t)(columnDef->maxValue - columnDef->minValue + 1);
        ParentRecordList = calloc(listSize, sizeof(uint8_t));
        clearParentRecordList(listSize);
    }
}


static void createTable(const uint8_t tableId) {
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    const uint8_t maxNumRecords = tableDef->maxNumRecords;
    const uint8_t maxRecordSize = rc_getMaxRecordSize(tableDef);
    rs_createTable(maxNumRecords, maxRecordSize);
}


const BDB_columnT* getColumnDefProps(const uint8_t tableId,
                                     const uint8_t recordId,
                                     const BDB_columnT* columnDef) {
    const uint8_t refCol = columnDef->copy.refCol;
    const uint8_t columnId = columnDef->copy.columnId;
    const BDB_recordT* recordDef = getRecordDefFromBuffer(tableId, recordId);
    const uint8_t propsTableId = recordDef->columns[refCol].ref.tableId;
    const uint8_t propsRecordId = (uint8_t)BDB_getValue(tableId, recordId, refCol);
    const BDB_recordT* propsRecordDef = getRecordDefFromBuffer(propsTableId, propsRecordId);
    return getColumnDef(propsRecordDef, columnId);
}


static uint16_t getMaxValue(const uint8_t tableId,
                            const uint8_t recordId,
                            const BDB_columnT* columnDef) {
    if (columnDef->colType == BDB_COLUMN_REFERENCE) {
        if (columnDef->ref.tableId == BDB_SELF_REFERENCE) {
            return rs_getNumRecords(tableId) - 1;
        } else {
            return rs_getNumRecords(columnDef->ref.tableId) - 1;
        }
    } else
    if (columnDef->colType == BDB_COLUMN_TXT_LIST_COPY) {
        if (recordId == 0xFF) return 4; // FIXME ugly hack:
                            // uses max value for import of AMDB - MUST be
                            // read from referenced table
        const BDB_columnT* propsColumnDef = getColumnDefProps(tableId,
                                                              recordId,
                                                              columnDef);
        return propsColumnDef->maxValue;
    } else {
        return columnDef->maxValue;
    }
}


static uint16_t getDefaultValue(const BDB_columnT* columnDef) {
    if (rc_isVirtualColumn(columnDef)) {
        return INVALID_VALUE;
    }
    return columnDef->defaultVal;
}


void buildParentRecordList(void) {
    const BDB_recordT* recordDef = getRecordDefFromBuffer(ParentTableId, 0);
    const BDB_columnT* columnDef = getColumnDef(recordDef, ParentChildColumnId);
    const uint8_t minValue = (uint8_t)columnDef->minValue;
    const uint8_t listSize = (uint8_t)(columnDef->maxValue - minValue + 1);
    clearParentRecordList(listSize);
    const uint8_t numRecords = rs_getNumRecords(ParentTableId);
    for (uint8_t recId = 0; recId < numRecords; recId++) {
        const uint8_t childTableId = (uint8_t)BDB_getValue(ParentTableId,
                                                           recId,
                                                           ParentChildColumnId);
        const uint8_t i = childTableId - minValue;
        ParentRecordList[i] = recId;
    }
}


static void setColumnsToDefaultValue(const uint8_t tableId,
                                     const uint8_t recordId,
                                     const uint8_t startColumnId,
                                     const BDB_recordT* recordDef) {
    writeRecordBufferIfDirty(tableId);
    RecordBuffers[tableId].recordId = recordId;
    uint8_t numColumns = recordDef->numColumns;
    for (uint8_t col = startColumnId; col < numColumns; col++) {
        uint16_t value;
        if (tableId == ParentTableId && col == ParentChildColumnId) {
            const BDB_columnT* columnDef = &recordDef->columns[col];
            const uint8_t minValue = (uint8_t)columnDef->minValue;
            value = minValue + recordId;
        } else {
            const BDB_columnT* columnDef = &recordDef->columns[col];
            value = getDefaultValue(columnDef);
        }
        RecordBuffers[tableId].columns[col] = value;
    }
    storeRecordBuffer(tableId);
    if (tableId == ParentTableId) {
        buildParentRecordList();
    }
}


static void setRecordToDefaultValues(const uint8_t tableId,
                                     const uint8_t recordId) {
    BDB_tableT tableDef = DbaseDef->tables[tableId];
    BDB_recordT recordDef = tableDef.recordDefs[0]; // default is always 1st def
    setColumnsToDefaultValue(tableId, recordId, 0, &recordDef);
}


static void createFirstRecordInEveryTable(void) {
    uint8_t numTables = DbaseDef->numTables;
    for (uint8_t tableId = 0; tableId < numTables; tableId++) {
        uint8_t recordId = rs_appendRecord(tableId); // mandatory 1st record
        assert(recordId == 0);
        setRecordToDefaultValues(tableId, recordId);
    }
}


bool BDB_openDataBase(const BDB_dbaseDefT* dbaseDef,
                      BDB_txtHandlerFunction txtHandler) {
    DbaseDef = dbaseDef;
    GetTxtPtr = txtHandler;
#ifndef NDEBUG
    assertDbaseDefIsValid(DbaseDef);
#endif
    wc_initBuffer(DbaseDef->maxStringBufferSize);
    allocateRecordBuffers();
    allocateParentRecordList();
    const uint8_t numTables = DbaseDef->numTables;
    bool dbExists = true;
    if (!rs_tryToOpenRecordStore(numTables)) {
        for (uint8_t tableId = 0; tableId < numTables; tableId++) {
            createTable(tableId);
        }
        rs_commitTables();
        createFirstRecordInEveryTable();
        dbExists = false;
    } else {
        buildParentRecordList();
    }
    return dbExists;
}


static void freeRecordBuffers(void) {
    if (RecordBuffers != NULL) {
        const uint8_t numTables = DbaseDef->numTables;
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
    wc_freeBuffer();
}


static void freeParentRecordLists(void) {
    if (ParentRecordList != NULL) {
        free(ParentRecordList);
        ParentRecordList = NULL;
    }
    NumChildColumns = 0;
    ParentTableId = 0xFF;
    ParentChildColumnId = 0xFF;
}


void BDB_closeDataBase(void) {
    rs_closeRecordStore();
    freeRecordBuffers();
    freeParentRecordLists();
    DbaseDef = NULL;
}


// managing records:


uint8_t BDB_getNumRecords(const uint8_t tableId) {
    return rs_getNumRecords(tableId);
}


typedef bool (*operatorFunction)(const uint8_t tableId,
                                 const uint8_t recordId,
                                 const uint8_t columnId,
                                 const uint8_t refRecordId);


static bool forEachReference(const uint8_t refTableId,
                             const uint8_t refRecordId,
                             operatorFunction operation) {
    const uint8_t numTables = DbaseDef->numTables;

    for (uint8_t tableId = 0; tableId < numTables; tableId++) {
        const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
        const uint8_t numRecordDefs = tableDef->numRecordDefs;
        const uint8_t numRecords = rs_getNumRecords(tableId);

        const bool hasVariableRecordDef = (numRecordDefs > 1);

        for (uint8_t recDef = 0; recDef < numRecordDefs; recDef++) {
            const BDB_recordT* recordDef = &tableDef->recordDefs[recDef];
            const uint8_t numColumns = recordDef->numColumns;
            const uint8_t startCol = hasVariableRecordDef ? 1 : 0;
            for (uint8_t col = startCol; col < numColumns; col++) {
                const BDB_columnT* columnDef = &recordDef->columns[col];
                const BDB_colTypeT colType = columnDef->colType;
                if (colType != BDB_COLUMN_REFERENCE) continue;

                if ( !( (columnDef->ref.tableId == BDB_SELF_REFERENCE
                            && tableId == refTableId)
                        || columnDef->ref.tableId == refTableId) ) continue;

                // a column was found that references the right table (refTableId)
                for (uint8_t rec = 0; rec < numRecords; rec++) {
                    if (hasVariableRecordDef && BDB_getValue(tableId, rec, 0) != recDef) {
                        continue;
                    }
                    // a record was found that has the right recordType

                    // exclude self referencing column that points to its own record:
                    if (columnDef->ref.tableId == BDB_SELF_REFERENCE
                        && tableId == refTableId
                        && BDB_getValue(tableId, rec, col) == refRecordId) continue;

                    if (operation(tableId, rec, col, refRecordId)) {
//printf("\t%i\t%i\t%i\t%i\n", tableId, rec, col, refRecordId);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}


static bool matchFirst(const uint8_t tableId,
                       const uint8_t recordId,
                       const uint8_t columnId,
                       const uint8_t refRecordId) {
    return BDB_getValue(tableId, recordId, columnId) == refRecordId; // match stops traversal
}


static bool isRecordReferenced(const uint8_t refTableId,
                               const uint8_t refRecordId) {
    return forEachReference(refTableId, refRecordId, matchFirst);
}


static bool shiftReferenceDown(const uint8_t tableId,
                               const uint8_t recordId,
                               const uint8_t columnId,
                               const uint8_t RecordToShiftId) {
    const uint8_t refRecordId = (uint8_t)BDB_getValue(tableId, recordId, columnId);
    if (refRecordId > RecordToShiftId) {
        BDB_changeValue(tableId, recordId, columnId, -1);
    }
    return false; // continue traversal
}


static bool shiftRecordReferencesDown(const uint8_t refTableId,
                                      const uint8_t refRecordId) {
    return forEachReference(refTableId, refRecordId, shiftReferenceDown);
}


static bool shiftReferenceUp(const uint8_t tableId,
                             const uint8_t recordId,
                             const uint8_t columnId,
                             const uint8_t RecordToShiftId) {
    const uint8_t refRecordId = (uint8_t)BDB_getValue(tableId, recordId, columnId);
    if (refRecordId > RecordToShiftId) {
        BDB_changeValue(tableId, recordId, columnId, 1);
    }
    return false; // continue traversal
}


static bool shiftRecordReferencesUp(const uint8_t refTableId,
                                    const uint8_t refRecordId) {
    return forEachReference(refTableId, refRecordId, shiftReferenceUp);
}


bool BDB_canRecordBeDeleted(const uint8_t tableId,
                            const uint8_t recordId) {
    return (rs_getNumRecords(tableId) > 1 && !isRecordReferenced(tableId, recordId));
}


bool BDB_deleteRecord(const uint8_t tableId,
                      const uint8_t recordId) {
    if (BDB_canRecordBeDeleted(tableId, recordId)) {
        rs_deleteRecord(tableId, recordId);
        shiftRecordReferencesDown(tableId, recordId);
        if (tableId == ParentTableId) {
            buildParentRecordList();
        }
        return true;
    }
    return false;
}


bool BDB_canRecordBeAdded(const int8_t tableId) {
    uint8_t maxNumRecords = DbaseDef->tables[tableId].maxNumRecords;
    return (rs_getNumRecords(tableId) < maxNumRecords);
}


bool BDB_insertRecordAfter(const int8_t tableId,
                           const uint8_t recordId) {
    writeRecordBufferIfDirty(tableId);
    uint8_t newRecordId = rs_insertRecordAfter(tableId, recordId);
    if (newRecordId == MAX_NUM_RECORDS_REACHED) {
        return false;
    }
    shiftRecordReferencesUp(tableId, recordId);
    setRecordToDefaultValues(tableId, newRecordId);
    return true;
}


// returns false if a column is DECIMAL and its value is invalid
static bool validateDecimalValue(const BDB_columnT* columnDef,
                                 const uint16_t value) {
    return (columnDef->colType != BDB_COLUMN_DECIMAL)
            || (value - columnDef->minValue) % columnDef->dec.step == 0;
}


// returns false if a column is INT_STEP and its value is invalid
static bool validateIntStepValue(const BDB_columnT* columnDef,
                                 const uint16_t value) {
    return (columnDef->colType != BDB_COLUMN_INT_STEP)
            || (value - columnDef->minValue) % columnDef->intS.step == 0;
}


static bool validateRecord(const uint8_t tableId,
                           const uint8_t recordId,
                           const uint16_t* data) {
    const BDB_recordT* recordDef = getRecordDefFromData(tableId, data);
    const uint8_t numColumns = recordDef->numColumns;
    for (uint8_t col = 0; col < numColumns; col++) {
        const BDB_columnT* columnDef = getColumnDef(recordDef, col);
        if (rc_isVirtualColumn(columnDef)) continue;
        if (       data[col] < columnDef->minValue
                || data[col] > getMaxValue(tableId, recordId, columnDef)
                || !validateDecimalValue(columnDef, data[col])
                || !validateIntStepValue(columnDef, data[col])) {
            return false;
        }
    }
    return true;
}


static void copyDataToRecordBuffer(const uint8_t tableId,
                                   const uint8_t recordId,
                                   const uint16_t* data) {
    recordBufferT* recordBuffer = &RecordBuffers[tableId];
    const BDB_recordT* recordDef = getRecordDefFromData(tableId, data);
    const uint8_t numColumns = recordDef->numColumns;
    for (uint8_t col = 0; col < numColumns; col++) {
        const BDB_columnT* columnDef = getColumnDef(recordDef, col);
        if (rc_isVirtualColumn(columnDef)) continue;
        recordBuffer->columns[col] = data[col];
    }
    recordBuffer->isModified = true;
    recordBuffer->recordId = recordId;
}


void clearRecordBuffer(const uint8_t tableId) {
    RecordBuffers[tableId].recordId = NO_RECORD_ID;
    RecordBuffers[tableId].isModified = false;
}


bool BDB_setRecord(const uint8_t tableId,
                   const uint8_t recordId,
                   const uint16_t* data) {
    writeRecordBufferIfDirty(tableId);
    copyDataToRecordBuffer(tableId, recordId, data);
    if (!validateRecord(tableId, recordId, data)) {
        clearRecordBuffer(tableId);
        return false;
    }
    return true;
}


const uint16_t* BDB_getRecord(const uint8_t tableId,
                        const uint8_t recordId) {
    fillRecordBuffer(tableId, recordId);
    return RecordBuffers[tableId].columns;
}


// column functions:

uint8_t BDB_getNumRealColumns(const uint8_t tableId,
                              const uint8_t recordId) {
    const BDB_recordT* recordDef = getRecordDefFromBuffer(tableId, recordId);
    const uint8_t numColumns = recordDef->numColumns;
    for (uint8_t col = 0; col < numColumns; col++) {
        const BDB_columnT* columnDef = getColumnDef(recordDef, col);
        if (rc_isVirtualColumn(columnDef)) {
            return col;
        }
    }
    return numColumns;
}


/*
 * NOTE: the reference column is in the same table,record as the virtual
 *  column. This is why getColumnDef is called twice with the same recordDef,
 *  just with a different column. This is also why the .recordId output comes
 *  straight from RecordBuffers[tableId].columns[refColumId]
 */
static TrcT resolveVirtualColumn(const uint8_t tableId,
                                 const uint8_t recordId,
                                 const uint8_t columnId) {
    const BDB_recordT* recordDef = getRecordDefFromBuffer(tableId, recordId);
    const BDB_columnT* columnDef = getColumnDef(recordDef, columnId);
    uint8_t refColumnId = columnDef->virt.refCol; // columnId of the REFERENCE COLUMN
    const BDB_columnT* refColumnDef = getColumnDef(recordDef, refColumnId);
    return (TrcT) {
        .tableId  = refColumnDef->ref.tableId,
        .recordId = (uint8_t)RecordBuffers[tableId].columns[refColumnId],
        .columnId = columnDef->virt.valueCol,
    };
}


uint16_t BDB_getValue(const uint8_t tableId,
                      const uint8_t recordId,
                      const uint8_t columnId) {
    const BDB_recordT* recordDef = getRecordDefFromBuffer(tableId, recordId);
    const BDB_columnT* columnDef = getColumnDef(recordDef, columnId);
    if (columnDef->colType == BDB_COLUMN_VIRTUAL) {
        TrcT target = resolveVirtualColumn(tableId, recordId, columnId);
        return BDB_getValue(target.tableId,
                            target.recordId,
                            target.columnId);
    } else if (columnDef->colType == BDB_COLUMN_STRING) {
        return INVALID_VALUE;
    }
    return RecordBuffers[tableId].columns[columnId];
}


static void setVariableRecordToDefaultValues(const uint8_t tableId,
                                             const uint8_t recordId,
                                             const BDB_recordT* recordDef) {
    setColumnsToDefaultValue(tableId, recordId, 1, recordDef);
}


static void setValue(const uint8_t tableId,
                     const uint8_t recordId,
                     const uint8_t columnId,
                     const uint16_t value) {
    RecordBuffers[tableId].columns[columnId] = value;
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    if (columnId == 0 && tableDef->numRecordDefs > 1) { // changing record type:
        const BDB_recordT* recordDef = &tableDef->recordDefs[value];
        setVariableRecordToDefaultValues(tableId, recordId, recordDef);
    }
    RecordBuffers[tableId].isModified = true;
}


// returns false if the value is out of range
bool BDB_setValue (const uint8_t tableId,
                   const uint8_t recordId,
                   const uint8_t columnId,
                   const uint16_t value) {
    const BDB_recordT* recordDef = getRecordDefFromBuffer(tableId, recordId);
    const BDB_columnT* columnDef = getColumnDef(recordDef, columnId);
    if (value < columnDef->minValue
            || value > getMaxValue(tableId, recordId, columnDef)
            || rc_isVirtualColumn(columnDef)
            || !validateDecimalValue(columnDef, value)
            || !validateIntStepValue(columnDef, value)
            || columnDef->colType == BDB_COLUMN_CHILD_TABLE) {
        return false;
    }
    setValue(tableId, recordId, columnId, value);
    return true;
}


bool BDB_changeValue(const uint8_t tableId,
                     const uint8_t recordId,
                     const uint8_t columnId,
                     int16_t delta) {
    const BDB_recordT* recordDef = getRecordDefFromBuffer(tableId, recordId);
    const BDB_columnT* columnDef = getColumnDef(recordDef, columnId);
    if (rc_isVirtualColumn(columnDef)
            || columnDef->colType == BDB_COLUMN_CHILD_TABLE) {
        return false;
    }
    if (columnDef->colType == BDB_COLUMN_DECIMAL) {
        delta *= columnDef->dec.step;
    }
    if (columnDef->colType == BDB_COLUMN_INT_STEP) {
        delta *= columnDef->intS.step;
    }
    int16_t value = BDB_getValue(tableId, recordId, columnId);
    int16_t newValue = mu_limitValue(columnDef->minValue,
                                     value + delta,
                                     getMaxValue(tableId, recordId, columnDef));
    if (newValue == value) {
        return false;
    }
    setValue(tableId, recordId, columnId, (uint16_t)(newValue));
    return true;
}


uint8_t BDB_importTable(const uint8_t tableId,
                        const uint16_t* data,
                        const uint8_t numRecords) {
    uint8_t maxNumRecords = DbaseDef->tables[tableId].maxNumRecords;
    assert(numRecords <= maxNumRecords);    // FIXME: this could become runtime error!
    rs_deleteAllRecords(tableId);
    clearRecordBuffer(tableId);
    uint16_t dataId = 0;
    uint8_t rec = 0;
    for (; rec < numRecords; rec++) {
        rs_appendRecord(tableId);
        if (!BDB_setRecord(tableId, rec, &data[dataId])) break;
        dataId += BDB_getNumRealColumns(tableId, rec);
    }
    BDB_setRecord(tableId, 0, &data[0]);
    if (tableId == ParentTableId) {
        buildParentRecordList();
    }
    return rec;
}


// parent/child


uint8_t BDB_getParentTable(const uint8_t tableId) {
    assert(ParentTableId != NOT_SET && ParentChildColumnId != NOT_SET);
    return ParentTableId;
}


uint8_t BDB_getParentRecord(const uint8_t tableId) {
    assert(ParentTableId != NOT_SET && ParentChildColumnId != NOT_SET);
    const BDB_recordT* recordDef = getRecordDefFromBuffer(ParentTableId, 0);
    const BDB_columnT* columnDef = getColumnDef(recordDef, ParentChildColumnId);
    const uint8_t minValue = (uint8_t)columnDef->minValue;
    return ParentRecordList[tableId - minValue];
}


// write text output functions:


// returns ptr to string output generated by writeValue and writeRecord
char* BDB_getWriteBuffer(void) {
    return wc_getWriteBuffer();
}


static uint8_t getMaxLengthFromTxtList(const uint8_t* const txtList,
                                       const uint8_t listSize) {
    uint8_t maxLength = 0;
    for (uint8_t i = 0; i < listSize; i++) {
        const char** dummyPtr = NULL;
        const uint8_t index = txtList[i];
        const uint8_t length = GetTxtPtr(dummyPtr, index);
        if (length > maxLength) {
            maxLength = length;
        }
    }
    return maxLength;
}


static bool isRecordTypeColumn(const uint8_t tableId,
                               const uint8_t recordId,
                               const uint8_t columnId){
    if (columnId) return false;
    const BDB_recordT* recordDef = getRecordDefFromBuffer(tableId, recordId);
    return recordDef->columns[0].colType == BDB_COLUMN_RECORD_TYPE;
}


// writes column value to writeBuffer at the current cursorposition
static void writeValue(const uint8_t tableId,
                       const uint8_t recordId,
                       const uint8_t columnId,
                       const PropT properties) {
    const bool leading0 = properties.leading0;
    const bool offset1 = properties.offset1;
    const uint16_t value = BDB_getValue(tableId, recordId, columnId);
    const BDB_recordT* recordDef = getRecordDefFromBuffer(tableId, recordId);
    const BDB_columnT* columnDef = getColumnDef(recordDef, columnId);
    switch(columnDef->colType) {
        case BDB_COLUMN_INTEGER : {
            const bool offset1 = properties.offset1;
            const uint16_t maxValue = columnDef->maxValue;
            const uint8_t numDigits = mu_getNumDigits(maxValue);
            wc_writeInteger(value + offset1, numDigits, leading0);
            break;
        }
        case BDB_COLUMN_PERCENTAGE : {
            const uint16_t maxValue = columnDef->maxValue;
            wc_writeInteger((100 * value)/maxValue, 3, leading0);
            break;
        }
        case BDB_COLUMN_INT_STEP : {
            const uint16_t maxValue = columnDef->maxValue;
            const uint8_t numDigits = mu_getNumDigits(maxValue);
            wc_writeInteger(value, numDigits, leading0);
            break;
        }
        case BDB_COLUMN_TXT_LIST : {
            assert(GetTxtPtr != NULL); // column relies on static text
            const uint8_t maxValue = (uint8_t)columnDef->maxValue;
            const uint8_t maxLen = getMaxLengthFromTxtList(columnDef->txt.list, maxValue + 1);
            const char* textFromList = NULL;
            GetTxtPtr(&textFromList, columnDef->txt.list[value]);
            wc_writeTxt(textFromList, maxLen);
            break;
        }
        case BDB_COLUMN_TXT_LIST_CLONE : { // FIXME cleanup / DRY with BDB_COLUMN_TXT_LIST?
            assert(GetTxtPtr != NULL); // column relies on static text
// writeValue of another (table,record?,)column value with properties of this column
            const uint8_t valueColumn = columnDef->valueColumn;
            const uint8_t maxValue = (uint8_t)recordDef->columns[valueColumn].maxValue;
            const uint16_t newValue = BDB_getValue(tableId, recordId, valueColumn);

            const uint8_t maxLen = getMaxLengthFromTxtList(columnDef->txt.list, maxValue);
            const char* textFromList = NULL;
            GetTxtPtr(&textFromList, columnDef->txt.list[newValue]);
            wc_writeTxt(textFromList, maxLen);
            break;
        }
        case BDB_COLUMN_DECIMAL : {
            const uint16_t maxValue = columnDef->maxValue;
            uint8_t numDigits = mu_getNumDigits(maxValue) + 1; // +1 for decimal point
            if (columnDef->dec.shift >= numDigits - 1) {
                numDigits = columnDef->dec.shift + 2; // for extra leading 0's after pt
            }
            wc_writeDecimal(value, numDigits, columnDef->dec.shift, leading0);
            break;
        }
        case BDB_COLUMN_CHAR : {
            wc_writeChar((uint8_t)value, columnDef->chr.set);
            break;
        }
        case BDB_COLUMN_CHILD_TABLE : {
            const uint16_t maxNumRecords = DbaseDef->tables[value].maxNumRecords;
            uint8_t numDigits = mu_getNumDigits(maxNumRecords);
            const uint8_t numRecords = rs_getNumRecords((uint8_t)value);
            wc_writeInteger(numRecords, numDigits, false);
            break;
        }
        case BDB_COLUMN_REFERENCE : {
            const uint8_t refTableId = columnDef->ref.tableId;
            const uint8_t refRecordId = (uint8_t)value;
            const uint8_t refColumnId = columnDef->ref.columnId;
            if (refTableId == BDB_SELF_REFERENCE
                    || isRecordTypeColumn(refTableId, refRecordId, refColumnId)) {
                // write the record number:
                const uint16_t maxValue = columnDef->maxValue + 1;
                uint8_t numDigits = mu_getNumDigits(maxValue);
                wc_writeInteger(value + offset1,  numDigits, leading0);
            } else {
                writeValue(refTableId, refRecordId, refColumnId, properties);
            }
            break;
        }
        case BDB_COLUMN_TXT_LIST_COPY : { // FIXME cleanup / DRY with BDB_COLUMN_TXT_LIST?
// writeValue of this column, using properties of another table,record,column
            const BDB_columnT* propsColumnDef = getColumnDefProps(tableId, recordId, columnDef);
            assert(GetTxtPtr != NULL); // column relies on static text
            const uint8_t maxValue = (uint8_t)propsColumnDef->maxValue;
            const uint8_t maxLen = getMaxLengthFromTxtList(propsColumnDef->txt.list, maxValue);
            const char* textFromList = NULL;
            GetTxtPtr(&textFromList, propsColumnDef->txt.list[value]);
            wc_writeTxt(textFromList, maxLen);
            break;
        }
        case BDB_COLUMN_STRING : {
            for (uint8_t c = 0; c < columnDef->str.length; c++) {
                const uint8_t columnId = columnDef->str.firstChar + c;
                writeValue(tableId, recordId, columnId, properties);
            }
            break;
        }
        case BDB_COLUMN_VIRTUAL : {
            TrcT target = resolveVirtualColumn(tableId, recordId, columnId);
            writeValue(target.tableId,
                       target.recordId,
                       target.columnId,
                       properties);
            break;
        }
        default :
            assert(0 && "Invalid column type");
    }
}


// takes startposition for writing value and returns the next empty position
uint8_t BDB_writeValue(const uint8_t tableId,
                       const uint8_t recordId,
                       const uint8_t columnId,
                       const uint8_t position) {
    wc_setCursorPosition(position);
    PropT properties =  {.leading0 = false, .offset1 = false};
    writeValue(tableId, recordId, columnId, properties);
    return wc_getCursorPosition();
}


static void writeRecordID(const uint8_t tableId,
                          uint8_t recordId,
                          const PropT properties) {
    const uint16_t maxNumRecords = DbaseDef->tables[tableId].maxNumRecords;
    uint8_t numDigits = mu_getNumDigits(maxNumRecords);
    if (properties.offset1) {
        recordId++;
    }
    wc_writeInteger(recordId, numDigits, properties.leading0);
}


static void writeNumRecords(const uint8_t tableId,
                            const PropT properties) {
    const uint16_t maxNumRecords = DbaseDef->tables[tableId].maxNumRecords;
    uint8_t numDigits = mu_getNumDigits(maxNumRecords);
    const uint8_t numRecords = rs_getNumRecords(tableId);
    wc_writeInteger(numRecords, numDigits, properties.leading0);
}


enum {
    PLAIN_TEXT,
    START_TAG,
    COLUMN_ID_TAG,
    IF_TAG_COLUMN_ID,
    IF_TAG_RIGHT_HAND_SIDE,
    IF_LITERAL_VALUE,
    IF_RECORD_ID,
    IF_OTHER_COLUMN,
    RECORD_ID_TAG,
    NUM_RECORDS_TAG,
    PARENT_TAG,
    SKIP_TO_ELSE,
    ELSE_TAG,
    SKIP_TO_ENDIF,
    ENDIF_TAG,
};


static uint8_t updateNumber(const uint8_t number,
                            const uint8_t chr) {
    assert((chr >= '0' && chr <= '9') && "invalid char");
    return number * 10 + chr - '0';
}


static bool matchTag(const char *txt,
                     uint8_t i,
                     const char tagChr) {
    return txt[i] == '{'
        && txt[i + 1] == tagChr
        && txt[i + 2] == '}';
}


void BDB_writeRecordWithFormat(const uint8_t tableId,
                               const uint8_t recordId,
                               const char* txtFormat) {
    wc_setCursorPosition(0);
    bool condition = false;
    mu_operatorT operator = MU_NO_OPERATOR;
    uint8_t number = 0;
    uint16_t value = 0;
    PropT properties = {.leading0 = false, .offset1 = false};
    uint8_t state = PLAIN_TEXT;
    for (uint8_t i = 0; txtFormat[i] != '\0'; i++) {
        const char chr = txtFormat[i];
        switch (state) {

            case PLAIN_TEXT : {
                if (chr == '{') {
                    number = 0;
                    properties.leading0 = false;
                    properties.offset1 = false;
                    state = START_TAG;
                } else {
                    wc_writeTxt(&chr, 1);
                }
                break;
            }

            case START_TAG : {
                switch (chr) {
                    case '?' :
                        assert(txtFormat[i + 1] == '&');
                        i++; // skip '&'
                        state = IF_TAG_COLUMN_ID;
                        break;
                    case ':' :
                        state = ELSE_TAG;
                        break;
                    case ';' :
                        state = ENDIF_TAG;
                        break;
                    case '#' :
                        state = RECORD_ID_TAG;
                        break;
                    case '*' :
                        state = NUM_RECORDS_TAG;
                        break;
                    case '^' :
                        state = PARENT_TAG;
                        break;
                    case 'o' :
                        properties.leading0 = true;
                        // remain in START_TAG state!
                        break;
                    case '&' :
                        state = COLUMN_ID_TAG;
                        break;
                    default :
                        assert(0 && "Invalid start character in tag");
                        break;
                }
                break;
            }

            case COLUMN_ID_TAG :
                if (chr == '}' ) {
                    writeValue(tableId, recordId, number, properties);
                    state = PLAIN_TEXT;
                } else
                if (chr == '+') {
                    properties.offset1 = true;
                } else {
                    number = updateNumber(number, chr);
                }
                break;

            case NUM_RECORDS_TAG :
                assert(chr == '}' && "invalid character in num records tag");
                writeNumRecords(tableId, properties);
                state = PLAIN_TEXT;
                break;

            case RECORD_ID_TAG :
                if (chr == '+') {
                    properties.offset1 = true;
                } else {
                    assert(chr == '}' && "invalid character in recordId tag");
                    writeRecordID(tableId, recordId, properties);
                    state = PLAIN_TEXT;
                }
                break;

            case PARENT_TAG :
                if (chr == '}' ) {
                    const uint8_t recId = BDB_getParentRecord(tableId);
                    writeValue(ParentTableId, recId, number, properties);
                    state = PLAIN_TEXT;
                } else {
                    number = updateNumber(number, chr);
                }
                break;

            case ELSE_TAG :
                assert(chr == '}' && "invalid character in else tag");
                state = condition ? SKIP_TO_ENDIF : PLAIN_TEXT;
                break;

            case ENDIF_TAG :
                assert(chr == '}' && "invalid character in endif tag");
                state = PLAIN_TEXT;
                break;

            case IF_TAG_COLUMN_ID :
                if (chr == '=' || chr == '>' || chr == '<'  || chr == '!' ) {
                    operator = tu_getOperator(&txtFormat[i]);
                    if (tu_getOperatorLength(operator) == 2) i++;
                    value = BDB_getValue(tableId, recordId, number);
                    number = 0;
                    state = IF_TAG_RIGHT_HAND_SIDE;
                } else {
                    number = updateNumber(number, chr);
                }
                break;

            case IF_TAG_RIGHT_HAND_SIDE :
                if (chr == '$') {
                    state = IF_LITERAL_VALUE;
                } else
                if (chr == '#') {
                    state = IF_RECORD_ID;
                } else
                if (chr == '&') {
                    state = IF_OTHER_COLUMN;
                } else {
                    assert(0 && "invalid character in if tag RHS");
                }
                break;

            case IF_LITERAL_VALUE :
                if (chr == '}') {
                    condition = mu_evalCondition(value, operator, number);
                    state = condition ? PLAIN_TEXT : SKIP_TO_ELSE;
                } else {
                    number = updateNumber(number, chr);
                }
                break;

            case IF_RECORD_ID :
                if (chr == '}') {
                    uint8_t rhs = recordId;
                    if (properties.offset1) {
                        rhs++;
                    }
                    condition = mu_evalCondition(value, operator, rhs);
                    state = condition ? PLAIN_TEXT : SKIP_TO_ELSE;
                } else
                if (chr == '+') {
                    properties.offset1 = true;
                } else {
                    assert(0 && "invalid character in if record tag");
                }
                break;

            case IF_OTHER_COLUMN :
                if (chr == '}') {
                    const uint16_t value2 = BDB_getValue(tableId,
                                                         recordId,
                                                         number);
                    condition = mu_evalCondition(value, operator, value2);
                    state = condition ? PLAIN_TEXT : SKIP_TO_ELSE;
                } else {
                    number = updateNumber(number, chr);
                }
                break;

            case SKIP_TO_ELSE:
                if (matchTag(txtFormat, i, ':')) {
                    i += 2;
                    state = PLAIN_TEXT;
                }
                break;

            case SKIP_TO_ENDIF:
                if (matchTag(txtFormat, i, ';')) {
                    i += 2;
                    state = PLAIN_TEXT;
                }
                break;

            default :
                assert(0 && "invalid state");
                break;

        }
    }
    uint8_t maxLen = DbaseDef->maxStringBufferSize;
    while (wc_getCursorPosition() < maxLen) {
        wc_writeTxt(" ", 1); // pad with spaces
    }
}


// writes record to writeBuffert using record format
void BDB_writeRecord(const uint8_t tableId,
                     const uint8_t recordId) {
    assert(GetTxtPtr != NULL); // this function relies on static text
    const BDB_recordT* recordDef = getRecordDefFromBuffer(tableId, recordId);
    const char* txtFormat = NULL;
    GetTxtPtr(&txtFormat, recordDef->txtFormat);
    BDB_writeRecordWithFormat(tableId, recordId, txtFormat);
}


// writes table header to writeBuffert using record format
void BDB_writeHeader(const uint8_t tableId,
                     const uint8_t recordId) {
    assert(GetTxtPtr != NULL); // this function relies on static text
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    const char* txtFormat = NULL;
    GetTxtPtr(&txtFormat, tableDef->headerFormat);
    BDB_writeRecordWithFormat(tableId, recordId, txtFormat);
}

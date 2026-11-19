// DbaseEngine.c


#include <stdint.h>
#include <stdbool.h>
#include <assert.h>


#include "DbaseEngine.h"
#include "BitsOfDataTypes.h"
#include "RecordCodec.h"
#include "RecordStore.h"


#define NO_RECORD_ID 0xFF
#define NOT_SET 0xFF
#define INVALID_VALUE 0xFFFF


typedef struct {
    uint8_t recordId;
    bool isModified;
    uint16_t *columns;
} recordBufferT;

static const BDB_dbaseDefT *DbaseDef = NULL;
static uint8_t *RawRecordBuffer = NULL;
static recordBufferT* RecordBuffers = NULL;
static uint8_t NumChildColumns = 0;
static uint8_t ParentTableId = NOT_SET;
static uint8_t ParentChildColumnId = NOT_SET;
static uint8_t *ParentRecordList = NULL; // There can only be 1 db table with children!


void de_setDbaseDef(const BDB_dbaseDefT *dbaseDef) {
    DbaseDef = dbaseDef;
}


const BDB_dbaseDefT *de_getDbaseDef(void) {
    return DbaseDef;
}


uint8_t de_getParentTableId(void) {
    assert(ParentTableId != NOT_SET && ParentChildColumnId != NOT_SET);
    return ParentTableId;
}

uint8_t de_getParentChildColumnId(void) {
    assert(ParentTableId != NOT_SET && ParentChildColumnId != NOT_SET);
    return ParentChildColumnId;
}


uint8_t de_getParentRecordId(const uint8_t tableOffset) {
    return ParentRecordList[tableOffset];
}


void de_storeRecordBuffer(const uint8_t tableId) {
    recordBufferT* recordBuffer = &RecordBuffers[tableId];
    const uint8_t recordId = recordBuffer->recordId;
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    rc_encodeRecord(recordBuffer->columns, RawRecordBuffer, tableDef);
    rs_setRawRecord(tableId, recordId, RawRecordBuffer);
}


void de_writeRecordBufferIfDirty(const uint8_t tableId) {
    recordBufferT* recordBuffer = &RecordBuffers[tableId];
    if (recordBuffer->isModified) {
        de_storeRecordBuffer(tableId);
    }
}


static void fillRecordBuffer(const uint8_t tableId,
                             const uint8_t recordId) {
    recordBufferT* recordBuffer = &RecordBuffers[tableId];
    if (recordBuffer->recordId != recordId) {
        de_writeRecordBufferIfDirty(tableId);
        recordBuffer->recordId = recordId;
        const uint8_t* rawRecord = rs_getRawRecord(tableId, recordId);
        const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
        rc_decodeRecord(rawRecord, recordBuffer->columns, tableDef);
        recordBuffer->isModified = false;
    }
}


const uint16_t* de_getRecord(const uint8_t tableId,
                             const uint8_t recordId) {
    fillRecordBuffer(tableId, recordId);
    return RecordBuffers[tableId].columns;
}


/* Returns true if a table has variable record type, false if it is fixed.
 */
bool de_hasVariableRecordType(const uint8_t tableId) {
    return DbaseDef->tables[tableId].numRecordDefs > 1;
}


bool de_isRecordTypeColumn(const uint8_t tableId,
                           const uint8_t columnId) {
    return (columnId == 0 && de_hasVariableRecordType(tableId));
}


uint8_t de_getHeaderFormatString(const uint8_t tableId) {
    return DbaseDef->tables[tableId].headerFormat;
}


uint8_t de_getRecordFormatString(const uint8_t tableId,
                                 const uint8_t recordId) {
    const BDB_recordT* recordDef = de_getRecordDef(tableId, recordId);
    return recordDef->txtFormat;
}


uint8_t de_getNumTables(void) {
    return DbaseDef->numTables;
}


uint8_t de_getMaxStringBufferSize(void) {
    return DbaseDef->maxStringBufferSize;
}


uint8_t de_getMaxNumRecords(const uint8_t tableId) {
    return DbaseDef->tables[tableId].maxNumRecords;
}


uint8_t de_getNumRecordDefs(const uint8_t tableId) {
    return DbaseDef->tables[tableId].numRecordDefs;
}


const BDB_tableT *de_getTableDef(const uint8_t tableId) {
    return &DbaseDef->tables[tableId];
}


/* NOTE: never use this directly, only through:
 *  getRecordDefFromBuffer
 *  getRecordDefFromData
 */
static const BDB_recordT* getRecordDef(const uint8_t tableId,
                                       uint8_t recordType) {
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    if (!de_hasVariableRecordType(tableId)) {
        recordType = 0;
    }
    return &tableDef->recordDefs[recordType];
}


static const BDB_columnT* getColumnDef(const BDB_recordT* recordDef,
                                       const uint8_t columnId) {
    return &recordDef->columns[columnId];
}


// makes sure record exists in buffer
const BDB_recordT* de_getRecordDef(const uint8_t tableId,
                                   const uint8_t recordId) {
    fillRecordBuffer(tableId, recordId);
    return getRecordDef(tableId, (uint8_t)RecordBuffers[tableId].columns[0]);
}


const BDB_columnT* de_getColumnDef(const uint8_t tableId,
                                   const uint8_t recordId,
                                   const uint8_t columnId) {
    const BDB_recordT* recordDef = de_getRecordDef(tableId, recordId);
    return getColumnDef(recordDef, columnId);
}


// returns an alternative columnDef, as defined in BDB_COLUMN_TXT_LIST_COPY
const BDB_columnT* de_getAltColumnDef(const uint8_t tableId,
                                   const uint8_t recordId,
                                   const BDB_columnT* columnDef) {
    assert(columnDef->colType == BDB_COLUMN_TXT_LIST_COPY);
    const uint8_t refCol = columnDef->copy.refCol;
    const uint8_t columnId = columnDef->copy.columnId;
    const BDB_recordT* recordDef = de_getRecordDef(tableId, recordId);
    const uint8_t refTableId = recordDef->columns[refCol].ref.tableId;
    const uint8_t refRecordId = (uint8_t)de_getValue(tableId, recordId, refCol);
    const BDB_recordT* refRecordDef = de_getRecordDef(refTableId, refRecordId);
    return getColumnDef(refRecordDef, columnId);
}


// FIXME: does this belong in rc?
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


uint16_t de_getMaxValue(const uint8_t tableId,
                        const uint8_t recordId,
                        const BDB_columnT* columnDef) {
    if (columnDef->colType == BDB_COLUMN_REFERENCE) {
        if (columnDef->ref.tableId == BDB_SELF_REFERENCE) {
            return rs_getNumRecords(tableId) - 1;
        } else {
            return rs_getNumRecords(columnDef->ref.tableId) - 1;
        }
    }
    if (columnDef->colType == BDB_COLUMN_TXT_LIST_COPY) {
        if (recordId == 0xFF) return 4; // FIXME ugly hack:
                            // uses max value for import of AMDB - MUST be
                            // read from referenced table
        const BDB_columnT* altColumnDef = de_getAltColumnDef(tableId,
                                                          recordId,
                                                          columnDef);
        return altColumnDef->maxValue;
    }
    return columnDef->maxValue;
}


static void clearParentRecordList(const uint8_t listSize) {
    for (uint8_t i = 0; i < listSize; i++) {
        ParentRecordList[i] = NO_RECORD_ID;
    }
}


void de_allocateParentRecordList(void) {
    const uint8_t numTables = DbaseDef->numTables;
    for (uint8_t tableId = 0; tableId < numTables; tableId++) {
        const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
        if(de_hasVariableRecordType(tableId)) {
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


void de_buildParentRecordList(void) {
    const BDB_columnT* columnDef = de_getColumnDef(ParentTableId, 0, ParentChildColumnId);
    const uint8_t minValue = (uint8_t)columnDef->minValue;
    const uint8_t listSize = (uint8_t)(columnDef->maxValue - minValue + 1);
    clearParentRecordList(listSize);
    const uint8_t numRecords = rs_getNumRecords(ParentTableId);
    for (uint8_t recId = 0; recId < numRecords; recId++) {
        const uint8_t childTableId = (uint8_t)de_getValue(ParentTableId,
                                                          recId,
                                                          ParentChildColumnId);
        const uint8_t i = childTableId - minValue;
        ParentRecordList[i] = recId;
    }
}


void de_freeParentRecordLists(void) {
    if (ParentRecordList != NULL) {
        free(ParentRecordList);
        ParentRecordList = NULL;
    }
    NumChildColumns = 0;
    ParentTableId = 0xFF;
    ParentChildColumnId = 0xFF;
}


void de_allocateRecordBuffers(void) {
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


static uint16_t getDefaultValue(const BDB_columnT* columnDef) {
    if (rc_isVirtualColumn(columnDef)) {
        return INVALID_VALUE;
    }
    return columnDef->defaultVal;
}


static void setColumnsToDefaultValue(const uint8_t tableId,
                                     const uint8_t recordId,
                                     const uint8_t startColumnId,
                                     const BDB_recordT* recordDef) {
    de_writeRecordBufferIfDirty(tableId);
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
    de_storeRecordBuffer(tableId);
    if (tableId == ParentTableId) {
        de_buildParentRecordList();
    }
}


void de_setRecordToDefaultValues(const uint8_t tableId,
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


void de_createFirstRecordInEveryTable(void) {
    uint8_t numTables = DbaseDef->numTables;
    for (uint8_t tableId = 0; tableId < numTables; tableId++) {
        uint8_t recordId = rs_appendRecord(tableId); // mandatory 1st record
        assert(recordId == 0);
        de_setRecordToDefaultValues(tableId, recordId);
    }
}


void de_freeRecordBuffers(void) {
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
}


static const BDB_recordT* getRecordDefFromData(const uint8_t tableId,
                                               const uint16_t* data) {
    return getRecordDef(tableId, (uint8_t)data[0]);
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


void de_clearRecordBuffer(const uint8_t tableId) {
    RecordBuffers[tableId].recordId = NO_RECORD_ID;
    RecordBuffers[tableId].isModified = false;
}


// returns false if a column is DECIMAL and its value is invalid
static bool validateDecimalValue(const BDB_columnT* columnDef,
                                 const uint16_t value) {
    return (columnDef->colType != BDB_COLUMN_DECIMAL)
            || (value - columnDef->minValue) % columnDef->dec.step == 0;
}


// returns false if a column is INTEGER, has a step > 1 and its value is invalid
static bool validateIntStepValue(const BDB_columnT* columnDef,
                                 const uint16_t value) {
    return columnDef->colType != BDB_COLUMN_INTEGER
            || columnDef->intg.step < 2
            || (value - columnDef->minValue) % columnDef->intg.step == 0;
}


bool de_validateColumn(const uint8_t tableId,
                       const uint8_t recordId,
                       const BDB_columnT* columnDef,
                       const uint16_t value) {
    return (value >= columnDef->minValue
            && value <= de_getMaxValue(tableId, recordId, columnDef)
            && validateDecimalValue(columnDef, value)
            && validateIntStepValue(columnDef, value));
}


static bool validateRecord(const uint8_t tableId,
                           const uint8_t recordId,
                           const uint16_t* data) {
    const BDB_recordT* recordDef = getRecordDefFromData(tableId, data);
    const uint8_t numColumns = recordDef->numColumns;
    for (uint8_t col = 0; col < numColumns; col++) {
        const BDB_columnT* columnDef = getColumnDef(recordDef, col);
        if (rc_isVirtualColumn(columnDef)) continue;
        const uint16_t value = data[col];
        if (!de_validateColumn(tableId, recordId, columnDef, value)) {
            return false;
        }
    }
    return true;
}


// returns true if data was valid
bool de_copyAndValidateRecordDataToBuf(const uint8_t tableId,
                                    const uint8_t recordId,
                                    const uint16_t *data) {
    copyDataToRecordBuffer(tableId, recordId, data);
    return validateRecord(tableId, recordId, data);
}


/*
 * NOTE: the reference column is in the same table,record as the virtual
 *  column. This is why getColumnDef is called twice with the same recordDef,
 *  just with a different column. This is also why the .recordId output comes
 *  straight from RecordBuffers[tableId].columns[refColumId]
 */
void de_resolveVirtualColumn(uint8_t *tableId,
                          uint8_t *recordId,
                          uint8_t *columnId) {
    const BDB_recordT* recordDef = de_getRecordDef(*tableId, *recordId);
    const BDB_columnT* columnDef = getColumnDef(recordDef, *columnId);
    uint8_t refColumnId = columnDef->virt.refCol; // columnId of REFERENCE COLUMN
    const BDB_columnT* refColumnDef = getColumnDef(recordDef, refColumnId);
    *recordId = (uint8_t)RecordBuffers[*tableId].columns[refColumnId];
    *tableId  = refColumnDef->ref.tableId;
    *columnId = columnDef->virt.valueCol;
}


void de_setValue(const uint8_t tableId,
                 const uint8_t recordId,
                 const uint8_t columnId,
                 const uint16_t value) {
    RecordBuffers[tableId].columns[columnId] = value;
    if (de_isRecordTypeColumn(tableId, columnId)) {
        // changing record type:
        const BDB_recordT* recordDef = &DbaseDef->tables[tableId].recordDefs[value];
        setVariableRecordToDefaultValues(tableId, recordId, recordDef);
    }
    RecordBuffers[tableId].isModified = true;
}


uint16_t de_getBufferedValue(const uint8_t tableId,
                             const uint8_t columnId) {
    return RecordBuffers[tableId].columns[columnId];
}



uint16_t de_getValue(const uint8_t tableId,
                      const uint8_t recordId,
                      const uint8_t columnId) {
    const BDB_columnT* columnDef = de_getColumnDef(tableId, recordId, columnId);
    if (columnDef->colType == BDB_COLUMN_VIRTUAL) {
        uint8_t virtTable = tableId;
        uint8_t virtRecord = recordId;
        uint8_t virtColumn = columnId;
        de_resolveVirtualColumn(&virtTable, &virtRecord, &virtColumn);
        return de_getValue(virtTable, virtRecord, virtColumn);
    } else if (columnDef->colType == BDB_COLUMN_STRING) {
        return INVALID_VALUE;
    }
    return de_getBufferedValue(tableId, columnId);
}

/*
 * BitsOfData.c
 *
 * TODO:
 * do all tests also work if recId != 0 ?
 * remove prototype definitions in RecordStore, RecordCodec
 * document recordStore/recordCodec headers
 * update README
 */

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "BitsOfData.h"
#include "BitsOfDataTypes.h"
#include "RecordStore.h"
#include "RecordCodec.h"
#include "WriteColumns.h"
#include "MathUtils.h"

#define NO_RECORD_ID 0xFF
#define INVALID_VALUE 0xFFFF


typedef struct {
    uint8_t tableId;
    uint8_t recordId;
    uint8_t columnId;
} TrcT;


typedef struct {
    uint8_t recordId;
    bool isModified;
    uint16_t* columns;
} recordBufferT;

#ifndef NDEBUG
extern void assertDbaseDefIsValid(const BDB_dbaseDefT* dbaseDef);
#endif

static recordBufferT* RecordBuffers = NULL;
static uint8_t* RawRecordBuffer = NULL;
static const BDB_dbaseDefT* DbaseDef = NULL;
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
        uint8_t* rawRecord = rs_getRawRecord(tableId, recordId);
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
    const uint8_t numTables = DbaseDef->numTables;
    for (uint8_t tableId = 0; tableId < numTables; tableId++) {
        const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
        const uint8_t numRecDefs = tableDef->numRecordDefs;
        for (uint8_t recDefId = 0; recDefId < numRecDefs; recDefId++) {
            const BDB_recordT* recordDef = &tableDef->recordDefs[recDefId];
            const uint8_t numColumns = recordDef->numColumns;
            if (numColumns > maxNumColumns) {
                maxNumColumns = numColumns;
            }
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


static void createTable(const uint8_t tableId) {
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    const uint8_t numRecords = tableDef->maxNumRecords;
    const uint8_t maxRecordSize = rc_getMaxRecordSize(tableDef);
    rs_createTable(numRecords, maxRecordSize);
}


static uint16_t getMaxValue(const BDB_columnT* columnDef) {
    if (columnDef->colType == BDB_COLUMN_REFERENCE) {
        return rs_getNumRecords(columnDef->refTable) - 1;
    } else {
        return columnDef->maxValue;
    }
}


static uint16_t getDefaultValue(const BDB_columnT* columnDef) {
    if (rc_isVirtualColumn(columnDef)) {
        return INVALID_VALUE;
    }
    assert(columnDef->defaultVal <= getMaxValue(columnDef));
    return columnDef->defaultVal;
}


static void setColumnsToDefaultValue(const uint8_t tableId,
                                     const uint8_t recordId,
                                     const uint8_t startColumnId,
                                     const BDB_recordT* recordDef) {
    RecordBuffers[tableId].recordId = recordId;
    uint8_t numColumns = recordDef->numColumns;
    for (uint8_t col = startColumnId; col < numColumns; col++) {
        BDB_columnT columnDef = recordDef->columns[col];
        uint16_t defaultValue = getDefaultValue(&columnDef);
        RecordBuffers[tableId].columns[col] = defaultValue;
    }
    storeRecordBuffer(tableId);
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
    const uint8_t numTables = DbaseDef->numTables;
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


void BDB_closeDataBase(void) {
    rs_closeRecordStore();
    freeRecordBuffers();
    DbaseDef = NULL;
}


// managing records:


uint8_t BDB_getNumRecords(const uint8_t tableId) {
    return rs_getNumRecords(tableId);
}


static bool forEachReference(const uint8_t refTableId,
                             const uint8_t refRecordId,
                             bool (*operation)(uint8_t, uint8_t, uint8_t, uint8_t)) {
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
                if (colType != BDB_COLUMN_REFERENCE || columnDef->refTable != refTableId) {
                    continue;
                }
                // a column was found that references the right table (refTableId)
                for (uint8_t rec = 0; rec < numRecords; rec++) {
                    if (hasVariableRecordDef && BDB_getValue(tableId, rec, 0) != recDef) {
                        continue;
                    }
                    // a record was found that has the right recordType
                    if (operation(tableId, rec, col, refRecordId)) {
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
    uint8_t newRecordId = rs_insertRecordAfter(tableId, recordId);
    if (newRecordId == MAX_NUM_RECORDS_REACHED) {
        return false;
    }
    shiftRecordReferencesUp(tableId, recordId);
    setRecordToDefaultValues(tableId, recordId + 1);
    return true;
}


// returns false if a column is DECIMAL and its value is invalid
static bool validateDecimalValue(const BDB_columnT* columnDef,
                                 const uint16_t value) {
    return (columnDef->colType != BDB_COLUMN_DECIMAL)
            || (value - columnDef->minValue) % columnDef->decStep == 0;
}


static bool validateRecord(const uint8_t tableId,
                           const uint16_t* data) {
    const BDB_recordT* recordDef = getRecordDefFromData(tableId, data);
    const uint8_t numColumns = recordDef->numColumns;
    for (uint8_t col = 0; col < numColumns; col++) {
        const BDB_columnT* columnDef = getColumnDef(recordDef, col);
        if (rc_isVirtualColumn(columnDef)) continue;
        if (       data[col] < columnDef->minValue
                || data[col] > getMaxValue(columnDef)
                || !validateDecimalValue(columnDef, data[col])) {
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


bool BDB_setRecord(const uint8_t tableId,
                   const uint8_t recordId,
                   const uint16_t* data) {
    if (!validateRecord(tableId, data)) {
        return false;
    }
    writeRecordBufferIfDirty(tableId);
    copyDataToRecordBuffer(tableId, recordId, data);
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
    uint8_t refColumnId = columnDef->virtRecordCol; // columnId of the REFERENCE COLUMN
    const BDB_columnT* refColumnDef = getColumnDef(recordDef, refColumnId);
    return (TrcT) {
        .tableId  = refColumnDef->refTable,
        .recordId = (uint8_t)RecordBuffers[tableId].columns[refColumnId],
        .columnId = columnDef->virtValueCol,
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
    if (       value < columnDef->minValue
            || value > getMaxValue(columnDef)
            || rc_isVirtualColumn(columnDef)
            || !validateDecimalValue(columnDef, value)) {
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
    if (rc_isVirtualColumn(columnDef)) {
        return false;
    }
    if (columnDef->colType == BDB_COLUMN_DECIMAL) {
        delta *= columnDef->decStep;
    }
    int16_t value = BDB_getValue(tableId, recordId, columnId);
    int16_t newValue = mu_limitValue(columnDef->minValue,
                                     value + delta,
                                     getMaxValue(columnDef));
    if (newValue == value) {
        return false;
    }
    setValue(tableId, recordId, columnId, (uint16_t)(newValue));
    return true;
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


// writes column value to writeBuffer
static void writeValue(const uint8_t tableId,
                       const uint8_t recordId,
                       const uint8_t columnId) {
    const uint16_t value = BDB_getValue(tableId, recordId, columnId);
    const BDB_recordT* recordDef = getRecordDefFromBuffer(tableId, recordId);
    const BDB_columnT* columnDef = getColumnDef(recordDef, columnId);
    switch(columnDef->colType) {
        case BDB_COLUMN_INTEGER : {
            const uint16_t maxValue = columnDef->maxValue;
            const uint8_t numDigits = mu_getNumDigits(maxValue);
            wc_writeInteger(value, numDigits, columnDef->leading0);
            break;
        }
        case BDB_COLUMN_INT_ZEROTXT : {
            assert(GetTxtPtr != NULL); // column relies on static text
            const uint16_t maxValue = columnDef->maxValue;
            const uint8_t numDigits = mu_getNumDigits(maxValue);
            const char* int0text = NULL;
            GetTxtPtr(&int0text, columnDef->int0txt);
            wc_writeIntZeroTxt(value, numDigits, int0text);
            break;
        }
        case BDB_COLUMN_TXT_LIST : {
            assert(GetTxtPtr != NULL); // column relies on static text
            const uint8_t maxValue = (uint8_t)columnDef->maxValue;
            const uint8_t maxLen = getMaxLengthFromTxtList(columnDef->txtList, maxValue);
            const char* textFromList = NULL;
            GetTxtPtr(&textFromList, columnDef->txtList[value]);
            wc_writeTxt(textFromList, maxLen);
            break;
        }
        case BDB_COLUMN_DECIMAL : {
            const uint16_t maxValue = columnDef->maxValue;
            uint8_t numDigits = mu_getNumDigits(maxValue) + 1; // +1 for decimal point
            if (columnDef->decimalShift >= numDigits - 1) {
                numDigits = columnDef->decimalShift + 2; // for extra leading 0's after pt
            }
            wc_writeDecimal(value, numDigits, columnDef->decimalShift);
            break;
        }
        case BDB_COLUMN_CHAR : {
            wc_writeChar((uint8_t)value, columnDef->charSet);
            break;
        }
        case BDB_COLUMN_STRING : {
            for (uint8_t c = 0; c < columnDef->strLength; c++) {
                const uint8_t columnId = columnDef->strFirstChar + c;
                writeValue(tableId, recordId, columnId);
            }
            break;
        }
        case BDB_COLUMN_VIRTUAL : {
            TrcT target = resolveVirtualColumn(tableId, recordId, columnId);
            writeValue(target.tableId,
                       target.recordId,
                       target.columnId);
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
    writeValue(tableId, recordId, columnId);
    return wc_getCursorPosition();
}


// writes record to writeBuffert using record format
uint8_t BDB_writeRecord(const uint8_t tableId,
                        const uint8_t recordId) {
    assert(GetTxtPtr != NULL); // relies on static text
    const BDB_recordT* recordDef = getRecordDefFromBuffer(tableId, recordId);
    const char* txtFormat = NULL;
    const uint8_t len = GetTxtPtr(&txtFormat, recordDef->txtFormat);
    wc_setCursorPosition(0);
    bool columnOpened = false;
    uint8_t columnId = 0;
    for (uint8_t i = 0; i < len; i++) {
        if (txtFormat[i] == '}') {
            columnOpened = false;
            writeValue(tableId, recordId, columnId);
            columnId = 0;
        } else
        if (columnOpened) {
            columnId *= 10;
            columnId += txtFormat[i] - '0';
        } else
        if (txtFormat[i] == '{') {
            columnOpened = true;
        } else {
            wc_writeTxt(&txtFormat[i], 1);
        }
    }
    return wc_getCursorPosition() - 1;
}

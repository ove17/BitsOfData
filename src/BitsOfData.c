/*
 * BitsOfData.c
 *
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

#include <stdio.h>  // FIXME: remove when stable
#define NO_RECORD_ID 0xFF
#define INVALID_VALUE 0xFFFF


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

static uint8_t (*GetTxtPtr)(const char**, const uint8_t) = NULL;


static void allocateRecordBuffers(void);
static uint8_t getMaxNumColumns(const uint8_t tableId);
static uint8_t getMaxRawRecordSize(void);

static void createTable(const uint8_t tableId);

static void createFirstRecordInEveryTable(void);

static void setRecordToDefaultValues(const uint8_t table,
                                     const uint8_t record);
static void setVariableRecordToDefaultValues(const uint8_t tableId,
                                             const uint8_t recordId,
                                             const BDB_recordT* recordDef);
static void setColumnsToDefaultValue(const uint8_t tableId,
                                     const uint8_t recordId,
                                     const uint8_t startColumnId,
                                     const BDB_recordT* recordDef);
static uint16_t getDefaultValue(const BDB_columnT* columnDef);

static void freeRecordBuffers(void);

static uint16_t getMaxValue(const BDB_columnT* columnDef);
static void setValue(const uint8_t tableId,
                     const uint8_t recordId,
                     const uint8_t columnId,
                     const uint16_t value);

static void storeRecordBuffer(const uint8_t tableId,
                              const uint8_t recordId);
static void fillRecordBuffer(const uint8_t tableId,
                             const uint8_t recordId);

static bool isRecordReferenced(const uint8_t tableId,
                               const uint8_t recordId);

static bool forEachReference(const uint8_t refTableId,
                             const uint8_t refRecordId,
                             bool (*operation)(uint8_t, uint8_t, uint8_t, uint8_t));

static bool matchFirst(const uint8_t tableId,
                       const uint8_t recordId,
                       const uint8_t columnId,
                       const uint8_t refRecordId);
static bool shiftReferenceDown(const uint8_t tableId,
                               const uint8_t recordId,
                               const uint8_t columnId,
                               const uint8_t RecordToShiftId);
static bool shiftReferenceUp(const uint8_t tableId,
                             const uint8_t recordId,
                             const uint8_t columnId,
                             const uint8_t RecordToShiftId);

static bool isRecordReferenced(const uint8_t refTableId,
                               const uint8_t refRecordId);
static bool shiftRecordReferencesDown(const uint8_t refTableId,
                                      const uint8_t refRecordId);
static bool shiftRecordReferencesUp(const uint8_t refTableId,
                                    const uint8_t refRecordId);


// returns true if a database was found and opened, false if one was created
bool BDB_openDataBase(const BDB_dbaseDefT* dbaseDef,
                      uint8_t (*txtHandler)(const char**, const uint8_t)) {
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
    //FIXME testing recordCodec requires MAX_NUM_COLUMNS == 6 :
    //   BUT WE SHOULD: assert(maxNumColumns == MAX_NUM_COLUMNS);
    // can this be fixed with a macro?
    return maxNumColumns;
}


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


static void createTable(const uint8_t tableId) {
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    const uint8_t numRecords = tableDef->maxNumRecords;
    const uint8_t maxRecordSize = rc_getMaxRecordSize(tableDef);
    rs_createTable(numRecords, maxRecordSize);
}


static void createFirstRecordInEveryTable(void) {
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
        BDB_columnT columnDef = recordDef->columns[col];
        uint16_t defaultValue = getDefaultValue(&columnDef);
        RecordBuffers[tableId].columns[col] = defaultValue;
    }
    storeRecordBuffer(tableId, recordId);
}


static uint16_t getDefaultValue(const BDB_columnT* columnDef) {
    if (rc_isVirtualColumn(columnDef)) {
        return INVALID_VALUE;
    }
    assert(columnDef->defaultVal >= columnDef->minValue);
    assert(columnDef->defaultVal <= columnDef->maxValue);
    return columnDef->defaultVal;
}


void BDB_closeDataBase(void) {
    rs_closeRecordStore();
    freeRecordBuffers();
    DbaseDef = NULL;
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



static const BDB_recordT* getRecordDef(const uint8_t tableId,
                                       const uint16_t column0value) {
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    const uint8_t numRecordDefs = tableDef->numRecordDefs;
    uint8_t recordType = (uint8_t)((numRecordDefs > 1) ? column0value : 0);
    return &tableDef->recordDefs[recordType];
}


static const BDB_columnT* getColumnDef(const uint8_t tableId,
                                       const uint8_t columnId,
                                       const uint16_t column0value ) {
    const BDB_recordT* recordDef = getRecordDef(tableId, column0value);
    return &recordDef->columns[columnId];
}



uint16_t BDB_getValue(const uint8_t tableId,
                      const uint8_t recordId,
                      const uint8_t columnId) {
    fillRecordBuffer(tableId, recordId);
    const uint16_t column0value = RecordBuffers[tableId].columns[0];
    const BDB_recordT* recordDef = getRecordDef(tableId, column0value);
    const BDB_columnT* columnDef = &recordDef->columns[columnId];
    uint16_t returnValue = 0;
    if (columnDef->colType == BDB_COLUMN_VIRTUAL) {
        uint8_t refColumnId = columnDef->virtRecordCol;
        const BDB_columnT* refColumnDef = &recordDef->columns[refColumnId];
        uint8_t refTableId = refColumnDef->refTable;
        uint8_t virtRecordId = (uint8_t)(RecordBuffers[tableId].columns[refColumnId]);
        uint8_t virtColumnId = columnDef->virtValueCol;
        returnValue = BDB_getValue(refTableId, virtRecordId, virtColumnId);
    } else if (columnDef->colType == BDB_COLUMN_STRING) {
        returnValue =  INVALID_VALUE;
    } else {
        returnValue = RecordBuffers[tableId].columns[columnId];
    }
    return returnValue;
}


// returns false if a column is DECIMAL and its value is invalid
static bool validateDecimalValue(const BDB_columnT* columnDef,
                                 const uint16_t value) {
    return (columnDef->colType != BDB_COLUMN_DECIMAL)
            || (value - columnDef->minValue) % columnDef->decStep == 0;
}


// returns false if the value is out of range
bool BDB_setValue (const uint8_t tableId,
                   const uint8_t recordId,
                   const uint8_t columnId,
                   const uint16_t value) {
    const uint16_t column0value = RecordBuffers[tableId].columns[0];
    const BDB_recordT* recordDef = getRecordDef(tableId, column0value);
    const BDB_columnT* columnDef = &recordDef->columns[columnId];
    if (       value < columnDef->minValue
            || value > getMaxValue(columnDef)
            || rc_isVirtualColumn(columnDef)
            || !validateDecimalValue(columnDef, value)) {
        return false;
    }
    fillRecordBuffer(tableId, recordId);
    setValue(tableId, recordId, columnId, value);
    return true;
}


bool BDB_changeValue(const uint8_t tableId,
                     const uint8_t recordId,
                     const uint8_t columnId,
                     int16_t delta) {
    const uint16_t column0value = RecordBuffers[tableId].columns[0];
    const BDB_recordT* recordDef = getRecordDef(tableId, column0value);
    const BDB_columnT* columnDef = &recordDef->columns[columnId];
    if (rc_isVirtualColumn(columnDef)) {
        return false;
    }
    if (columnDef->colType == BDB_COLUMN_DECIMAL) {
        delta *= columnDef->decStep;
    }
    int16_t value = BDB_getValue(tableId, recordId, columnId); // also fills RecordBuffer
    int16_t newValue = mu_limitValue(columnDef->minValue, value + delta, getMaxValue(columnDef));
    if (newValue == value) {
        return false;
    }
    setValue(tableId, recordId, columnId, (uint16_t)(newValue));
    return true;
}


static uint16_t getMaxValue(const BDB_columnT* columnDef) {
    if (columnDef->colType == BDB_COLUMN_REFERENCE) {
        return rs_getNumRecords(columnDef->refTable) - 1;
    } else {
        return columnDef->maxValue;
    }
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


void BDB_storeRecord(const uint8_t tableId,
                     const uint8_t recordId) {
    storeRecordBuffer(tableId, recordId);
}


static void storeRecordBuffer(const uint8_t tableId,
                              const uint8_t recordId) {
    recordBufferT* recordBuffer = &RecordBuffers[tableId];
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    rc_encodeRecord(recordBuffer->columns, RawRecordBuffer, tableDef);
    rs_setRawRecord(tableId, recordId, RawRecordBuffer);
}



bool validateRecord(const uint8_t tableId,
                    const uint8_t recordId,
                    const uint16_t* data) {
    const uint16_t column0value = data[0];
    const BDB_recordT* recordDef = getRecordDef(tableId, column0value);
    const uint8_t numColumns = recordDef->numColumns;
    for (uint8_t col = 0; col < numColumns; col++) {
        const BDB_columnT* columnDef = &recordDef->columns[col];
        if (       data[col] < columnDef->minValue
                || data[col] > getMaxValue(columnDef)
                || !validateDecimalValue(columnDef, data[col])) {
            return false;
        }
    }
    return true;
}


bool BDB_setRecord(const uint8_t tableId,
                   const uint8_t recordId,
                   const uint16_t* data) {
    if (!validateRecord(tableId, recordId, data)) {
        return false;
    }
    const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
    rc_encodeRecord(data, RawRecordBuffer, tableDef);
    rs_setRawRecord(tableId, recordId, RawRecordBuffer);
    return true;
}


uint16_t* BDB_getRecord(const uint8_t tableId,
                        const uint8_t recordId) {
    fillRecordBuffer(tableId, recordId);
    return RecordBuffers[tableId].columns;
}


uint8_t BDB_getNumRealColumns(const uint8_t tableId,
                              const uint8_t recordId) {
    const uint16_t column0value = RecordBuffers[tableId].columns[0];
    const BDB_recordT* recordDef = getRecordDef(tableId, column0value);
    const uint8_t numColumns = recordDef->numColumns;
    for (uint8_t col = 0; col < numColumns; col++) {
        const BDB_columnT* columnDef = &recordDef->columns[col];
        if (rc_isVirtualColumn(columnDef)) {
            return col;
        }
    }
    return numColumns;
}


static void fillRecordBuffer(const uint8_t tableId,
                             const uint8_t recordId) {
    recordBufferT* recordBuffer = &RecordBuffers[tableId];
    uint8_t* bufferedRecord = &recordBuffer->recordId;
    if (*bufferedRecord != recordId) {
        if (recordBuffer->isModified) {
            storeRecordBuffer(tableId, *bufferedRecord);
        }
        *bufferedRecord = recordId;
        uint8_t* rawRecord = rs_getRawRecord(tableId, recordId);
        const BDB_tableT* tableDef = &DbaseDef->tables[tableId];
        rc_decodeRecord(rawRecord, recordBuffer->columns, tableDef);
        recordBuffer->isModified = false;
    }
}


// managing records:


uint8_t BDB_getNumRecords(const uint8_t tableId) {
    return rs_getNumRecords(tableId);
}


bool BDB_canRecordBeAdded(const int8_t tableId) {
    uint8_t maxNumRecords = DbaseDef->tables[tableId].maxNumRecords;
    return (rs_getNumRecords(tableId) < maxNumRecords);
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


// Returns false if max. no of records was reached
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


bool BDB_canRecordBeDeleted(const uint8_t tableId,
                            const uint8_t recordId) {
    return (rs_getNumRecords(tableId) > 1 && !isRecordReferenced(tableId, recordId));
}


static bool forEachReference(const uint8_t refTableId,
                             const uint8_t refRecordId,
                             bool (*operation)(uint8_t, uint8_t, uint8_t, uint8_t)) {
    const uint8_t numTables = DbaseDef->numTables;

    for (uint8_t tab = 0; tab < numTables; tab++) {
        const BDB_tableT* tableDef = &DbaseDef->tables[tab];
        const uint8_t numRecordDefs = tableDef->numRecordDefs;
        const uint8_t numRecords = rs_getNumRecords(tab);

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
                for (uint8_t rec = 0; rec < numRecords; rec++) {
                    if (hasVariableRecordDef && BDB_getValue(tab, rec, 0) != recDef) {
                        continue;
                    }
                    // for each record that references [refTableId]:
                    if (operation(tab, rec, col, refRecordId)) {
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


static bool isRecordReferenced(const uint8_t refTableId,
                               const uint8_t refRecordId) {
    return forEachReference(refTableId, refRecordId, matchFirst);
}


static bool shiftRecordReferencesDown(const uint8_t refTableId,
                                      const uint8_t refRecordId) {
    return forEachReference(refTableId, refRecordId, shiftReferenceDown);
}


static bool shiftRecordReferencesUp(const uint8_t refTableId,
                                    const uint8_t refRecordId) {
    return forEachReference(refTableId, refRecordId, shiftReferenceUp);

}


// writeValue functions:


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


// takes startposition for writing value and returns the next empty position
uint8_t BDB_writeValue(const uint8_t tableId,
                       const uint8_t recordId,
                       const uint8_t columnId,
                       uint8_t position) {
    const uint16_t value = BDB_getValue(tableId, recordId, columnId);
    const uint16_t column0value = RecordBuffers[tableId].columns[0];
    const BDB_columnT* columnDef = getColumnDef(tableId, columnId, column0value);
    switch(columnDef->colType) {
        case BDB_COLUMN_INTEGER : {
            const uint16_t maxValue = columnDef->maxValue;
            const uint8_t numDigits = mu_getNumDigits(maxValue);
            wc_writeInteger(value, numDigits, columnDef->leading0);
            position += numDigits;
            break;
        }
        case BDB_COLUMN_INT_ZEROTXT : {
            assert(GetTxtPtr != NULL); // column relies on static text
            const uint16_t maxValue = columnDef->maxValue;
            const uint8_t numDigits = mu_getNumDigits(maxValue);
            const char* int0text = NULL;
            GetTxtPtr(&int0text, columnDef->int0txt);
            wc_writeIntZeroTxt(value, numDigits, int0text);
            position += numDigits;
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
            position += numDigits;
            break;
        }
        case BDB_COLUMN_CHAR : {
            wc_writeChar((uint8_t)value, columnDef->charSet);
            position++;
            break;
        }
        case BDB_COLUMN_STRING : {
            for (uint8_t c = 0; c < columnDef->strLength; c++) {
                const uint8_t columnId = columnDef->strFirstChar + c;
                position = BDB_writeValue(tableId, recordId, columnId, position);
            }
            break;
        }
        default :
            assert(0 && "Invalid column type");
    }
    return position;
}

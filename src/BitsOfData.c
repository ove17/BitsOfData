/*
 * BitsOfData.c
 *
 * TODO 1: (in progress)
 *  replace 3 format functions with BDB_changeFormatFieldBy()
 *
 * TODO 3:
 *  separate dbaseEngine.c (with recordBuffer, etc)
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
#include "WriteValues.h"
#include "TemplateLanguage.h"
#include "RecordWriter.h"
#include "MathUtils.h"
#include "DbaseEngine.h"


#ifndef NDEBUG
#include <stdio.h>
extern void assertDbaseDefIsValid(const BDB_dbaseDefT* dbaseDef);
#endif


void BDB_syncTable(const uint8_t tableId) {
    de_storeRecordBuffer(tableId);
}


void BDB_syncDbase(void) {
    for (uint8_t tableId = 0; tableId < de_getNumTables(); tableId++) {
        de_storeRecordBuffer(tableId);
    }
}


static void createTable(const uint8_t tableId) {
    const BDB_tableT* tableDef = de_getTableDef(tableId);
    const uint8_t maxNumRecords = de_getMaxNumRecords(tableId);
    const uint8_t maxRecordSize = rc_getMaxRecordSize(tableDef);
    rs_createTable(maxNumRecords, maxRecordSize);
}


bool BDB_openDataBase(const BDB_dbaseDefT* dbaseDef,
                      BDB_txtHandlerFunction txtHandler) {
    de_setDbaseDef(dbaseDef);
    rw_setTxtPtrFn(txtHandler);
// FIXME: remove the #ifndef and replace with externally linked function
#ifndef NDEBUG
    assertDbaseDefIsValid(de_getDbaseDef());
#endif
    wv_initBuffer(de_getMaxStringBufferSize());
    de_allocateRecordBuffers();
    de_allocateParentRecordList();
    const uint8_t numTables = de_getNumTables();
    bool dbExists = true;
    if (!rs_tryToOpenRecordStore(numTables)) {
        for (uint8_t tableId = 0; tableId < numTables; tableId++) {
            createTable(tableId);
        }
        rs_commitTables();
        de_createFirstRecordInEveryTable();
        dbExists = false;
    } else {
        de_buildParentRecordList();
    }
    return dbExists;
}


void BDB_closeDataBase(void) {
    rs_closeRecordStore();
    de_freeRecordBuffers();
    de_freeParentRecordLists();
    wv_freeBuffer();
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
    for (uint8_t tableId = 0; tableId < de_getNumTables(); tableId++) {
        const BDB_tableT* tableDef = de_getTableDef(tableId);
        const uint8_t numRecords = rs_getNumRecords(tableId);
        const bool hasVariableRecordDef = de_hasVariableRecordType(tableId);

        for (uint8_t recDef = 0; recDef < de_getNumRecordDefs(tableId); recDef++) {
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
                    if (hasVariableRecordDef
                            && BDB_getValue(tableId, rec, 0) != recDef) {
                        continue;
                    }
                    // a record was found that has the right recordType

                    // exclude self referencing column that points to its own record:
                    if (columnDef->ref.tableId == BDB_SELF_REFERENCE
                        && tableId == refTableId
                        && BDB_getValue(tableId, rec, col) == refRecordId) continue;

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
    return (rs_getNumRecords(tableId) > 1
            && !isRecordReferenced(tableId, recordId));
}


bool BDB_deleteRecord(const uint8_t tableId,
                      const uint8_t recordId) {
    if (BDB_canRecordBeDeleted(tableId, recordId)) {
        rs_deleteRecord(tableId, recordId);
        shiftRecordReferencesDown(tableId, recordId);
        if (tableId == de_getParentTableId()) {
            de_buildParentRecordList();
        }
        return true;
    }
    return false;
}


bool BDB_canRecordBeAdded(const int8_t tableId) {
    return (rs_getNumRecords(tableId) < de_getMaxNumRecords(tableId));
}


bool BDB_insertRecordAfter(const int8_t tableId,
                           const uint8_t recordId) {
    de_writeRecordBufferIfDirty(tableId);
    uint8_t newRecordId = rs_insertRecordAfter(tableId, recordId);
    if (newRecordId == MAX_NUM_RECORDS_REACHED) {
        return false;
    }
    shiftRecordReferencesUp(tableId, recordId);
    de_setRecordToDefaultValues(tableId, newRecordId);
    return true;
}


bool BDB_setRecord(const uint8_t tableId,
                   const uint8_t recordId,
                   const uint16_t* data) {
    de_writeRecordBufferIfDirty(tableId);
    if (!de_copyAndValidateRecordDataToBuf(tableId, recordId, data)) {
        de_clearRecordBuffer(tableId);
        return false;
    }
    return true;
}


const uint16_t* BDB_getRecord(const uint8_t tableId,
                              const uint8_t recordId) {
    return de_getRecord(tableId, recordId);
}


// column functions:

uint8_t BDB_getNumRealColumns(const uint8_t tableId,
                              const uint8_t recordId) {
    const BDB_recordT* recordDef = de_getRecordDef(tableId, recordId);
    const uint8_t numColumns = recordDef->numColumns;
    for (uint8_t col = 0; col < numColumns; col++) {
        const BDB_columnT* columnDef = de_getColumnDef(tableId, recordId, col);
        if (rc_isVirtualColumn(columnDef)) {
            return col;
        }
    }
    return numColumns;
}


uint16_t BDB_getValue(const uint8_t tableId,
                      const uint8_t recordId,
                      const uint8_t columnId) {
    return de_getValue(tableId, recordId, columnId);
}

// returns false if the value is out of range
bool BDB_setValue (const uint8_t tableId,
                   const uint8_t recordId,
                   const uint8_t columnId,
                   const uint16_t value) {
    const BDB_columnT* columnDef = de_getColumnDef(tableId, recordId, columnId);
    if (!de_validateColumn(tableId, recordId, columnDef, value)
            || rc_isVirtualColumn(columnDef)
            || columnDef->colType == BDB_COLUMN_CHILD_TABLE) {
        return false;
    }
    de_setValue(tableId, recordId, columnId, value);
    return true;
}


bool BDB_changeValue(const uint8_t tableId,
                     const uint8_t recordId,
                     const uint8_t columnId,
                     int16_t delta) {
    const BDB_columnT* columnDef = de_getColumnDef(tableId, recordId, columnId);
    if (rc_isVirtualColumn(columnDef)
            || columnDef->colType == BDB_COLUMN_CHILD_TABLE) {
        return false;
    }
    if (columnDef->colType == BDB_COLUMN_DECIMAL) {
        delta *= columnDef->dec.step;
    }
    if (columnDef->colType == BDB_COLUMN_INTEGER
            && columnDef->intg.step > 1) {
        delta *= columnDef->intg.step;
    }
    int16_t value = BDB_getValue(tableId, recordId, columnId);
    int16_t newValue = mu_limitValue(columnDef->minValue,
                                     value + delta,
                                     de_getMaxValue(tableId, recordId, columnDef));
    if (newValue == value) {
        return false;
    }
    de_setValue(tableId, recordId, columnId, (uint16_t)(newValue));
    return true;
}


uint8_t BDB_importTable(const uint8_t tableId,
                        const uint16_t* data,
                        const uint8_t numRecords) {
    assert(numRecords <= de_getMaxNumRecords(tableId)); // FIXME: runtime error?
    rs_deleteAllRecords(tableId);
    de_clearRecordBuffer(tableId);
    uint16_t dataId = 0;
    uint8_t rec = 0;
    for (; rec < numRecords; rec++) {
        rs_appendRecord(tableId);
        if (!BDB_setRecord(tableId, rec, &data[dataId])) break;
        dataId += BDB_getNumRealColumns(tableId, rec);
    }
    BDB_setRecord(tableId, 0, &data[0]);
    if (tableId == de_getParentTableId()) {
        de_buildParentRecordList();
    }
    return rec;
}


// parent/child


uint8_t BDB_getParentTable(void) {
    return de_getParentTableId();;
}


uint8_t BDB_getParentRecord(const uint8_t tableId) {
    uint8_t parentTableId = de_getParentTableId();
    uint8_t childColId = de_getParentChildColumnId();
    const BDB_columnT* columnDef = de_getColumnDef(parentTableId, 0, childColId);
    const uint8_t minValue = (uint8_t)columnDef->minValue;
    return de_getParentRecordId(tableId - minValue);
}


uint8_t BDB_getChildTable(const uint8_t recordId) {
    uint8_t parentTableId = de_getParentTableId();
    uint8_t parentChildColumnId = de_getParentChildColumnId();
    return (uint8_t)BDB_getValue(parentTableId, recordId, parentChildColumnId);
}


// write text output functions:


// returns ptr to string output generated by writeValue and writeRecord
char* BDB_getWriteBuffer(void) {
    return wv_getWriteBuffer();
}


// takes startposition for writing value and returns the next empty position
// TODO: check properties (what about other ones?)
uint8_t BDB_writeValue(const uint8_t tableId,
                       const uint8_t recordId,
                       const uint8_t columnId,
                       const uint8_t position) {
    wv_setCursorPosition(position);
    tl_valuePropT properties = {}; // TODO: check if all members are false
    rw_writeValue(tableId, recordId, columnId, properties);
    return wv_getCursorPosition();
}


// writes record to writeBuffert using record format
void BDB_writeRecord(const uint8_t tableId,
                     const uint8_t recordId) {
    BDB_txtHandlerFunction getTxtPtr = rw_getTxtPtrFn();
    assert(getTxtPtr != NULL);
    const char* txtFormat = NULL;
    getTxtPtr(&txtFormat, de_getRecordFormatString(tableId, recordId));
    rw_writeRecordWithFormat(tableId, recordId, txtFormat);
}


// writes table header to writeBuffert using record format
void BDB_writeHeader(const uint8_t tableId,
                     const uint8_t recordId) {
    BDB_txtHandlerFunction getTxtPtr = rw_getTxtPtrFn();
    assert(getTxtPtr != NULL);
    const char* txtFormat = NULL;
    getTxtPtr(&txtFormat, de_getHeaderFormatString(tableId));
    rw_writeRecordWithFormat(tableId, recordId, txtFormat);
}


// TODO: test
BDB_recordFieldPropertiesT *BDB_getRecordFields(const uint8_t tableId,
                                                const uint8_t recordId) {
    BDB_txtHandlerFunction getTxtPtr = rw_getTxtPtrFn();
    assert(getTxtPtr != NULL);
    const char* txtFormat = NULL;
    getTxtPtr(&txtFormat, de_getRecordFormatString(tableId, recordId));
    return rw_getRecordFieldProperties(tableId, recordId, txtFormat);
}

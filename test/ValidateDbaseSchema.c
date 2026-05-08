// ValidateDbaseSchema.c
// NOTE: only used for testing, it must be excluded from compilation on target


#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "BitsOfDataTypes.h"


static void assertColumnIntegerIsValid(const BDB_columnT* columnDef) {
    assert(columnDef->defaultVal >= columnDef->minValue);
    assert(columnDef->defaultVal <= columnDef->maxValue);
}


static void assertColumnRecordTypeIsValid(const BDB_tableT* tableDef,
                                          const BDB_columnT* columnDef,
                                          const uint8_t columnId) {
    assert(columnDef->minValue == 0);
    assert(columnDef->defaultVal == 0);
    assert(columnId == 0);
    assert(columnDef->colType == BDB_COLUMN_RECORD_TYPE);
    const uint8_t numRecDefs = tableDef->numRecordDefs;
    assert(columnDef->maxValue == numRecDefs - 1);
    assert(numRecDefs > 1);
}


static void assertColumnCharIsValid(const BDB_columnT* columnDef) {
    const uint8_t maxCharIndex = (uint8_t)columnDef->maxValue;
    assert(columnDef->charSet[maxCharIndex+1] == '\0');
    assert(columnDef->minValue == 0);
    assert(columnDef->defaultVal == 0);
}


static void assertColumnReferenceIsValid(const BDB_dbaseDefT* dbaseDef,
                                         const BDB_recordT* recordDef,
                                         const BDB_columnT* columnDef) {
    assert(columnDef->minValue == 0);
    assert(columnDef->defaultVal == 0);
    const uint8_t refTableId = columnDef->refTable;
    const uint8_t maxNumRecords = dbaseDef->tables[refTableId].maxNumRecords;
    const uint8_t numColumns = recordDef->numColumns;
    assert(refTableId < dbaseDef->numTables);
    assert(columnDef->refColumn < numColumns);
    assert(columnDef->maxValue == maxNumRecords - 1);
    //table referenced to must NOT have variable recordType:
    assert(dbaseDef->tables[refTableId].numRecordDefs == 1);

}


static void assertColumnVirtualIsValid(const BDB_dbaseDefT* dbaseDef,
                                       const BDB_recordT* recordDef,
                                       const BDB_columnT* columnDef) {
    assert(columnDef->minValue == 0);
    assert(columnDef->maxValue == 0);
    assert(columnDef->defaultVal == 0);
    const uint8_t virtualRecordCol = columnDef->virtRecordCol;
    const BDB_columnT* refColDef = &recordDef->columns[virtualRecordCol];
    const BDB_colTypeT refColType = refColDef->colType;
    assert(refColType == BDB_COLUMN_REFERENCE);
    // virtualColumn must not point to virtualColumn:
    const uint8_t refTableId = refColDef->refTable;
    const BDB_tableT* refTableDef = &dbaseDef->tables[refTableId];
    const BDB_recordT* refRrecordDef = &refTableDef->recordDefs[0];
    const uint8_t virtColumnId = columnDef->virtValueCol;
    const BDB_columnT* virtColDef = &refRrecordDef->columns[virtColumnId];
    const BDB_colTypeT virtColumnType = virtColDef->colType;
    assert(virtColumnType != BDB_COLUMN_VIRTUAL);
}


static void assertColumnStringIsValid(const BDB_recordT* recordDef,
                                      const BDB_columnT* columnDef) {
    assert(columnDef->minValue == 0);
    assert(columnDef->maxValue == 0);
    assert(columnDef->defaultVal == 0);
    const uint8_t firstCol = columnDef->strFirstChar;
    const uint8_t lastCol = firstCol + columnDef->strLength;
    for (uint8_t col = firstCol; col < lastCol; col++) {
        const BDB_columnT* charColumnDef = &recordDef->columns[col];
        assert(charColumnDef->colType == BDB_COLUMN_CHAR);
    }
}


void assertDbaseDefIsValid(const BDB_dbaseDefT* dbaseDef) {
    const uint8_t numTables = dbaseDef->numTables;
    for (uint8_t tableId = 0; tableId < numTables; tableId++) {
        const BDB_tableT* tableDef = &dbaseDef->tables[tableId];
        const uint8_t numRecDefs = tableDef->numRecordDefs;
        for (uint8_t recDefId = 0; recDefId < numRecDefs; recDefId++) {
            const BDB_recordT* recordDef = &tableDef->recordDefs[recDefId];
            const uint8_t numColumns = recordDef->numColumns;
            for (uint8_t col = 0; col < numColumns; col++) {
                const BDB_columnT* columnDef = &recordDef->columns[col];
                switch(columnDef->colType) {
                    case BDB_COLUMN_INTEGER :
                        assertColumnIntegerIsValid(columnDef);
                        break;
                    case BDB_COLUMN_RECORD_TYPE :
                        assertColumnRecordTypeIsValid(tableDef, columnDef, col);
                        break;
                    case BDB_COLUMN_REFERENCE :
                        assertColumnReferenceIsValid(dbaseDef, recordDef, columnDef);
                        break;
                    case BDB_COLUMN_VIRTUAL :
                        assertColumnVirtualIsValid(dbaseDef, recordDef, columnDef);
                        break;
                    case BDB_COLUMN_CHAR :
                        assertColumnCharIsValid(columnDef);
                        break;
                    case BDB_COLUMN_STRING :
                        assertColumnStringIsValid(recordDef, columnDef);
                        break;
                    default :
                        break;
                }
            }
        }
    }
}

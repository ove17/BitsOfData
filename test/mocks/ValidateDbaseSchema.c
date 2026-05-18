// ValidateDbaseSchema.c
// NOTE: only used for testing, it must be excluded from compilation on target


#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include "BitsOfDataTypes.h"


#define ASSERT_VALID(cond) \
do { \
    if (!(cond)) { \
        printf("\nINVALID SCHEMA @ TableId=%i RedDefId=%i Col=%i\n\n", TableId, RecDefId, Col);\
        assert(cond); \
    } \
} while (0)


static uint8_t TableId = 0;
static uint8_t RecDefId = 0;
static uint8_t Col = 0;


static void assertIntegerColumnIsValid(const BDB_columnT* columnDef) {
    assert(columnDef->maxValue > columnDef->minValue);
    assert(columnDef->defaultVal >= columnDef->minValue);
    assert(columnDef->defaultVal <= columnDef->maxValue);
}


static void assertIntStepColumnIsValid(const BDB_columnT* columnDef) {
    assert(columnDef->maxValue > columnDef->minValue);
    assert(columnDef->defaultVal >= columnDef->minValue);
    assert(columnDef->defaultVal <= columnDef->maxValue);
    const uint8_t step = columnDef->intStep;
    assert(step > 1);
    // step must not cause remainder:
    assert((columnDef->maxValue - columnDef->minValue) % step == 0);
    assert((columnDef->defaultVal - columnDef->minValue) % step == 0);
}


static void assertIntZeroTxtColumnIsValid(const BDB_columnT* columnDef) {
    assert(columnDef->minValue == 0);
    assert(columnDef->maxValue > 0);
    assert(columnDef->defaultVal <= columnDef->maxValue);
    // NOTE: assertion GetTxtPtr != NULL must be in BitsOfData.c
}


static void assertDecimalColumnIsValid(const BDB_columnT* columnDef) {
    assert(columnDef->maxValue > columnDef->minValue);
    assert(columnDef->defaultVal >= columnDef->minValue);
    assert(columnDef->defaultVal <= columnDef->maxValue);
    assert(columnDef->decimalShift > 0);
    assert(columnDef->decimalShift <= 5); // arbitrary? or practical?
    const uint8_t step = columnDef->decStep;
    assert(step == 1 || step == 2 || step == 5); // NOTE: step == 0 would cause div/0
    // step must not cause remainder:
    assert((columnDef->maxValue - columnDef->minValue) % step == 0);
    assert((columnDef->defaultVal - columnDef->minValue) % step == 0);
}


static void assertRecordTypeColumnIsValid(const BDB_tableT* tableDef,
                                          const BDB_columnT* columnDef,
                                          const uint8_t columnId) {
    assert(columnDef->minValue == 0);
    assert(columnDef->defaultVal == 0);
    assert(columnDef->maxValue > 0);
    assert(columnId == 0);
    const uint8_t numRecDefs = tableDef->numRecordDefs;
    assert(columnDef->maxValue == numRecDefs - 1);
    assert(numRecDefs > 1);
}


static void assertCharColumnIsValid(const BDB_columnT* columnDef) {
    const uint8_t maxCharIndex = (uint8_t)columnDef->maxValue;
    assert(columnDef->charSet != NULL);
    assert(columnDef->charSet[maxCharIndex + 1] == '\0');
    assert(columnDef->minValue == 0);
    assert(columnDef->defaultVal == 0);
    assert(columnDef->maxValue > 0);
}


static uint8_t getMinNumColumns(const BDB_tableT* tableDef) {
    uint8_t minNumColumns = 0xFF;
    const uint8_t numRecDefs = tableDef->numRecordDefs;
    for (uint8_t recDefId = 0; recDefId < numRecDefs; recDefId++) {
        const BDB_recordT* recordDef = &tableDef->recordDefs[recDefId];
        const uint8_t numColumns = recordDef->numColumns;
        if (numColumns < minNumColumns) {
            minNumColumns = numColumns;
        }
    }
    return minNumColumns;
}


static void assertReferenceColumnIsValid(const BDB_dbaseDefT* dbaseDef,
                                         const BDB_columnT* columnDef) {
    assert(columnDef->minValue == 0);
    assert(columnDef->defaultVal == 0);
    assert(columnDef->maxValue > 0);
    const uint8_t refTableId = columnDef->refTable;
    assert(refTableId < dbaseDef->numTables);
// make sure the reference column exists in all target record types:
    const BDB_tableT* refTableDef = &dbaseDef->tables[refTableId];
    const uint8_t minNumColumns = getMinNumColumns(refTableDef);
    assert(columnDef->refColumn < minNumColumns);
    const uint8_t maxNumRecords = dbaseDef->tables[refTableId].maxNumRecords;
    assert(columnDef->maxValue == maxNumRecords - 1);
}


static bool isReferenceColumn(const BDB_recordT* recordDef,
                              const uint8_t columnId) {
    const BDB_columnT* refColDef = &recordDef->columns[columnId];
    const BDB_colTypeT refColType = refColDef->colType;
    return refColType == BDB_COLUMN_REFERENCE;
}


static uint8_t getHighestMaxValue(const BDB_tableT* tableDef,
                                  const uint8_t columnId) {
    uint8_t highestMaxValue = 0;
    const uint8_t numRecDefs = tableDef->numRecordDefs;
    for (uint8_t recDefId = 0; recDefId < numRecDefs; recDefId++) {
        const BDB_recordT* recordDef = &tableDef->recordDefs[recDefId];
        const BDB_columnT* columnDef = &recordDef->columns[columnId];
        const uint16_t maxValue = columnDef->maxValue;
        if (maxValue > highestMaxValue) {
            highestMaxValue = (uint8_t)maxValue;
        }
    }
    return highestMaxValue;
}


static void assertCopyColumnIsValid(const BDB_dbaseDefT* dbaseDef,
                                    const BDB_recordT* recordDef,
                                    const BDB_columnT* columnDef) {
    assert(columnDef->minValue == 0);
    assert(columnDef->defaultVal == 0);
    assert(columnDef->maxValue > 0);
// refCol must be a REFERENCE column in the same table:
    const uint8_t refColId = columnDef->copyRefCol;
    assert(isReferenceColumn(recordDef, refColId));
// copyPropsCol must be < minNumColumns in the REFERENCED refTable
    const BDB_columnT* refColDef = &recordDef->columns[refColId];
    const uint8_t refTableId = refColDef->refTable;
    const BDB_tableT* refTableDef = &dbaseDef->tables[refTableId];
    uint8_t minNumColumns = getMinNumColumns(refTableDef);
    assert(columnDef->copyPropsCol < minNumColumns);
// maxValue must be highest maxValue in the target column of each variable record def:
    const uint8_t highestMaxValue = getHighestMaxValue(refTableDef, columnDef->copyPropsCol);
    assert(columnDef->maxValue == highestMaxValue);
}


static void assertVirtualColumnIsValid(const BDB_dbaseDefT* dbaseDef,
                                       const BDB_recordT* recordDef,
                                       const BDB_columnT* columnDef) {
    assert(columnDef->minValue == 0);
    assert(columnDef->defaultVal == 0);
    assert(columnDef->maxValue == 0);
// refCol must be a REFERENCE column in the same table:
    const uint8_t refColId = columnDef->virtRecordCol;
    assert(isReferenceColumn(recordDef, refColId));
// virtualColumn must not point to a virtualColumn:
    const BDB_columnT* refColDef = &recordDef->columns[refColId];
    const uint8_t refTableId = refColDef->refTable;
    const BDB_tableT* refTableDef = &dbaseDef->tables[refTableId];
    const BDB_recordT* refRrecordDef = &refTableDef->recordDefs[0];
    const uint8_t virtColumnId = columnDef->virtValueCol;
    const BDB_columnT* virtColDef = &refRrecordDef->columns[virtColumnId];
    const BDB_colTypeT virtColumnType = virtColDef->colType;
    assert(virtColumnType != BDB_COLUMN_VIRTUAL);
}


static void assertStringColumnIsValid(const BDB_recordT* recordDef,
                                      const BDB_columnT* columnDef) {
    assert(columnDef->minValue == 0);
    assert(columnDef->defaultVal == 0);
    assert(columnDef->maxValue == 0);
    const uint8_t firstCol = columnDef->strFirstChar;
    const uint8_t lastCol = firstCol + columnDef->strLength;
    for (uint8_t col = firstCol; col < lastCol; col++) {
        const BDB_columnT* charColumnDef = &recordDef->columns[col];
        assert(charColumnDef->colType == BDB_COLUMN_CHAR);
    }
}


static void assertTxtListColumnIsValid(const BDB_columnT* columnDef) {
    assert(columnDef->minValue == 0);
    assert(columnDef->maxValue > 0);
    assert(columnDef->defaultVal <= columnDef->maxValue);
    assert(columnDef->txtList != NULL);
}


void assertDbaseDefIsValid(const BDB_dbaseDefT* dbaseDef) {
    const uint8_t numTables = dbaseDef->numTables;
    for (TableId = 0; TableId < numTables; TableId++) {
        const BDB_tableT* tableDef = &dbaseDef->tables[TableId];
        const uint8_t numRecDefs = tableDef->numRecordDefs;
        for (RecDefId = 0; RecDefId < numRecDefs; RecDefId++) {
            const BDB_recordT* recordDef = &tableDef->recordDefs[RecDefId];
            const uint8_t numColumns = recordDef->numColumns;
            assert(numColumns > 0);
            bool virtualColumnPresent = false;
            for (Col = 0; Col < numColumns; Col++) {
                const BDB_columnT* columnDef = &recordDef->columns[Col];
                switch(columnDef->colType) {
                    case BDB_COLUMN_INTEGER :
                    case BDB_COLUMN_PERCENTAGE :
                        assertIntegerColumnIsValid(columnDef);
                        ASSERT_VALID(!virtualColumnPresent);
                        break;
                    case BDB_COLUMN_INT_STEP :
                        assertIntStepColumnIsValid(columnDef);
                        ASSERT_VALID(!virtualColumnPresent);
                        break;
                    case BDB_COLUMN_INT_ZEROTXT :
                        assertIntZeroTxtColumnIsValid(columnDef);
                        ASSERT_VALID(!virtualColumnPresent);
                        break;
                    case BDB_COLUMN_DECIMAL :
                        assertDecimalColumnIsValid(columnDef);
                        ASSERT_VALID(!virtualColumnPresent);
                        break;
                    case BDB_COLUMN_RECORD_TYPE :
                        assertRecordTypeColumnIsValid(tableDef, columnDef, Col);
                        ASSERT_VALID(!virtualColumnPresent);
                        break;
                    case BDB_COLUMN_REFERENCE :
                        assertReferenceColumnIsValid(dbaseDef, columnDef);
                        ASSERT_VALID(!virtualColumnPresent);
                        break;
                    case BDB_COLUMN_VIRTUAL :
                        assertVirtualColumnIsValid(dbaseDef, recordDef, columnDef);
                        virtualColumnPresent = true;
                        break;
                    case BDB_COLUMN_COPY :
                        assertCopyColumnIsValid(dbaseDef, recordDef, columnDef);
                        virtualColumnPresent = true;
                        break;
                    case BDB_COLUMN_CHAR :
                        assertCharColumnIsValid(columnDef);
                        ASSERT_VALID(!virtualColumnPresent);
                        break;
                    case BDB_COLUMN_STRING :
                        assertStringColumnIsValid(recordDef, columnDef);
                        virtualColumnPresent = true;
                        break;
                    case BDB_COLUMN_TXT_LIST :
                        assertTxtListColumnIsValid(columnDef);
                        ASSERT_VALID(!virtualColumnPresent);
                        break;
                    default :
                        assert(0 && "Invalid columnType");
                        break;
                }
            }
        }
    }
}

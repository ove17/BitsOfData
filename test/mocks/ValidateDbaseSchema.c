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


static void assertReferenceColumnIsValid(const BDB_dbaseDefT* dbaseDef,
                                         const BDB_recordT* recordDef,
                                         const BDB_columnT* columnDef) {
    assert(columnDef->minValue == 0);
    assert(columnDef->defaultVal == 0);
    assert(columnDef->maxValue > 0);
    const uint8_t refTableId = columnDef->refTable;
    const uint8_t maxNumRecords = dbaseDef->tables[refTableId].maxNumRecords;
    const uint8_t numColumns = recordDef->numColumns;
    assert(refTableId < dbaseDef->numTables);
    assert(columnDef->refColumn < numColumns);
    assert(columnDef->maxValue == maxNumRecords - 1);
    //table referenced to must NOT have variable recordType:
    assert(dbaseDef->tables[refTableId].numRecordDefs == 1);

}


static void assertVirtualColumnIsValid(const BDB_dbaseDefT* dbaseDef,
                                       const BDB_recordT* recordDef,
                                       const BDB_columnT* columnDef) {
    assert(columnDef->minValue == 0);
    assert(columnDef->defaultVal == 0);
    assert(columnDef->maxValue == 0);
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
                        assertReferenceColumnIsValid(dbaseDef, recordDef, columnDef);
                        ASSERT_VALID(!virtualColumnPresent);
                        break;
                    case BDB_COLUMN_VIRTUAL :
                        assertVirtualColumnIsValid(dbaseDef, recordDef, columnDef);
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

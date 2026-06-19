// SchemaForTesting.h


#ifndef SCHEMA_FOR_TESTING_H
#define SCHEMA_FOR_TESTING_H

#include <stdint.h>
#include <stdbool.h>
#include "BitsOfDataTypes.h"

//TODO: must also include txt enum & txt

static const char charSet[] =   " 123456789" \
                                "0_ABCDEFGH" \
                                "IJKLMNOPQR"
                                "STUVWXYZ-+" \
                                ".";

#define CHARSET_SIZE (sizeof(charSet)/sizeof(char) - 1) // -1 because of \0
#define CHARSET_MAX (CHARSET_SIZE - 1) // -1 because it starts at 0


static const uint8_t maxNumRecordsInTable2 = 4;
static const uint8_t txtList[] = { 1, 2, 0 };
#define TXT_LIST_LENGTH (sizeof(txtList) / sizeof(uint8_t))


static const BDB_columnT columnsDef[] = {
    {.maxValue = 8, .defaultVal = 7, .minValue = 3},
    {.maxValue = 4000, .defaultVal = 1234},
    {.colType = BDB_COLUMN_REFERENCE, .maxValue = maxNumRecordsInTable2 - 1, .ref = {.tableId = 2, .columnId = 7}, },
    {.colType = BDB_COLUMN_INTEGER, .maxValue = 10, .defaultVal = 2},
    {.colType = BDB_COLUMN_VIRTUAL, .virt = {.refCol = 2, .valueCol = 1}},
};

static const BDB_recordT recordDef = {
    .numColumns = 5,
    .columns = columnsDef,
};
static const BDB_recordT recordDefs0[] = {recordDef};
static const BDB_tableT table0 = {
    .maxNumRecords = 10,
    .numRecordDefs = 1,
    .headerFormat = 4,
    .recordDefs = recordDefs0
};

enum {
    RECORD_TYPE_0,
    RECORD_TYPE_1,
    NUM_RECORD_TYPES
};


static const BDB_columnT recordTypeColumn = {
    .colType = BDB_COLUMN_RECORD_TYPE,
    .maxValue = NUM_RECORD_TYPES - 1,
};

static const BDB_columnT columnsDef0[] = {
    recordTypeColumn,
    {.maxValue = 300, .defaultVal = 150},
    {.colType = BDB_COLUMN_DECIMAL, .maxValue = 100, .dec = {.shift = 4, .step = 1}},
    {.maxValue = 1024, .defaultVal = 255},
    {.colType = BDB_COLUMN_DECIMAL, .minValue = 5, .maxValue = 995, .defaultVal = 10, .dec =            {.shift = 1, .step = 5}},
    {.colType = BDB_COLUMN_TXT_LIST, .maxValue = TXT_LIST_LENGTH - 1, .txt = {.list = txtList}},
};
static const BDB_recordT recordDef0 = {
    .numColumns = 6,
    .columns = columnsDef0,
};

static const BDB_columnT columnsDef1[] = {
    recordTypeColumn,
    {.maxValue = 85, .defaultVal = 45, .minValue = 5},
    {.colType = BDB_COLUMN_REFERENCE, .maxValue = maxNumRecordsInTable2 - 1, .ref = {.tableId = 2},},
    {.colType = BDB_COLUMN_PERCENTAGE, .maxValue = 25, .defaultVal = 10},
    {.colType = BDB_COLUMN_VIRTUAL, .virt = {.refCol = 2, .valueCol = 1}},
};
static const BDB_recordT recordDef1 = {
    .numColumns = 5,
    .columns = columnsDef1,
};
static const BDB_recordT recordDefs1[] = {recordDef0, recordDef1};

static const BDB_tableT table1 = {
    .maxNumRecords = 20,
    .numRecordDefs = NUM_RECORD_TYPES,
    .recordDefs = recordDefs1
};

static const BDB_columnT columnsDef2[] = {
    {.colType = BDB_COLUMN_INT_STEP, .maxValue = 248, .defaultVal = 24, .intS = {.step = 8}},
    {.colType = BDB_COLUMN_CHAR, .maxValue = CHARSET_MAX, .chr = {.set = charSet}},
    {.colType = BDB_COLUMN_CHAR, .maxValue = CHARSET_MAX, .chr = {.set = charSet}},
    {.colType = BDB_COLUMN_CHAR, .maxValue = CHARSET_MAX, .chr = {.set = charSet}},
    {.colType = BDB_COLUMN_CHAR, .maxValue = CHARSET_MAX, .chr = {.set = charSet}},
    {.colType = BDB_COLUMN_CHILD_TABLE, .minValue = 3, .maxValue = 6},
    {.colType = BDB_COLUMN_REFERENCE, .maxValue = maxNumRecordsInTable2 - 1, .ref = {.tableId = BDB_SELF_REFERENCE}},
    {.colType = BDB_COLUMN_STRING, .str = {.firstChar = 1, .length = 4}}
};
static const BDB_recordT recordDef2 = {
    .numColumns = 8,
    .txtFormat = 3, // 4th in test cpp
    .columns = columnsDef2,
};
static const BDB_recordT recordDefs2[] = {recordDef2};
static const BDB_tableT table2 = {
    .maxNumRecords = maxNumRecordsInTable2,
    .numRecordDefs = 1,
    .recordDefs = recordDefs2
};


static const BDB_columnT columnsDef3[] = {
    {.colType = BDB_COLUMN_INT_STEP, .maxValue = 248, .defaultVal = 24, .intS = {.step = 8}},
    {.colType = BDB_COLUMN_CHAR, .maxValue = CHARSET_MAX, .chr = {.set = charSet}},
};
static const BDB_recordT recordDef3 = {
    .numColumns = 2,
    .txtFormat = 3, // 4th in test cpp
    .columns = columnsDef3,
};
static const BDB_recordT recordDefs3[] = {recordDef3};
static const BDB_tableT table3 = {
    .maxNumRecords = 25,
    .numRecordDefs = 1,
    .hasParent = true,
    .recordDefs = recordDefs3
};

static const BDB_tableT tables[] = { table0, table1, table2, table3, table3, table3, table3};
const BDB_dbaseDefT dbaseDef = {
    .numTables = 7,
    .tables = tables,
    .maxStringBufferSize = 21,
};

#endif

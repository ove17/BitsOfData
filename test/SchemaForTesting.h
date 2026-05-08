// SchemaForTesting.h


#ifndef SCHEMA_FOR_TESTING_H
#define SCHEMA_FOR_TESTING_H

#include <stdint.h>
#include <stdbool.h>
#include "BitsOfDataTypes.h"


static const char charSet[] =   " 123456789" \
                                "0_ABCDEFGH" \
                                "IJKLMNOPQR"
                                "STUVWXYZ-+" \
                                ".";

#define CHARSET_SIZE (sizeof(charSet)/sizeof(char) - 1) // -1 because of trailing \0
#define CHARSET_MAX (CHARSET_SIZE - 1) // -1 because it starts at 0


static const uint8_t maxNumRecordsInTable2 = 15;

static const BDB_recordT recordDef = {
    .numColumns = 5,
    .columns = {
        {.maxValue = 8, .defaultVal = 7, .minValue = 3},
        {.maxValue = 4000, .defaultVal = 1234,},
        {.colType = BDB_COLUMN_REFERENCE, .refTable = 2, .maxValue = maxNumRecordsInTable2 - 1,},
        {.maxValue = 3, .defaultVal = 2, .minValue = 2},
        {.colType = BDB_COLUMN_VIRTUAL, .virtRecordCol = 2, .virtValueCol = 1},
    },
};
static const BDB_recordT recordDefs0[] = {recordDef};
static const BDB_tableT table0 = {
    .maxNumRecords = 10,
    .numRecordDefs = 1,
    .recordDefs = recordDefs0
};

enum {
    RECORD_TYPE_0,
    RECORD_TYPE_1,
    NUM_RECORD_TYPES
};


static const BDB_columnT recordTypeColumn = {
    .colType = BDB_COLUMN_RECORD_TYPE,
    .minValue = 0,
    .maxValue = NUM_RECORD_TYPES - 1,
    .defaultVal = 0
};

static const BDB_recordT recordDef0 = {
    .numColumns = 5,
    .columns = {
        recordTypeColumn,
        {.maxValue = 300, .defaultVal = 150},
        {.colType = BDB_COLUMN_DECIMAL, .maxValue = 100, .decimalShift = 4, .decStep = 1},
        {.maxValue = 1024, .defaultVal = 255},
        {.colType = BDB_COLUMN_DECIMAL, .minValue = 5, .maxValue = 995, .defaultVal = 10, .decimalShift = 1, .decStep = 5},
    },
};
static const BDB_recordT recordDef1 = {
    .numColumns = 3,
    .columns = {
        recordTypeColumn,
        {.maxValue = 85, .defaultVal = 45, .minValue = 5},
        {.colType = BDB_COLUMN_REFERENCE, .refTable = 2, .maxValue = maxNumRecordsInTable2 - 1,},
        {.colType = BDB_COLUMN_VIRTUAL, .virtRecordCol = 2, .virtValueCol = 1},
    },
};
static const BDB_recordT recordDefs1[] = {recordDef0, recordDef1};

static const BDB_tableT table1 = {
    .maxNumRecords = 20,
    .numRecordDefs = NUM_RECORD_TYPES,
    .recordDefs = recordDefs1
};


static const BDB_recordT recordDef2 = {
    .numColumns = 6,
    .columns = {
        {.colType = BDB_COLUMN_INTEGER, .maxValue = 255, .leading0 = true,},
        {.colType = BDB_COLUMN_CHAR, .maxValue = CHARSET_MAX, .charSet = charSet},
        {.colType = BDB_COLUMN_CHAR, .maxValue = CHARSET_MAX, .charSet = charSet},
        {.colType = BDB_COLUMN_CHAR, .maxValue = CHARSET_MAX, .charSet = charSet},
        {.colType = BDB_COLUMN_CHAR, .maxValue = CHARSET_MAX, .charSet = charSet},
        {.colType = BDB_COLUMN_STRING, .strFirstChar = 1, .strLength = 4},
    },
};
static const BDB_recordT recordDefs2[] = {recordDef2};
static const BDB_tableT table2 = {
    .maxNumRecords = maxNumRecordsInTable2,
    .numRecordDefs = 1,
    .recordDefs = recordDefs2
};


static const BDB_tableT tables[] = { table0, table1, table2};
const BDB_dbaseDefT dbaseDef = {
    .numTables = 3,
    .tables = tables,
    .maxStringBufferSize = 21,
};

#endif

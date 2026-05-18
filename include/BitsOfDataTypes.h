/*
 * BitsOfDataTypes.h
 *
 * Contains public types for defining a database schema.
 *
 * Data values can be accessed in the form of uint16_t's (set, get, change).
 * Data values and records can be represented as strings, allowing formatting.
 */

#ifndef BITS_OF_DATA_TYPES_H
#define BITS_OF_DATA_TYPES_H

#include <stdint.h>
#include <stdbool.h>


#define BDB_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


/*
 * Columns:
 * ========
 *
 * Each record holds one or more columns, Each column value is an integer with
 *  a .minValue, a .maxValue and a .defaultVal
 * Each column may store between 1 and 16 bits
 * Columns have a .colType property which determines the meaning/presentation
 *  of the value.
 */
typedef enum {
    BDB_COLUMN_INTEGER,         // the default coltype
    BDB_COLUMN_PERCENTAGE,      // percentage of .maxValue
    BDB_COLUMN_INT_STEP,        // an integer that increases by a step value
    BDB_COLUMN_INT_ZEROTXT,	    // writes text instead of 0
    BDB_COLUMN_DECIMAL,         // introduces a decimal point
    BDB_COLUMN_RECORD_TYPE,	    // for variable record types
    BDB_COLUMN_TXT_LIST,        // the value is an index to a list of strings
    BDB_COLUMN_CHAR,            // the value is the index of a character set
    BDB_COLUMN_REFERENCE,	    // the value is the recordId of another table
    BDB_COLUMN_COPY,            // the value has the properties from another column
// the following columns are virtual and hold no data:
    BDB_COLUMN_STRING,		    // virtual column that points to CHAR columns
    BDB_COLUMN_VIRTUAL,		    // points to column in another table
} BDB_colTypeT;

/*
 * In the record definition, virtual columns (without data) MUST come AFTER the
 *  last column with data
 */


typedef struct {
    BDB_colTypeT colType;
    uint16_t minValue;
    uint16_t maxValue;
    uint16_t defaultVal;
    union {
/* BDB_COLUMN_INTEGER:
 *
 * This is the default column type
 *
 * Constraints:
 *      minValue, defaultVal, maxValue
 *
 * getValue:                returns actual value
 * setValue, changeValue:   act directly on the data value
 * writeValue:              right-aligned numerical value optionally with
 *                          leading 0
 *
 * Parameters:  */
        struct {
            bool leading0;
        };

/* BDB_COLUMN_PERCENTAGE:
 *
 * Numeric value that is a % of .maxValue
 *
 * Constraints:
 *      minValue, defaultVal, maxValue
 *
 * getValue:                returns actual integer value
 * setValue, changeValue:   act directly on the data value
 * writeValue:              right-aligned numerical value between
 *                          100* .minValue/.maxValue and 100 * value/.maxValue
 *
 * No parameters
 */
        // no struct!

/* BDB_COLUMN_INT_STEP:
 *
 * Numeric value that can only be a multiple of .intStep
 *
 * Constraints:
 *      minValue, defaultVal, maxValue: MUST all be divisible by .intStep
 *
 * getValue:                returns actual value
 * setValue:                must be divisible by .intStep
 * changeValue:             changeValue by 1 increases value by .intStep
 * writeValue:              right-aligned numerical value
 *
 * Parameters:  */
        struct {
            int8_t intStep;
        };


/* BDB_COLUMN_INT_ZEROTXT:
 *
 * Numeric value, but a text string is displayed instead of 0
 *
 * Constraints:
 *      defaultVal, maxValue
 *      minValue MUST be 0 (default)
 *
 * getValue:                returns actual value
 * setValue, changeValue:   act directly on the data value
 * writeValue:              right-aligned numerical value, OR the text with
 *                          index .int0txt if the value == 0
 *
 * Parameters:  */
        struct {
            uint8_t int0txt;
        };

/* BDB_COLUMN_DECIMAL:
 *
 * Numeric value with fixed decimal point.
 *
 * Constraints:
 *      minValue, defaultVal, maxValue
 *
 * getValue:                returns actual integer value
 * setValue, changeValue:   act directly on the data value
 * writeValue:              right-aligned numerical value with decimal point,
 *                          the decimal point is shifted by .decimalShift digits
 *                          the value can be limited to multiples of .decStep
 *                          .decStep can have values 1, 2 or 5.
 *
 * Parameters:  */
        struct {
            uint8_t decimalShift;   // left-shift of decimal point: >0 && <=5
            uint8_t decStep;        // allows steps of 1, 2 or 5
        };

/* BDB_COLUMN_RECORD_TYPE:
 *
 * Numeric value that indicates the record type for a table with a variable
 * record types
 *
 * Constraints:
 *      .maxValue   must be equal to the number of record types - 1
 *      minValue, defaultVal MUST be 0 (default)
 *
 * getValue:                returns actual integer value
 * setValue, changeValue:   act directly on the data value
 * writeValue:              returns no text
 *
 * No parameters
 */
        // no struct!

/* BDB_COLUMN_TXT_LIST:
 *
 * The value is an index to a list of strings
 *
 * Constraints:
 *      defaultVal, maxValue
 *      minValue MUST be 0 (default)
 *
 * getValue:                returns actual value
 * setValue, changeValue:   act directly on the data value
 * writeValue:              returns the string .txtList[value]
 *
 * Parameters:  */
        struct { // TXT_LIST
            const uint8_t* txtList;
        };

/* BDB_COLUMN_CHAR:
 *
 * The value is an index to a character set
 *
 * Constraints:
 *      defaultVal, maxValue
 *      minValue MUST be 0 (default)
 *
 * getValue:                returns actual value
 * setValue, changeValue:   act directly on the data value
 * writeValue:              single character .charSet[value]
 *
 * Parameters:  */
        struct {
            const char* charSet;
        };

/* BDB_COLUMN_REFERENCE:
 *
 * The value is the recordId of another table
 *
 * Constraints:
 *      defaultVal
 *      minValue MUST be 0 (default)
 *      maxValue MUST be the maximum number of records in the referenced table
 *
 * getValue:                returns actual value
 * setValue, changeValue:   act directly on the data value
 * writeValue:              returns the string representation of the value in
 *                          .refTable, value (=recordId), .refColumn
 *
 * Parameters:  */
        struct {
            uint8_t refTable;
            uint8_t refColumn;  // column id in the target table
        };

/* BDB_COLUMN_COPY:
 *
 * The value has the properties from another column
 * (only) makes sense if the target table has variable record type
 *
 * Constraints:
 *      minValue and defaultVal MUST be 0 (default)
 *      maxValue must be the highest maxvalue of the referenced columns
 *
 * The target properties are determined by:
 *  tableId:    .refTable of BDB_COLUMN_REFERENCE .copyRefCol
 *  recordId:   value in BDB_COLUMN_REFERENCE .copyRefCol
 *  columnId:   .copyPropsCol
 *
 * getValue:                returns actual value
 * setValue, changeValue:   act directly on the data value
 * writeValue:              returns the string representation of value
 *                          according to the column properties of target
 *
 * Parameters:  */
        struct {
            uint8_t copyRefCol;     // columnId of REFERENCE_COLUMN in the same table
            uint8_t copyPropsCol;   // columnId of target column in referenced table
        };

/* BDB_COLUMN_STRING:
 *
 * Virtual column that points to CHAR columns
 * The string column holds NO data.
 *
 * Constraints: none, no data
 *
 * getValue:                n/a
 * setValue, changeValue:   n/a
 * writeValue:              returns the string starting with CHAR column
 *                          .strFirstChar and the following .strLength char
 *                          columns in the same table
 *
 * Parameters:  */
        struct {
            uint8_t strFirstChar;
            uint8_t strLength;
        };

/* BDB_COLUMN_VIRTUAL:
 *
 * A virtual column points to a column in another table
 * It holds NO data.
 *
 * Constraints: none, no data
 *
 * The target value is determined by:
 *  tableId:    .refTable of BDB_COLUMN_REFERENCE .virtRecordCol
 *  recordId:   value in BDB_COLUMN_REFERENCE .virtRecordCol
 *  columnId:   .virtValueCol
 *
 * getValue:                returns the target value
 * setValue, changeValue:   n/a
 * writeValue:              returns the string representation of the target
 *                          value, as determined by the target properties
 *
 * Parameters:  */
        struct {
            uint8_t virtRecordCol; // columnId of REFERENCE_COLUMN in the same table
            uint8_t virtValueCol;  // columnId of target value in referenced table
        };
    };
} BDB_columnT;


/*
 * Records:
 * ========
 *
 * Every record in a table has the same length in bytes
 * Each table has one or more record definitions that define its columns:
 *  + if there is a single recordDef, all records have this type
 *  + in case of multiple recordDef's, it is a variable recordDef table:
 *      . The first column of every record definition must be identical and
 *          have .colType = BDB_COLUMN_RECORD_TYPE
 *      . each record takes up the same space, regardless of its size in bytes
 */
typedef struct {
    uint8_t numColumns;
    uint8_t txtFormat;   // id of txt that contains format for writeRecord
    const BDB_columnT* columns;
} BDB_recordT;


/*
 * There are 2 scenarios:
 * 1) the table has only 1 type of record:
 *      - numRecordDefs = 1
 *      - recordDefs[0] is the definition of its record
 * 2) the table has a variable record type:
 *      - numRecordDefs > 1
 *      - recordDefs[] holds an array of record definitions
 *      - recordDefs[i].columns[0].colType MUST BE BDB_COLUMN_RECORD_TYPE
 */
typedef struct {
    uint8_t maxNumRecords;
    uint8_t numRecordDefs;
    const BDB_recordT* recordDefs;
} BDB_tableT;


typedef struct {
    uint8_t numTables;
    const BDB_tableT* tables;
    uint8_t maxStringBufferSize;      // for writeColumns functions
} BDB_dbaseDefT;


#endif

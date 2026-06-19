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

#define BDB_SELF_REFERENCE 0xFF // for BDB_columnT.ref.tableId
#define BDB_MAX_NUM_CHILD_COLUMNS 1     // TODO: in validation.c: add check for max


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
    BDB_COLUMN_DECIMAL,         // introduces a decimal point
    BDB_COLUMN_RECORD_TYPE,	    // for variable record types
    BDB_COLUMN_TXT_LIST,        // the value is an index to a list of strings
    BDB_COLUMN_TXT_LIST_COPY,   // the value uses the columnDef from a TXT_LIST
                                //  in another table
    BDB_COLUMN_CHAR,            // the value is the index of a character set
    BDB_COLUMN_CHILD_TABLE,     // the value is the tableId of another table
    BDB_COLUMN_REFERENCE,	    // the value is the recordId of another table
// the following columns are virtual and hold no data:
    BDB_COLUMN_STRING,		    // virtual column that points to CHAR columns
    BDB_COLUMN_VIRTUAL,		    // points to column in another table
    BDB_COLUMN_TXT_LIST_CLONE,  // writes value from another column as TXT_LIST
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

    uint8_t valueColumn; // FIXME: column for BDB_COLUMN_TXT_LIST_CLONE

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
 * writeValue:              right-aligned numerical value
 *
 * No parameters:  */

        // no struct!

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
            int8_t step;
        } intS;

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
            uint8_t shift;      // left-shift of decimal point: >0 && <=5
            uint8_t step;       // allows steps of 1, 2 or 5
        } dec;

/* BDB_COLUMN_RECORD_TYPE:
 *
 * Numeric value that indicates the record type for a table with a variable
 * record type
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
            const uint8_t* list;
        } txt;

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
            const char* set;
        } chr;

/* BDB_COLUMN_CHILD_TABLE
 *
 * The value is the tableId of another table: creates a 1:1 unique relationship
 *  between a record of this table and an entire other table
 *
 * All tables in the target range (as set by minValue, maxValue) MUST have the
 * following two parameters set:
 *      .parentTableId
 *      .parentChildColumnId
 * to the table, column of the current BDB_COLUMN_CHILD_TABLE
 *
 * Constraints:
 *      defaultVal must not be set, it is set automatically
 *      minValue, maxValue
 *
 * getValue:                returns tableId
 * setValue, changeValue    the value is set automatically, these functions
 *                              can not be used
 * writeValue:              returns a string containing the childs number of
 *                          records
 *
 * No Parameters:  */
    // no struct

/* BDB_COLUMN_REFERENCE:
 *
 * The value is the recordId in another table
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
 * Setting tableId to BDB_SELF_REFERENCE will reference another record in the
 *  same table.
 *
 * Parameters:  */
        struct {
            uint8_t tableId;
            uint8_t columnId;  // column id in the target table
        } ref;

/* BDB_COLUMN_TXT_LIST_COPY:
 *
 * The value uses the columnDef from a column in another table
 * (only) makes sense if the target table has variable record type
 *
 * Constraints:
 *      minValue and defaultVal MUST be 0 (default)
 *      maxValue must be the highest maxvalue of the referenced columns
 *
 * The target properties are determined by:
 *  tableId:    .refTable of BDB_COLUMN_REFERENCE .copy.refCol
 *  recordId:   value in BDB_COLUMN_REFERENCE .copy.refCol
 *  columnId:   .copy.columnId
 *
 * getValue:                returns actual value
 * setValue, changeValue:   act directly on the data value
 *                              BUT: use the maxValue from the target!
 * writeValue:              returns the string representation of value
 *                          according to the column properties of target
 *
 * Parameters:  */
        struct {
            uint8_t refCol;     // columnId of REFERENCE_COLUMN in the same table
            uint8_t columnId;   // columnId of target column in referenced table
        } copy;

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
            uint8_t firstChar;
            uint8_t length;
        } str;

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
            uint8_t refCol; // columnId of REFERENCE_COLUMN in the same table
            uint8_t valueCol;  // columnId of target value in referenced table
        } virt;
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
    uint8_t headerFormat;
    bool hasParent;
    const BDB_recordT* recordDefs;
} BDB_tableT;


typedef struct {
    uint8_t numTables;
    const BDB_tableT* tables;
    uint8_t maxStringBufferSize;      // for writeColumns functions
} BDB_dbaseDefT;


#endif

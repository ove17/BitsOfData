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
    BDB_COLUMN_INT_STEP,        // an integer that increases by a step value
    BDB_COLUMN_INT_ZEROTXT,	    // writes text instead of 0
    BDB_COLUMN_DECIMAL,         // introduces a decimal point
    BDB_COLUMN_RECORD_TYPE,	    // for variable record types
    BDB_COLUMN_TXT_LIST,        // the value is an index to a list of strings
    BDB_COLUMN_REFERENCE,	    // the value is the recordId of another table
    BDB_COLUMN_CHAR,            // the value is the index of a character set
    BDB_COLUMN_STRING,		    // virtual column, but points to CHAR columns
    BDB_COLUMN_VIRTUAL,		    // points to column in another table
} BDB_colTypeT;

/*
 * In the record definition, virtual columns (without data) MUST come AFTER the
 *  last column with data
 *
                                         setValue
                             getValue   changeValue	 printValue
 BDB_COLUMN_RECORD_TYPE     recordDefId		ok		   n/a
 BDB_COLUMN_INTEGER             int			ok			ok
 BDB_COLUMN_INT_STEP            int		   step			ok
 BDB_COLUMN_INT_ZEROTXT         int			ok			ok
 BDB_COLUMN_DECIMAL          int*step		ok			ok
 BDB_COLUMN_CHAR                int			ok			ok
 BDB_COLUMN_TXT_LIST            int			ok			ok
 BDB_COLUMN_REFERENCE        recordId       ok        refCol

 // the following columns hold no data:
 BDB_COLUMN_VIRTUAL          targetCol		n/a		 targetCol
 BDB_COLUMN_STRING              n/a			n/a			ok

 */

typedef struct {
    BDB_colTypeT colType;
    uint16_t minValue;
    uint16_t maxValue;
    uint16_t defaultVal;
    union {
        struct { // INTEGER:
            bool leading0;
        };
        struct { // INT_STEP:
            int8_t intStep;
        };
        struct { // DECIMAL:
            uint8_t decimalShift;   // left-shift of decimal point: >0 && <=5
            uint8_t decStep;        // allows steps of 1, 2 or 5
        };
        struct { // INT_ZEROTXT:
            uint8_t int0txt;        // id of static text to display if value == 0
        };
        struct { // TXT_LIST
            const uint8_t* txtList;	// list of static text id's
        };
        struct { // CHAR:
            const char* charSet;
        };
        struct { // STRING: virtual column, NO DATA!
            uint8_t strFirstChar;   // index of a CHAR column in the same table
            uint8_t strLength;		// number of CHARs (immediately following the 1st)
        };
        struct { // REFERENCE: data value = recordId in other table
            uint8_t refTable;
            uint8_t refColumn; // in the referenced table, generally a string column
        };
        struct { // VIRTUAL: holds no data, points to a column in a referenced table
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

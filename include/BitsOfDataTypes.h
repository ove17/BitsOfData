/*
 * BitsOfDataTypes.h
 *
 */

#ifndef __bits_of_data_types_h__
#define __bits_of_data_types_h__

#include <stdint.h>
#include <stdbool.h>


typedef enum {
    BDB_COLUMN_RECORD_TYPE,	// for variable records
    BDB_COLUMN_INTEGER,
    BDB_COLUMN_INTEGER_ZEROVAL,	// prints a string instead of 0
    BDB_COLUMN_DECIMAL,
    BDB_COLUMN_CHAR,
    BDB_COLUMN_STRING,		// no data, points to CHAR's
    BDB_COLUMN_SYMBOL,
    BDB_COLUMN_KEY_VALUE,
    BDB_COLUMN_KEY_VALUES,
    BDB_COLUMN_REFERENCE,	// reference to a record (, column) in another table
    BDB_COLUMN_VIRTUAL,		// no data, column in another table
    //	BDB_NUM_COLUMN_TYPES
} BDB_colTypeT;

/*
 * General approach: custom type is preferred over extra parameters
 *
 g etValue	changeV*alue	printValue			parameters
 DB_COLUMN_RECORD_TYPE	recordType		ok			n/a			-
 DB_COLUMN_INTEGER			val			ok			ok			.leading0, .intMultiplier
 DB_COLUMN_INTEGER_ZEROVAL	val			ok			ok			.leading0, .zeroStr(default='0')
 DB_COLUMN_DECIMAL			int			ok			ok			.numDecimals, .decMultiplier
 DB_COLUMN_CHAR				int			ok			ok			.charSet
 DB_COLUMN_STRING			n/a			n/a			ok			.strFirstChar, .strLength
 DB_COLUMN_SYMBOL			int			ok			ok			.symbolListId
 DB_COLUMN_KEY_VALUE			int			ok			ok			.keyValueListId
 DB_COLUMN_KEY_VALUES		int			ok			ok			.keyValueListIds
 DB_COLUMN_REFERENCE		recordId		ok		target params	.refTable, refColumn(=string)
 DB_COLUMN_VIRTUAL			int			n/a			n/a			.targetColumn

 *
 */


// TODO: is BOOLEAN een apart type?
typedef struct {
    BDB_colTypeT colType;
    uint16_t minValue; // default = 0
    uint16_t maxValue;
    union {
        struct { 	 // INTEGER:
//            uint8_t intMultiplier;	// necessary?
            bool    intLeading0;
        };
        struct { // INTEGER_ZEROVAL:
    // TODO: apply offset after checking for int0string (inputNumber for trainTable)
            uint8_t int0String;	// string to display if value == 0
        };
        struct { // DECIMAL:
            uint8_t decNumdecimals;	// >0
            uint8_t decMultiplier; // >0 - enables e.g. 0.2 or 0.5 steps
        };
        struct { // CHAR:
            uint8_t* charSet;		// there may be more than 1 (?)
        };
        struct { // STRING: virtual column, NO DATA!
            uint8_t strFirstChar;   // index of a CHAR column in the same table
            uint8_t strLength;		// number of CHARs (immediately following the 1st)
        };
        struct { // SYMBOL:
            uint8_t* symbolList;	// list of custom symbols (non-ascii)
        };
        struct { // STRING_LIST
            uint8_t* stringList;	// list of (translatable) string id
        };
        struct { // STRING_LISTS
            uint8_t* stringLists;	//  list of (list of string ids)
            uint8_t stringListColumn;	// column that determines which list is used
        };
        struct { // REFERENCE: data value = recordId in other table
            uint8_t refTable;
            uint8_t refColumn; // in the reference table, generally a string column
        };
        struct { // VIRTUAL: points to a column in another table, NO DATA!
            uint8_t virtualRecordColumn; // REFERENCE_COLUMN in this table
            uint8_t virtualValueColumn; // alternative column in the reference table
        };
    };
} BDB_columnT;


// recordType determines display format!!
//	NOTE: so settings can be record(type)s, rather than columns! Fewer exceptions!
typedef struct {
    uint8_t numColumns;
    uint8_t formatString;   // id of (translatable) string
    BDB_columnT columns[];   // pointer to array of column definitions
} BDB_recordT;


typedef struct {
    uint8_t maxNumRecords;
//    uint8_t recordSize; // in bytes FIXME: komt uit RecordCodec:getRecordSize
    uint8_t recordTypeColumnIndex; // eg 0xFF = FIXED_RECORD_TYPE (TODO: nodig?)
    // FIXME: numRecordTypes = necessary
    BDB_recordT* recordTypes; // ptr to array of record types
} BDB_tableT;


typedef struct {
    uint8_t numTables;
    BDB_tableT tables[];
} BDB_dbaseDefT;

#endif

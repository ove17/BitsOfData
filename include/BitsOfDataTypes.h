/*
 * BitsOfDataTypes.h
 *
 * Contains public types for defining a database.
 */

#ifndef __bits_of_data_types_h__
#define __bits_of_data_types_h__

#include <stdint.h>
#include <stdbool.h>


#define MAX_NUM_COLUMNS 6   // FIXME: ADJUST VALUE AS REQUIRED, NEED 6 FOR PASSING TESTS

typedef enum {
    BDB_COLUMN_INTEGER,     // =0, so default
    BDB_COLUMN_RECORD_TYPE,	// for variable record typs
//    BDB_COLUMN_INTEGER_ZEROVAL,	// prints a string instead of 0
//    BDB_COLUMN_INTEGER_MAXVAL,    // prints a string instead of the maximum value
//    BDB_COLUMN_DECIMAL,
//    BDB_COLUMN_CHAR,
    BDB_COLUMN_STRING,		// no data, points to CHAR's
//    BDB_COLUMN_SYMBOL_LIST,
//    BDB_COLUMN_STRING_LIST,
//    BDB_COLUMN_STRING_LISTS,
    BDB_COLUMN_REFERENCE,	// reference to a record (, column) in another table
    BDB_COLUMN_VIRTUAL,		// no data, column in another table
    //	BDB_NUM_COLUMN_TYPES
} BDB_colTypeT;

/*
 * General approach: custom type is preferred over extra parameters
 *
                                      setValue
                         getValue   changeValue	 printValue
 BDB_COLUMN_RECORD_TYPE	recordDefId		ok			n/a
 BDB_COLUMN_INTEGER			int			ok			ok			.leading0
 BDB_COLUMN_INTEGER_ZEROSTR	int			ok			ok			.zeroStr
 BDB_COLUMN_INTEGER_MAXSTR	int			ok			ok			.maxStr
 BDB_COLUMN_DECIMAL			int			ok			ok			.numDecimals, .decMultiplier
 BDB_COLUMN_CHAR			int			ok			ok			.charSet
 BDB_COLUMN_SYMBOL_LIST		int			ok			ok			.symbolListId
 BDB_COLUMN_STRING_LIST		int			ok			ok			.strListId
 BDB_COLUMN_STRING_LISTS	int			ok			ok			.strListIds
 BDB_COLUMN_REFERENCE	    int   	    ok		   refCol       value = recordId in .refTable
 // the following columns hold no data:
 BDB_COLUMN_VIRTUAL		 targetCol		n/a		 targetCol
 BDB_COLUMN_STRING			n/a			n/a			ok			.strFirstChar, .strLength

 *
 */


// TODO: is BOOLEAN een apart type?
typedef struct {
    BDB_colTypeT colType;
    uint16_t minValue; // default = 0
    uint16_t maxValue;
    uint16_t defaultVal;
/*TODO: either the following, OR separate colType structs for each columnType ?
        OR: only colType and externally manage its properties (does not feel good...)
        SHOULD sprintRecord/value live in a separate module?
                PROBABLY!
                SEPARATE formatting (leading0, decMultiplier, stringList) from dbase
                    logic (refTable, virtualValueColumn)
*/
    union {
        struct { 	 // INTEGER:
            bool    intLeading0;
        };
/*        struct { // INTEGER_ZEROVAL:
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
        };*/
        struct { // REFERENCE: data value = recordId in other table
            uint8_t refTable;
            uint8_t refColumn; // in the reference table, generally a string column
        };
        struct { // VIRTUAL: holds no data, points to a column in another table
            uint8_t virtRecordCol; // columnId of REFERENCE_COLUMN in the same table
            uint8_t virtValueCol;  // columnId of target value in referenced table
        };
    };
} BDB_columnT;


// recordDef (also) determines display format!!
//	NOTE: so settings can be record(type)s, rather than columns! Fewer exceptions!
typedef struct {
    uint8_t numColumns;
//    uint8_t formatString;   // TODO id of (translatable) string
                                // AND how does this fit here? or elsewhere?
    BDB_columnT columns[MAX_NUM_COLUMNS];   // pointer to array of column definitions
} BDB_recordT;


/*
 * There are 2 scenarios:
 * 1) the table has only 1 type of record:
 *      - numRecordDefs = 1
 *      - recordDefs[0] is the definition of its record
 * 2) the table has a variable record type:
 *      - numRecordDefs > 1
 *      - recordDefs[] holds an array of record definitions
 */
typedef struct {
    uint8_t maxNumRecords;
    uint8_t numRecordDefs;
    const BDB_recordT* recordDefs;
} BDB_tableT;



typedef struct {
    uint8_t numTables;
    const BDB_tableT* tables;
} BDB_dbaseDefT;


#endif

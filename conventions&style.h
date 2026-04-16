/* conventions&style.h

 Conventions:
 ============

ONLY the public API is tested (and fully tested)

public API :
	functions:	ACRONYM_camelCase()
	types:		ACRONYM_camelCaseT
	variables:	avoid at all cost
	defines:	ACRONYM_UPPER_CASE
	enum values: ACRONYM_lower_case

internal api (module):
	functions:	acronym_camelCase()
	types:		acronym_camelCaseT
	variables:	avoid at all cost
	defines:	acronym_UPPER_CASE
	enum values: acronym_lower_case

static/local (file scope):
	functions:	camelCase()
	types:		camelCaseT
	variables:	CamelCase
	defines:	UPPER_CASE
	enum values: lower_case

local (function scope):
	variables:	camelCase

general:
	struct members: .camelCase

	BitsOfData/
	├── include/			// public API
	│   └── BitsOfData.h
	├── src/
	│   ├── BitsOfData.c
	│   ├── IndexList.c
	│   ├── Cache.c
	│   ├── Records.c
	│   └── internal/		// internal headers, module-private
	│       ├── IndexList.h
	│       ├── Cache.h
	│       ├── Records.h
	│       └── Utils.h
	├── tests/				// may replace *.c during tests
	│   └── mockRecord.c

 */


/* LOWEST DATABASE LEVEL, ABOVE PLAIN LISTS OF BYTES (in EeDirectory)
 *
 * implementation of tables
 * 		every table has an indexList
 *          the indexList contains the index of each record in the table, so:
 *              indexList[0] = 1st record, etc
 * 			the indexList determines the number of records and their order
 * 			the indexList has a fixed maximum number of records (its length)
 *          unused items in the indexList have value 0xFF
 *			the indexList is invisible to the caller
 * 			records are inserted and deleted in the indexList
 * 		every table has a data area
 * 			the size of the data area corresponds to the length of the indexList
 * 			the data are stored in records of n bytes
 *
 * implementation of records
 * 		a table has at least one record
 * 		every record in a table has the same length in bytes
 * 		a record has a recordType that defines its colum types
 * 		if there is a single recordType definition, all records have this type
 * 		in case of multiple recordType definitions, it is a variable recordType table:
 * 			recordType must be the first column
 * 			record size must fit the biggest recordType
 *		possibly the record size in bytes could be hardcoded (instead of inferred)
 *
 * implementation of columns
 *		a record consists of one or more columns
 * 		each column contains an integer with a min and a max value
 * 			or an offset and a range
 * 		a column may be between 1 and 16 bits wide
 * 		columns are packed to the next size up in bytes
 * 		the value can be set, get or changed
 * 			packing and unpacking of columns happens when needed
 *			see also record cache below
 *
 * The entire database may be kept in cache for quick access
 *		alternatively: some records (per table) may be kept in cache, e.g. the last 3 accessed.
 *			in this case they are kept in an a uint16_t recordCache[]
 *		only modified records (or bytes) are written back to EE
 */

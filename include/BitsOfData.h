/*
 * BitsOfData.h
 *
 * public API of BitsOfData : simple database intended for embedded use
 *
 * implementation of records:
 * 		every record in a table has the same length in bytes
 * 		a record has a recordDef that defines its column types
 * 		if there is a single recordDef, all records have this type
 * 		in case of multiple recordDef's, it is a variable recordDef table:
 * 			the first column must hold the record type: .colType = BDB_COLUMN_RECORD_TYPE
 * 			each record will take up the same space, regardless of its size
 *
 * implementation of columns:
 *		each record consists of one or more columns
 * 		each column contains an integer with a min and a max value
 * 		a column may be between 1 and 16 bits wide
 * 		the value can be set, get or changed
 * 		custom column types are available, but:
 * 			they are represented in printed values only
 * 			set, get and change only work with int's
 */

#ifndef BITS_OF_DATA_H
#define BITS_OF_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "BitsOfDataTypes.h"


bool BDB_openDataBase(const BDB_dbaseDefT* dbaseDef);
void BDB_closeDataBase(void);

uint16_t BDB_getValue(const uint8_t tableId,
					  const uint8_t recordId,
					  const uint8_t columnId);
bool BDB_setValue(const uint8_t tableId,
				  const uint8_t recordId,
				  const uint8_t columnId,
				  const uint16_t value);
bool BDB_changeValue(const uint8_t tableId,
					 const uint8_t recordId,
					 const uint8_t columnId,
					 const int16_t delta);
void BDB_storeRecord(const uint8_t tableId,
					 const uint8_t recordId);

char* BDB_getWriteBuffer(void);
uint8_t BDB_writeValue(const uint8_t tableId,
					   const uint8_t recordId,
					   const uint8_t columnId,
					   const uint8_t startPosition);
/*
 * char* BDB_sPrintRecord(const uint8_t tableId,
					   const uint8_t recordId);
*/

uint16_t* BDB_getRecord(const uint8_t tableId,
						const uint8_t recordId);
void BDB_setRecord(const uint8_t tableId,
				   const uint8_t recordId,
				   const uint16_t* data);

uint8_t BDB_getNumRecords(const uint8_t tableId);

bool BDB_canRecordBeAdded(const int8_t tableId);
// returns true if record was inserted:
bool BDB_insertRecordAfter(const int8_t tableId,
						   const uint8_t recordId);

// not if other records depend on it (BDB_COLUMN_REFERENCE)
bool BDB_canRecordBeDeleted(const uint8_t tableId,
							const uint8_t record);
// returns 0 if record was not deleted
bool BDB_deleteRecord(const uint8_t tableId,
					  const uint8_t recordId);

#endif

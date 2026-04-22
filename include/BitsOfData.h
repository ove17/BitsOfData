/*
 * BitsOfData.h
 *
 * public API of BitsOfData : simple database intended for embedded use
 *
 *
 *
 * setup dbase in EE (zonder columns)
 * 		kan nu al
 * 		behalve vullen met lege records
 * 		tableDef: numRecords
 * 		columnDef: recordSize
 *

 *
 * import/export table
 * import/export record
 * 		setRecord ipv setRawRecord ?
 *
 *
 *
 * implementation of records
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
 */

#ifndef __bits_of_data_h__
#define __bits_of_data_h__

#include <stdint.h>
#include <stdbool.h>
#include "BitsOfDataTypes.h"


void BDB_openDataBase(const BDB_dbaseDefT* dbaseDef);

bool BDB_setValue(const uint8_t tableId,
				  const uint8_t recordId,
				  const uint8_t columnId,
				  const uint16_t value);
uint32_t BDB_getValue(const uint8_t tableId,
					  const uint8_t recordId,
					  const uint8_t columnId);
// returns 0 if value did not change:
bool BDB_changeValue(const uint8_t tableId,
					 const uint8_t recordId,
					 const uint8_t columnId,
					 const uint16_t delta);

uint8_t BDB_getNumRecords(const uint8_t table);

char* BDB_sPrintValue(const uint8_t tableId, // TODO: is this fn necessary?
					  const uint8_t recordId,
					  const uint8_t columnId);
char* BDB_sPrintRecord(const uint8_t tableId,
					   const uint8_t recordId);


bool BDB_canRecordBeDeleted(const uint8_t tableId,
							const uint8_t record);

// returns 0 if record was not deleted
bool BDB_deleteRecord(const uint8_t tableId,
					  const uint8_t record);

bool BDB_canRecordBeInsertedAfter(const int8_t tableId,
								  const uint8_t record);

// returns 0 if record was not inserted
bool BDB_insertRecordAfter(const int8_t tableId,
						   const uint8_t record);

#endif

/* BitsOfData.h
 *
 * Public API of BitsOfData:
 * A simple embedded database.
 *
 * Schema definition is documented in BitsOfDataTypes.h.
 * The schema is defined at compile time and is immutable.
 *
 * Record IDs are positional indices within a table.
 * Insert/delete operations may shift records and change the mapping
 * between recordId and the actual record.
 *
 * All data values are exposed as uint16_t through the API,
 * regardless of their logical schema type.
 *
 * BDB API contract:
 *
 * All API calls require valid identifiers.
 * Invalid tableId, recordId or columnId values are considered
 * programming errors and will trigger an assert (by design).
 */


#ifndef BITS_OF_DATA_H
#define BITS_OF_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "BitsOfDataTypes.h"


/*
 * Callback used to translate a text ID into a string.
 *
 * The returned string is read-only and must remain valid for the lifetime of
 *	the program. No allocation or deallocation is required by the caller.
 *
 * Parameters:
 *   outStr - pointer to a const string pointer (points to
 *            static read-only storage)
 *   txtId  - numeric text identifier
 *
 * Returns:
 *   the length of the returned string
 */
typedef uint8_t (*BDB_txtHandlerFunction)(const char** outStr, uint8_t txtId);


/*
 * Opens an existing database or creates a new one if none exists.
 *
 * Returns:
 *   true  - existing database was found and opened
 *   false - no database existed and a new one was created
 *
 * The caller must provide:
 *   - dbaseDef: schema and presentation definition
 *   - txtHandler: callback translating txtId values to strings
 *
 * In debug builds (when NDEBUG is not defined), the schema is
 * validated and an assertion is triggered if it is invalid.
 */
bool BDB_openDataBase(const BDB_dbaseDefT* dbaseDef,
					  BDB_txtHandlerFunction txtHandler);

/*
 * Closes the database and deallocates all allocated RAM.
 * NOTE: avoid opening and closing repeatedly, as it will lead to fragmentation
 */
void BDB_closeDataBase(void);


// Managing records:
// =================

/*
 * Returns the current number of records in the table.
 * This value is always <= the maximum defined in the schema.
 */
uint8_t BDB_getNumRecords(const uint8_t tableId);

/*
 * Returns false if the actual number of records is < than
 * 	the maximum defined in the schema.
 */
bool BDB_canRecordBeAdded(const int8_t tableId);

/*
 * Inserts a new record after the one with recordId, shifting
 * 	subsequent records.
 * Returns false if the record could not be inserted.
 * On success it increases the actual number of records by 1 and
 * 	sets its columns to the defaults as defined by the schema.
 */
bool BDB_insertRecordAfter(const int8_t tableId,
						   const uint8_t recordId);

/*
 * Returns false if the record is referenced by any other record or
 * 	if it is the last remaining record.
 */
bool BDB_canRecordBeDeleted(const uint8_t tableId,
							const uint8_t recordId);

/*
 * Deletes the record with recordId.
 * Subsequent records are shifted to fill the gap.
 *
 * On success:
 *   - the number of records decreases by 1
 *   - after mutation, recordId refers to the record that previously followed
 *		the deleted one (if it exists)
 *   - otherwise, accessing recordId triggers an assert due to invalid index
 *
 * Returns false if the record cannot be deleted.
 */
bool BDB_deleteRecord(const uint8_t tableId,
					  const uint8_t recordId);


/*
 * Returns a read-only view of the record, which is a an array of column values
 * 	with a length that is guaranteed to be >= numRealColumns.
 * Modifications must be done through BDB_setValue or BDB_setRecord.
 *
 * The pointer remains valid until BDB_closeDataBase(), but its contents may
 *	change when other records are accessed.
 */
const uint16_t* BDB_getRecord(const uint8_t tableId,
						const uint8_t recordId);

/*
 * Copies data into the table's record buffer.
 * The input data is not required after the call.
 *
 * The update is stored in RAM and is only persisted to non-volatile storage
 * 	when:
 *   - another record in the same table is accessed, or
 *   - BDB_syncTable() is called.
 *
 * Returns false if the provided data does not conform to the schema.
 */
bool BDB_setRecord(const uint8_t tableId,
				   const uint8_t recordId,
				   const uint16_t* data);


// Managing columns:
// =================

/*
 * Returns the number of non-virtual columns in a record.
 */
uint8_t BDB_getNumRealColumns(const uint8_t tableId,
							  const uint8_t recordId);

/*
 * Returns a value from the database as a uint16_t, regardless of the
 * 	recordType (documented in BitsOfDataTypes.h)
 *
 * Interpretation of the value depends on the schema definition.
 */
uint16_t BDB_getValue(const uint8_t tableId,
					  const uint8_t recordId,
					  const uint8_t columnId);

/*
 * Sets a value in the table's record buffer.
 *
 * This update is stored in RAM and is only persisted to non-volatile storage
 * 	when:
 *   - another record in the same table is accessed, or
 *   - BDB_syncTable() is called.
 *
 * Returns false if the provided data does not conform to the schema.
 */
bool BDB_setValue(const uint8_t tableId,
				  const uint8_t recordId,
				  const uint8_t columnId,
				  const uint16_t value);

/*
 * Changes a value in the table's record buffer.
 *
 * This update is stored in RAM and is only persisted to non-volatile storage
 * 	when:
 *   - another record in the same table is accessed, or
 *   - BDB_syncTable() is called.
 *
 * If the change would exceed schema limits, it is truncated ot the limit.
 * Returns false if the data in the buffer after changeValue != the value before.
 */
bool BDB_changeValue(const uint8_t tableId,
					 const uint8_t recordId,
					 const uint8_t columnId,
					 const int16_t delta);

/*
 * Writes the recordBuffer of one table to non-volatile storage.
 */
void BDB_syncTable(const uint8_t tableId);

/*
 * Writes the recordBuffer of all tables to non-volatile storage.
 */
void BDB_syncDbase(void);


// import data:
// ============

/*
 * Data is imported per record and validated against their .min/maxVal's as it
 * is written.
 * As soon as any invalid data is encountered, the import will halt and the
 * number of successfully written records will be returned.
 *
 * Returns the number of successfully written records, so the caller should
 * check that the return value is equal to numRecords.
 *
 * NOTE: Existing data in this table is overwritten!
 */
uint8_t BDB_importTable(const uint8_t tableId,
						const uint16_t* data,
						const uint8_t numRecords);


// parent/child:
// =============


/*
 * Returns the table id of the parent of tableId. Calling this function on a
 * table that has no parent will result in an assertion failure.
 */
uint8_t BDB_getParentTable(const uint8_t tableId);


/*
 * Returns the record id of the parent of tableId. Calling this function on a
 * table that has no parent will result in an assertion failure.
 */
uint8_t BDB_getParentRecord(const uint8_t tableId);


// presentation:
// =============

/*
 * Returns a pointer to the database write buffer.
 * The buffer size is defined by maxStringBufferSize in the database schema
 *  (see BitsOfDataTypes.h).
 *
 * This buffer is filled by BDB_writeValue() and BDB_writeRecord().
 *
 * The pointer remains valid until BDB_closeDataBase().
 * The contents may be overwritten by subsequent write operations.
 *
 * The buffer is shared and mutable by design.
 */
char* BDB_getWriteBuffer(void);


/*
 * Writes the string representation of a database value into the write buffer.
 *
 * The value is formatted according to the column definition (.colType in
 * BDB_columnT, see BitsOfDataTypes.h).
 *
 * startPosition defines the offset in the write buffer where output begins.
 * Existing data at and after this position may be overwritten.
 *
 * Returns the buffer index immediately after the last written character.
 * Writing beyond the size of the buffer triggers an assert.
 */
uint8_t BDB_writeValue(const uint8_t tableId,
					   const uint8_t recordId,
					   const uint8_t columnId,
					   const uint8_t startPosition);

/*
 * Writes the string representation of a database record to the write buffer.
 *
 * This operation overwrites the entire write buffer. Any previous content is
 *  discarded. The resulting string is padded with spaces, if necessary, to
 *  fill the entire buffer.
 *
 * The output format is defined by BDB_recordT.txtFormat in BDB_recordT (see
 *  BitsOfDataTypes.h).
 */
void BDB_writeRecord(const uint8_t tableId,
					 const uint8_t recordId);


/*
 * Writes the string representation of a database table header to the write buffer.
 *
 * This operation overwrites the entire write buffer. Any previous content is
 *  discarded. The resulting string is padded with spaces, if necessary, to
 *  fill the entire buffer.
 *
 * The output format is defined by BDB_tableT.headerFormat in BDB_recordT (see
 *  BitsOfDataTypes.h).
 *
 * The recordId is an input, so that current record properties can be shown in
 *  the header, e.g. recordId
 */
void BDB_writeHeader(const uint8_t tableId,
					 const uint8_t recordId);


/*
 * Writes the string representation of a database record into the write buffer.
 *
 * This function works identically to writeRecord, but instead of getting the
 *  format from the database schema, the user can provide a custom format. The
 *  caller is responsible for providing a \0 terminated string.
 */
void BDB_writeRecordWithFormat(const uint8_t tableId,
							   const uint8_t recordId,
							   const char* txtFormat);

#endif

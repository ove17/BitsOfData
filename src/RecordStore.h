/* RecordStore.h
 *
 * This module manages tables with records of bytes which are stored in EE.
 *
 * USAGE:
 *
 * Call rs_tryToOpenRecordStore() to load an existing record store. If it
 *  returns true, the recordStore is ready for use.
 *
 * If no valid record store exists, initialize a new one by:
 *   - calling rs_createTable() for each table
 *   - calling rs_commitTables() to write them to EE and start using it.
 * After this, the record store is ready for use.
 *
 * The record store contains between 1 and 250 tables, each table contains
 * between 1 and 250 records, each record is 1 to 250 bytes long.
 *
 * Record contents are opaque byte arrays managed by the caller.
 * Newly created or freed records are not guaranteed to contain any specific
 *  value (often 0xFF, but may vary after reuse).
 *
 * Record access returns a pointer to a persistent buffer.
 *  - The buffer is owned by the RecordStore and reused for subsequent calls.
 *  - Its content is overwritten by each call to rs_getRawRecord().
 *  - The buffer length equals the record size of the table.
 *
 * Records are set using a pointer to a fixed-size byte array provided by the
 *  caller.
 *  - The RecordStore does not take ownership of the buffer and does not retain
 *  the pointer.
 *
 * The record store has a fixed schema defined at creation time:
 *      - number of tables
 *      - maximum number of records per table
 *      - record size
 *
 * Changing the schema requires recreating the record store, which destroys all
 *      existing content.
 */

#ifndef RECORD_STORE_H
#define RECORD_STORE_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_NUM_RECORDS_REACHED 0xFF


/*
 * Opens a recordStore if one exists. In that case it allocates RAM for buffers
 *  and returns true.
 * If no valid recordStore was found, EE is erased and false is returned.
 */
bool rs_tryToOpenRecordStore(const uint8_t numTables);

/*
 * Closes the record store and decallocates all buffer RAM.
 * Do not repeatedly open and close recordstore as that will fragment RAM.
 */
void rs_closeRecordStore(void);

/*
 * This function MUST be called after creating tables, it writes them to non-
 *  volatile memory and allocates RAM for buffers.
 */
void rs_commitTables(void);

/*
 * Deletes the recordStore from EE, it does not completely clear EE, it just
 *  makes it invalid, which causes clearing of EE the next time
 *  rs_tryToOpenRecordStore is called.
 */
void rs_deleteTableCatalog(void);


/*
 * Creates a new table and returns its tableId.
 * It will fail an assert if there is not enough space for the table.
 * The table is not written to EE, only its lookup tables are stored in RAM.
 */
uint8_t rs_createTable(const uint8_t maxNumRecords,
                       const uint8_t recordSize);

/*
 * Returns the current number of records (i.e. not the maximum).
 */
uint8_t rs_getNumRecords(const uint8_t tableId);

/*
 * Appends a record after the last record.
 * Returns the recordIndex of the created record.
 * Increases numRecords by 1
 */
uint8_t rs_appendRecord(const uint8_t tableId);

/*
 * Inserts a record after recordId, the recordId of all subsequent records
 *  shifts up by 1.
 * Increases numRecords by 1
 * Returns the recordIndex of the newly created record.
 * Returns MAX_NUM_RECORDS_REACHED if there is no more space.
 */
uint8_t rs_insertRecordAfter(const uint8_t tableId,
                             const uint8_t recordId);

/*
 * Deletes a record at recordId, the recordId of all subsequent records shifts
 *  down by 1.
 * Decreases numRecords by 1.
 */
void rs_deleteRecord(const uint8_t tableId,
                     const uint8_t recordId);

/*
 * Deletes all records in a table and sets numRecords to 0
 * Returns the number of removed records.
 */
uint8_t rs_deleteAllRecords(const uint8_t tableId);


/*
 * Writes data to a record in EE.
 */
void rs_setRawRecord(const uint8_t tableId,
                     const uint8_t recordId,
                     uint8_t* rawRecord);

/*
 * Copies the record data from EE to the data buffer.
 * Returns the pointer to the module-internal buffer array of this table.
 * The pointer remains valid for the life of the program and must not be freed.
 * The data buffer is read-only.
 */
const uint8_t* rs_getRawRecord(const uint8_t tableId,
                               const uint8_t recordId);

#endif

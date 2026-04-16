/* RecordStore.h
 *
 * This module manages tables with records of bytes which are stored in EE.
 *
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
 * between 1 and 250 records, each record is 1 to 250 bytes.
 *
 * Record contents are opaque byte arrays managed by the caller.
 * Newly created or freed records are not guaranteed to contain any specific
 *  value (often 0xFF, but may vary after reuse).
 *
 * Record access returns a pointer to a persistent buffer.
 *  - The buffer is owned by the RecordStore and reused for subsequent calls.
 *  - Its content is overwritten by each call to rs_getRecord().
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
 *      - record
 *
 * Changing the schema requires recreating the record store, which destroys all
 *      existing content.
 *
 */

#ifndef __record_store_h__
#define __record_store_h__

#include <stdint.h>
#include <stdbool.h>


bool rs_tryToOpenRecordStore(const uint8_t numTables);
void rs_closeTableCatalog();    // for testing only!
void rs_commitTables();
void rs_deleteTableCatalog();

uint8_t rs_createTable(const uint8_t maxNumRecords,
                       const uint8_t recordSize);

uint8_t rs_getNumRecords(const uint8_t table);

uint8_t rs_appendRecord(const uint8_t table);
uint8_t rs_insertRecordAfter(const uint8_t table,
                             const uint8_t record);
bool rs_deleteRecord(const uint8_t table,
                     const uint8_t record);
uint8_t rs_deleteAllRecordsIn(const uint8_t table);

void rs_setRecord(const uint8_t table,
                  const uint8_t record,
                  uint8_t* rawRecord);
uint8_t* rs_getRecord(const uint8_t table,
                      const uint8_t record);

#endif

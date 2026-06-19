/*
 * RecordCodec.h
 *
 * This is a private API from BitsOfData, it is NOT intended to be called
 *  directly or used stand-alone.
 *
 * Packs and unpacks records, this is the step between the public API of the
 *  database and low-level (EE) storage.
 *
 * All data and the database schema are assumed to be valid, there is no error
 *  checking, not even assertion.
 */

#ifndef RECORD_CODEC_H
#define RECORD_CODEC_H

#include <stdint.h>
#include <stdbool.h>
#include <BitsOfDataTypes.h>


/*
 * Returns the record size of a table in bytes
 * if it has multiple record types: the biggest record size is returned.
 */
uint8_t rc_getMaxRecordSize(const BDB_tableT* tableDef);

/*
 * Returns the record size in bytes. This is the packed size taken up by all
 * columns in one record.
 */
uint8_t rc_getRecordSize(const BDB_recordT* recordDef);

/*
 * Returns true if the column is virtual, i.e. it holds no data.
 */
static inline bool rc_isVirtualColumn(const BDB_columnT* columnDef) {
    return columnDef->colType == BDB_COLUMN_VIRTUAL || columnDef->colType == BDB_COLUMN_STRING
        || columnDef->colType == BDB_COLUMN_TXT_LIST_CLONE; //FIXME
}

/*
 * Packs the data of record (array of columns) into a packed byte sequence.
 */
void rc_encodeRecord(const uint16_t recordData[],   // input (separate values)
                     uint8_t rawRecord[],           // output (packed)
                     const BDB_tableT* tableDef);

/*
 * Unpacks a byte sequence to a record (array of columns).
 */
void rc_decodeRecord(const uint8_t rawRecord[], // input (packed)
                     uint16_t recordData[],     // output (separate values)
                     const BDB_tableT* tableDef);

#endif

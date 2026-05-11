/*
 * RecordCodec.h
 *
 * unpacks a raw record (array of bytes) into an array of uint16 column values
 * packs a record (array uint16 column values) into a raw record
 *
 */

#ifndef RECORD_CODEC_H
#define RECORD_CODEC_H

#include <stdint.h>
#include <stdbool.h>
#include <BitsOfDataTypes.h>


uint8_t rc_getMaxRecordSize(const BDB_tableT* tableDef);
uint8_t rc_getRecordSize(const BDB_recordT* recordDef);

static inline bool rc_isVirtualColumn(const BDB_columnT* columnDef) {
    return columnDef->colType == BDB_COLUMN_VIRTUAL || columnDef->colType == BDB_COLUMN_STRING;
}

void rc_encodeRecord(const uint16_t recordData[],   // input (separate values)
                     uint8_t rawRecord[],           // output (packed)
                     const BDB_tableT* tableDef);

void rc_decodeRecord(const uint8_t rawRecord[], // input (packed)
                     uint16_t recordData[],     // output (separate values)
                     const BDB_tableT* tableDef);

#endif

/*
 * RecordCodec.h
 *
 * unpacks a raw record (array of bytes) into an array of uint16 column values
 * packs a record (array uint16 column values) into a raw record
 *
 */

#ifndef __record_codec_h__
#define __record_codec_h__

#include <stdint.h>
#include <stdbool.h>
#include <BitsOfDataTypes.h>


uint8_t rc_getRecordSize(const BDB_recordT* recordDef);

void rc_encodeRecord(const uint16_t recordData[],
                     uint8_t rawRecord[],
                     const BDB_recordT* recordDef);

void rc_decodeRecord(const uint8_t rawRecord[],
                     uint16_t recordData[],
                     const BDB_recordT* recordDef);

#endif

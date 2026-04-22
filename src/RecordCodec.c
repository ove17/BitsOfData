/*
 * RecordCodec.c
 *
 */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "BitUtils.h"
#include "BitsOfDataTypes.h"


static uint8_t getNumBitsOfColumn(const BDB_recordT* recordDef,
                                  const uint8_t column);


// NOTE: column type is irrelevant
uint8_t rc_getRecordSize(const BDB_recordT* recordDef) {
    assert(recordDef->numColumns > 0);
    uint16_t numBits = 0;
    for (uint8_t col = 0; col < recordDef->numColumns; col++) {
        numBits += getNumBitsOfColumn(recordDef, col);
    }
//    return (uint8_t)((numBits + 7) / 8);
    return bu_getNumBytes(numBits);
}


// NOTE: column type is irrelevant
void rc_encodeRecord(const uint16_t recordData[],   // input (separate values)
                     uint8_t rawRecord[],           // output (packed)
                     const BDB_recordT* recordDef) {
    uint32_t bitBuffer = 0;
    uint8_t bitsInBuffer = 0;
    uint8_t outIndex = 0;

    for (uint8_t col = 0; col < recordDef->numColumns; col++) {
        uint8_t numBits = getNumBitsOfColumn(recordDef, col);

        uint32_t value = recordData[col] & bu_truncateMask(numBits);

        // add the entire value to the bitBuffer:
        bitBuffer = (bitBuffer << numBits) | value;
        bitsInBuffer += numBits;

        // write full bytes to output array
        while (bitsInBuffer >= 8) {
            bitsInBuffer -= 8;
            rawRecord[outIndex++] = (bitBuffer >> bitsInBuffer) & 0xFF;
        }
    }
    // write remaining bits in bitBuffer to output array:
    if (bitsInBuffer > 0) {
        rawRecord[outIndex] = (bitBuffer << (8 - bitsInBuffer)) & 0xFF;
    }
}


// NOTE: column type is irrelevant
void rc_decodeRecord(const uint8_t rawRecord[], // input (packed)
                     uint16_t recordData[],     // output (separate values)
                     const BDB_recordT* recordDef) {
    uint32_t bitBuffer = 0;
    uint8_t bitsInBuffer = 0;
    uint8_t inIndex = 0;

    for (uint8_t col = 0; col < recordDef->numColumns; col++) {
        uint8_t numBits = getNumBitsOfColumn(recordDef, col);

        while (bitsInBuffer < numBits) {
            bitBuffer = (bitBuffer << 8) | rawRecord[inIndex++];
            bitsInBuffer += 8;
        }

        uint8_t shift = bitsInBuffer - numBits;
        recordData[col] = (uint16_t)((bitBuffer >> shift) & bu_truncateMask(numBits));

        bitsInBuffer -= numBits;
        bitBuffer &= bu_truncateMask(bitsInBuffer); // keep remaining bits
    }
}


static uint8_t getNumBitsOfColumn(const BDB_recordT* recordDef,
                                  const uint8_t column) {
    BDB_columnT colDef = recordDef->columns[column];
    uint16_t colSize = colDef.maxValue - colDef.minValue;
    return bu_getNumBits(colSize);
}

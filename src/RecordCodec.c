/*
 * RecordCodec.c
 *
 * Space for the output arrays must be allocated by the caller
 */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "BitUtils.h"
#include "BitsOfDataTypes.h"
#include "RecordCodec.h"


static uint8_t getNumBitsOfColumn(const BDB_recordT* recordDef,
                                  const uint8_t column) {
    const BDB_columnT* colDef = &recordDef->columns[column];
    uint16_t colSize = colDef->maxValue - colDef->minValue;
    if (colDef->colType == BDB_COLUMN_DECIMAL && colDef->decStep > 1) {
        colSize /= colDef->decStep;
    }
    return bu_getNumBits(colSize);
}


// api:


// returns the packed size of a record in bytes
// if it has a variable record definition: the biggest size is returned
uint8_t rc_getMaxRecordSize(const BDB_tableT* tableDef) {
    uint8_t numRecordDefs = tableDef->numRecordDefs;
    uint8_t maxRecordSize = 0;
    for (uint8_t recordDefId = 0; recordDefId < numRecordDefs; recordDefId++) {
        const BDB_recordT* recordDef = &tableDef->recordDefs[recordDefId];
        uint8_t recordSize = rc_getRecordSize(recordDef);
        if (recordSize > maxRecordSize) {
            maxRecordSize = recordSize;
        }
    }
    return maxRecordSize;
}


// returns compressed record size in bytes
uint8_t rc_getRecordSize(const BDB_recordT* recordDef) {
    assert(recordDef->numColumns > 0);
    uint16_t numBits = 0;
    for (uint8_t col = 0; col < recordDef->numColumns; col++) {
        numBits += getNumBitsOfColumn(recordDef, col);
    }
    return bu_getNumBytes(numBits);
}


void rc_encodeRecord(const uint16_t recordData[],   // input (separate values)
                     uint8_t rawRecord[],           // output (packed)
                     const BDB_tableT* tableDef) {
    const uint8_t numRecordDefs = tableDef->numRecordDefs;
    uint8_t recordDefId = 0;
    if (numRecordDefs > 1) {
        recordDefId = (uint8_t)recordData[0];
    }
    const BDB_recordT* recordDef = &tableDef->recordDefs[recordDefId];

    uint32_t bitBuffer = 0;
    uint8_t bitsInBuffer = 0;
    uint8_t outIndex = 0;

    for (uint8_t col = 0; col < recordDef->numColumns; col++) {
        uint8_t numBits = getNumBitsOfColumn(recordDef, col);
        const BDB_columnT* columnDef = &recordDef->columns[col];
        uint16_t minVal = columnDef->minValue;
        uint32_t value = (recordData[col] - minVal);
        if (columnDef->colType == BDB_COLUMN_DECIMAL && columnDef->decStep > 1) {
            value /= columnDef->decStep;
        }
        value &= bu_truncateMask(numBits);

        // add the entire value to the bitBuffer:
        bitBuffer = (bitBuffer << numBits) | value;
        bitsInBuffer += numBits;

        // write full bytes to output array:
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

void rc_decodeRecord(const uint8_t rawRecord[], // input (packed)
                     uint16_t recordData[],     // output (separate values)
                     const BDB_tableT* tableDef) {
    const uint8_t numRecordDefs = tableDef->numRecordDefs;

    uint8_t recordDefId = 0;
    if (numRecordDefs > 1) { // need recordTypeId first
        const BDB_recordT* tmpRecordDef = &tableDef->recordDefs[0];
        uint8_t numBits = getNumBitsOfColumn(tmpRecordDef, 0);
        uint8_t shift = 8 - numBits;
        recordDefId = (uint8_t)((rawRecord[0] >> shift) & bu_truncateMask(numBits));
    }
    const BDB_recordT* recordDef = &tableDef->recordDefs[recordDefId];

    uint32_t bitBuffer = 0;
    uint8_t bitsInBuffer = 0;
    uint8_t inIndex = 0;

    for (uint8_t col = 0; col < recordDef->numColumns; col++) {
        uint8_t numBits = getNumBitsOfColumn(recordDef, col);
        const BDB_columnT* columnDef = &recordDef->columns[col];

        while (bitsInBuffer < numBits) {
            bitBuffer = (bitBuffer << 8) | rawRecord[inIndex++];
            bitsInBuffer += 8;
        }
        if (rc_isVirtualColumn(columnDef)) {
            recordData[col] = 0xFFFF;   // column value must not be used
        } else {
            uint8_t shift = bitsInBuffer - numBits;
            uint16_t minValue = columnDef->minValue;
            recordData[col] = (uint16_t)(bitBuffer >> shift);
            recordData[col] &= bu_truncateMask(numBits);
            if (columnDef->colType == BDB_COLUMN_DECIMAL && columnDef->decStep > 1) {
                recordData[col] *= columnDef->decStep;
            }
            recordData[col] += minValue;
        }

        bitsInBuffer -= numBits;
        bitBuffer &= bu_truncateMask(bitsInBuffer); // keep remaining bits
    }
}

/*
 * BitUtils.h
 */

#ifndef BIT_UTILS_H
#define BIT_UTILS_H


#include <stdint.h>
#include <assert.h>


// returns the byte which stores the bitIndex'th bit
static inline uint8_t bu_getByteIndex(const uint8_t bitIndex) {
    return bitIndex >> 3;
}


// returns the minimum number of bytes necessary to store numBits
static inline uint8_t bu_getNumBytes(const uint16_t numBits) {
    assert (numBits < 2033);
    return (uint8_t)((numBits + 7) / 8);
}


// Returns a mask with a single bit set at position (value % 8)
static inline uint8_t bu_getSingleBitMask(const uint8_t value) {
    return (uint8_t)(1U << (value & 0b0111));
}


// returns a mask to truncate to numBits
static inline uint32_t bu_truncateMask(const uint8_t numBits) {
    return (1U << numBits) - 1U;
}


// returns the minimum number of bits necessary to store value
static inline uint8_t bu_getNumBits(uint16_t value) {
    uint8_t numBits = 0;
    while (value > 0) {
        value >>= 1;
        numBits++;
    }
    return numBits;
}

#endif

/*
 * BitUtils.h
 */

#ifndef __bit_utils_h__
#define __bit_utils_h__


#include <stdint.h>


// returns the byte which stores the bitIndex'th bit
uint8_t bu_getByteIndex(const uint8_t bitIndex);

// returns the minimum number of bits necessary to store value
uint8_t bu_getNumBits(uint16_t value);

// returns the minimum number of bytes necessary to store numBits
uint8_t bu_getNumBytes(const uint16_t numBits);

// Returns a mask with a single bit set at position (value % 8)
uint8_t bu_getSingleBitMask(const uint8_t value);

// returns a mask to truncate to numBits
uint32_t bu_truncateMask(const uint8_t numBits);

#endif

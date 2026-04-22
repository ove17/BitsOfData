/*
 * BitUtils.c
 */


#include <stdint.h>
#include <assert.h>


uint8_t bu_getByteIndex(const uint8_t bitIndex) {
    return bitIndex >> 3;
}


uint8_t bu_getNumBits(uint16_t value) {
    uint8_t numBits = 0;
    while (value > 0) {
        value >>= 1;
        numBits++;
    }
    return numBits;
}


uint8_t bu_getNumBytes(const uint16_t numBits) {
    assert (numBits < 2033);
    return (uint8_t)((numBits + 7) / 8);
}


uint8_t bu_getSingleBitMask(const uint8_t value) {
    return (uint8_t)(1U << (value & 0b0111));
}


uint32_t bu_truncateMask(const uint8_t numBits) {
    return (1U << numBits) - 1U;
}



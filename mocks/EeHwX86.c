/** @file EeHwX86.c
 *  Low-level hardware-dependent functions to read and write EEPROM
 */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "EeHw.h"
#include "EeHwX86.h"


static uint8_t _fakeEE[EEPROM_SIZE];


static bool eeAddressValid(const eeAddress_t eeAddress);


void eeInit() {}

// real EE is clear by default
void eeClear() {
	for (eeAddress_t i = 0; i < EEPROM_SIZE; i++)
		_fakeEE[i] = EE_CLEAR_VALUE;
}

eeAddress_t eeGetSize() {
    return EEPROM_SIZE;
}

uint8_t eeReadUint8(const eeAddress_t eeAddress) {
    if (eeAddressValid(eeAddress)) {
		return _fakeEE[eeAddress];
    } else
        return 0x0F; // ERROR
}

void eeWriteUint16(const eeAddress_t eeAddress,
                            const uint16_t value) {
    eeWriteUint8(eeAddress, (uint8_t)(value & 0x0FF));
    eeWriteUint8(eeAddress + 1, (uint8_t)(value >> 8));
}


uint16_t eeReadUint16(const eeAddress_t eeAddress) {
    return (uint16_t)(eeReadUint8(eeAddress + 1) << 8) + eeReadUint8(eeAddress);
}

void eeWriteUint8(const eeAddress_t eeAddress, const uint8_t eeData) {
    if (eeAddressValid(eeAddress)) {
		_fakeEE[eeAddress] = eeData;
    }
}


void eeReadUint8Array(const eeAddress_t eeAddress,
                      uint8_t *buffer,
                      const uint16_t numBytes) {
    assert(numBytes <= EEPROM_PAGE_SIZE);
    for (uint16_t i=0; i<numBytes; i++) {
        buffer[i] = eeReadUint8(eeAddress + i);
    }
}


uint16_t eeGetPageSize() {
    return EEPROM_PAGE_SIZE;
}


void eeReadPage(const eeAddress_t eeAddress,
                uint8_t *buffer) {
    eeReadUint8Array(eeAddress, buffer, EEPROM_PAGE_SIZE);
}


void dumpFakeEe(uint16_t startLine, uint8_t numLines, uint8_t bytesPerLine) {
    printf("\nEE size = %i\n", eeGetSize());
    printf("\nline\tB0\tB1\tB2\tB3\tB4\n");
    for (int line=startLine; line<startLine+numLines; line++) {
        printf("%3i\t", line);
        for (int col=0; col<bytesPerLine; col++) {
            eeAddress_t addr = (eeAddress_t)(col+bytesPerLine*line);
            if (eeAddressValid(addr)) {
                printf("%3i\t", eeReadUint8(addr));
            } else {
                printf("  x\t");
            }
        }
        printf("\n");
    }
    printf("\n");
}


void dumpEeTable(uint8_t tableId, uint8_t bytesPerRecord) {
    printf("\nrecord");
    for (int i=0; i < bytesPerRecord; i++) {
        printf("\tB%i", i);
    }
    printf("\n");

    eeAddress_t eeAddress = 2 * tableId - 1;
    int start = (eeReadUint8(eeAddress + 1) << 8) + eeReadUint8(eeAddress);
    eeAddress += 2;
    int end =  (eeReadUint8(eeAddress + 1) << 8) + eeReadUint8(eeAddress);
    int numRecords = (end - start) / bytesPerRecord;

    printf("n=%i\n", numRecords);

    for (int record = 0; record < numRecords; record++) {
        printf("%3i\t", record);
        for (int col = 0; col < bytesPerRecord; col++) {
            eeAddress_t addr = (eeAddress_t)(start + col + bytesPerRecord*record);
            if (eeAddressValid(addr)) {
                printf("%3i\t", eeReadUint8(addr));
            } else {
                printf("  x\t");
            }
        }
        printf("\n");
    }
    printf("\n");
}

static bool eeAddressValid(const eeAddress_t eeAddress) {
    return (eeAddress < EEPROM_SIZE);
}

